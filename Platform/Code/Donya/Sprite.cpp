#include "Sprite.h"

#include <array>
#include <d3d11.h>
#include <memory>
#include <tchar.h>
#include <unordered_map>
#include <vector>
#include <WICTextureLoader.h>

#include "Constant.h"
#include "Direct3DUtil.h"
#include "Donya.h"
#include "Random.h"
#include "RenderingStates.h"
#include "Resource.h"
#include "ScreenShake.h"
#include "Shader.h"
#include "Surface.h"
#include "Useful.h"

#undef min
#undef max

using namespace DirectX;

namespace Donya
{
	// TODO:埋め込みシェーダと，他のシェーダの使用を切り替えられるようにする

	namespace Sprite
	{
		// HACK : The EmbeddedSourceCode, Shared namespaces has many copy-pasted code... :(

		namespace EmbeddedSourceCode
		{
			constexpr const char *EntryPointVS	= "VSMain";
			constexpr const char *EntryPointPS	= "PSMain";

			constexpr const char *DisplayCode()
			{
				return
				"Texture2D		diffuseMap			: register( t0 );\n"
				"SamplerState	diffuseMapSampler	: register( s0 );\n"
				"struct VS_IN\n"
				"{\n"
				"	float4		pos					: POSITION;\n"
				"	float4		color				: COLOR;\n"
				"	float2		texCoord			: TEXCOORD;\n"
				"};\n"
				"struct VS_OUT\n"
				"{\n"
				"	float4		pos					: SV_POSITION;\n"
				"	float4		color				: COLOR;\n"
				"	float2		texCoord			: TEXCOORD;\n"
				"};\n"
				"VS_OUT VSMain( VS_IN vin )\n"
				"{\n"
				"	VS_OUT vout		= ( VS_OUT )0;\n"
				"	vout.pos		= vin.pos;\n"
				"	vout.color		= vin.color;\n"
				"	vout.texCoord	= vin.texCoord;\n"
				"	return vout;\n"
				"}\n"
				"float4 PSMain( VS_OUT pin ) : SV_TARGET\n"
				"{\n"
				"	float4 sampleColor = diffuseMap.Sample( diffuseMapSampler, pin.texCoord ) * pin.color;\n"
				"	return sampleColor;\n"
				"}\n"
				;
			}
			constexpr const char *DisplayNameVS	= "Donya::DisplayVS";
			constexpr const char *DisplayNamePS	= "Donya::DisplayPS";
			constexpr const int   DisplaySlotSRV		= 0;
			constexpr const int   DisplaySlotSampler	= 0;
			constexpr const auto  DisplayInputElements()
			{
				return std::array<D3D11_INPUT_ELEMENT_DESC, 3>
				{
					D3D11_INPUT_ELEMENT_DESC{ "POSITION",	0, DXGI_FORMAT_R32G32B32_FLOAT,		0, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_VERTEX_DATA,	0 },
					D3D11_INPUT_ELEMENT_DESC{ "COLOR",		0, DXGI_FORMAT_R32G32B32A32_FLOAT,	0, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_VERTEX_DATA,	0 },
					D3D11_INPUT_ELEMENT_DESC{ "TEXCOORD",	0, DXGI_FORMAT_R32G32_FLOAT,		0, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_VERTEX_DATA,	0 },
				};
			}

			constexpr const char *SpriteCode()
			{
				return
				"Texture2D		diffuseMap			: register( t0 );\n"
				"SamplerState	diffuseMapSampler	: register( s0 );\n"
				"struct VS_IN\n"
				"{\n"
				"	float4		pos					: POSITION;\n"
				"	float2		texCoord			: TEXCOORD;\n"
				"	float4		color				: COLOR;\n"
				"	column_major\n"
				"	float4x4	NDCTransform		: NDC_TRANSFORM;\n"
				"	float4		texCoordTransform	: TEXCOORD_TRANSFORM;\n"
				"};\n"
				"struct VS_OUT\n"
				"{\n"
				"	float4 pos		: SV_POSITION;\n"
				"	float2 texCoord	: TEXCOORD;\n"
				"	float4 color	: COLOR;\n"
				"};\n"
				"VS_OUT VSMain( VS_IN vin )\n"
				"{\n"
				"	VS_OUT vout		= (VS_OUT)0;\n"
				"	vout.pos		= mul( vin.pos, vin.NDCTransform );\n"
				"	vout.texCoord	= vin.texCoord * vin.texCoordTransform.zw + vin.texCoordTransform.xy;\n"
				"	vout.color		= vin.color;\n"
				"	return vout;\n"
				"}\n"
				"float4 PSMain( VS_OUT pin ) : SV_TARGET\n"
				"{\n"
				"	float4 sampleColor = diffuseMap.Sample( diffuseMapSampler, pin.texCoord ) * pin.color;\n"
				"	return sampleColor;\n"
				"}\n"
				;
			}
			constexpr const char *SpriteNameVS	= "Donya::SpriteVS";
			constexpr const char *SpriteNamePS	= "Donya::SpritePS";
			constexpr const int   SpriteSlotSRV			= 0;
			constexpr const int   SpriteSlotSampler		= 0;
			constexpr const auto  SpriteInputElements()
			{
				return std::array<D3D11_INPUT_ELEMENT_DESC, 8>
				{
					D3D11_INPUT_ELEMENT_DESC{ "POSITION",			0, DXGI_FORMAT_R32G32B32_FLOAT,			0, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_VERTEX_DATA,	0 },
					D3D11_INPUT_ELEMENT_DESC{ "TEXCOORD",			0, DXGI_FORMAT_R32G32_FLOAT,			0, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_VERTEX_DATA,	0 },
					D3D11_INPUT_ELEMENT_DESC{ "COLOR",				0, DXGI_FORMAT_R32G32B32A32_FLOAT,		1, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_INSTANCE_DATA,	1 },
					D3D11_INPUT_ELEMENT_DESC{ "NDC_TRANSFORM",		0, DXGI_FORMAT_R32G32B32A32_FLOAT,		1, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_INSTANCE_DATA,	1 },
					D3D11_INPUT_ELEMENT_DESC{ "NDC_TRANSFORM",		1, DXGI_FORMAT_R32G32B32A32_FLOAT,		1, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_INSTANCE_DATA,	1 },
					D3D11_INPUT_ELEMENT_DESC{ "NDC_TRANSFORM",		2, DXGI_FORMAT_R32G32B32A32_FLOAT,		1, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_INSTANCE_DATA,	1 },
					D3D11_INPUT_ELEMENT_DESC{ "NDC_TRANSFORM",		3, DXGI_FORMAT_R32G32B32A32_FLOAT,		1, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_INSTANCE_DATA,	1 },
					D3D11_INPUT_ELEMENT_DESC{ "TEXCOORD_TRANSFORM",	0, DXGI_FORMAT_R32G32B32A32_FLOAT,		1, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_INSTANCE_DATA,	1 },
				};
			}

			constexpr const char *RectCode()
			{
				return
				"struct VS_OUT\n"
				"{\n"
				"	float4 pos					: SV_POSITION;\n"
				"	float4 color				: COLOR;\n"
				"};\n"
				"struct VS_IN\n"
				"{\n"
				"	float4		pos				: POSITION;\n"
				"	float4		color			: COLOR;\n"
				"	column_major\n"
				"	float4x4	NDCTransform	: NDC_TRANSFORM;\n"
				"};\n"
				"VS_OUT VSMain( VS_IN vin )\n"
				"{\n"
				"	VS_OUT vout = (VS_OUT)0;\n"
				"	vout.pos	= mul( vin.pos, vin.NDCTransform );\n"
				"	vout.color	= vin.color;\n"
				"	return vout;\n"
				"}\n"
				"float4 PSMain( VS_OUT pin ) : SV_TARGET\n"
				"{\n"
				"	return pin.color;\n"
				"}\n"
				;
			}
			constexpr const char *RectNameVS	= "Donya::RectVS";
			constexpr const char *RectNamePS	= "Donya::RectPS";
			constexpr const auto  RectInputElements()
			{
				return std::array<D3D11_INPUT_ELEMENT_DESC, 6>
				{
					D3D11_INPUT_ELEMENT_DESC{ "POSITION",			0, DXGI_FORMAT_R32G32B32_FLOAT,			0, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_VERTEX_DATA,	0 },
					D3D11_INPUT_ELEMENT_DESC{ "COLOR",				0, DXGI_FORMAT_R32G32B32A32_FLOAT,		1, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_INSTANCE_DATA,	1 },
					D3D11_INPUT_ELEMENT_DESC{ "NDC_TRANSFORM",		0, DXGI_FORMAT_R32G32B32A32_FLOAT,		1, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_INSTANCE_DATA,	1 },
					D3D11_INPUT_ELEMENT_DESC{ "NDC_TRANSFORM",		1, DXGI_FORMAT_R32G32B32A32_FLOAT,		1, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_INSTANCE_DATA,	1 },
					D3D11_INPUT_ELEMENT_DESC{ "NDC_TRANSFORM",		2, DXGI_FORMAT_R32G32B32A32_FLOAT,		1, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_INSTANCE_DATA,	1 },
					D3D11_INPUT_ELEMENT_DESC{ "NDC_TRANSFORM",		3, DXGI_FORMAT_R32G32B32A32_FLOAT,		1, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_INSTANCE_DATA,	1 },
				};
			}
		
			constexpr const char *CircleCode()
			{
				return
				"struct VS_OUT\n"
				"{\n"
				"	float4 pos					: SV_POSITION;\n"
				"	float4 color				: COLOR;\n"
				"};\n"
				"struct VS_IN\n"
				"{\n"
				"	float4		pos				: POSITION;\n"
				"	float4		color			: COLOR;\n"
				"	column_major\n"
				"	float4x4	NDCTransform	: NDC_TRANSFORM;\n"
				"};\n"
				"VS_OUT VSMain( VS_IN vin )\n"
				"{\n"
				"	VS_OUT vout = (VS_OUT)0;\n"
				"	vout.pos	= mul( vin.pos, vin.NDCTransform );\n"
				"	vout.color	= vin.color;\n"
				"	return vout;\n"
				"}\n"
				"float4 PSMain( VS_OUT pin ) : SV_TARGET\n"
				"{\n"
				"	return pin.color;\n"
				"}\n"
				;
			}
			constexpr const char *CircleNameVS	= "Donya::CircleVS";
			constexpr const char *CircleNamePS	= "Donya::CirclePS";
			constexpr const auto  CircleInputElements()
			{
				return std::array<D3D11_INPUT_ELEMENT_DESC, 6>
				{
					D3D11_INPUT_ELEMENT_DESC{ "POSITION",			0, DXGI_FORMAT_R32G32B32_FLOAT,			0, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_VERTEX_DATA,	0 },
					D3D11_INPUT_ELEMENT_DESC{ "COLOR",				0, DXGI_FORMAT_R32G32B32A32_FLOAT,		1, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_INSTANCE_DATA,	1 },
					D3D11_INPUT_ELEMENT_DESC{ "NDC_TRANSFORM",		0, DXGI_FORMAT_R32G32B32A32_FLOAT,		1, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_INSTANCE_DATA,	1 },
					D3D11_INPUT_ELEMENT_DESC{ "NDC_TRANSFORM",		1, DXGI_FORMAT_R32G32B32A32_FLOAT,		1, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_INSTANCE_DATA,	1 },
					D3D11_INPUT_ELEMENT_DESC{ "NDC_TRANSFORM",		2, DXGI_FORMAT_R32G32B32A32_FLOAT,		1, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_INSTANCE_DATA,	1 },
					D3D11_INPUT_ELEMENT_DESC{ "NDC_TRANSFORM",		3, DXGI_FORMAT_R32G32B32A32_FLOAT,		1, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_INSTANCE_DATA,	1 },
				};
			}
		}

		namespace Shared
		{
			D3D11_RASTERIZER_DESC		SpriteRasterizerDesc()
			{
				D3D11_RASTERIZER_DESC standard{};
				standard.FillMode				= D3D11_FILL_SOLID;
				standard.CullMode				= D3D11_CULL_NONE;
				standard.FrontCounterClockwise	= FALSE;
				standard.DepthBias				= 0;
				standard.DepthBiasClamp			= 0;
				standard.SlopeScaledDepthBias	= 0;
				standard.DepthClipEnable		= FALSE;
				standard.ScissorEnable			= FALSE;
				standard.MultisampleEnable		= FALSE;
				standard.AntialiasedLineEnable	= FALSE;
				return standard;
			}
			D3D11_DEPTH_STENCIL_DESC	SpriteDepthStencilDesc()
			{
				D3D11_DEPTH_STENCIL_DESC standard{};
				standard.DepthEnable		= TRUE;
				standard.DepthWriteMask		= D3D11_DEPTH_WRITE_MASK_ALL;
				standard.DepthFunc			= D3D11_COMPARISON_LESS_EQUAL;
				standard.StencilEnable		= FALSE;
				return standard;
			}
			D3D11_SAMPLER_DESC			SpriteSamplerDesc()
			{
				D3D11_SAMPLER_DESC standard{};
				/*
				standard.MipLODBias		= 0;
				standard.MaxAnisotropy	= 16;
				*/
				standard.Filter				= D3D11_FILTER_MIN_MAG_MIP_POINT;
				standard.AddressU			= D3D11_TEXTURE_ADDRESS_BORDER;
				standard.AddressV			= D3D11_TEXTURE_ADDRESS_BORDER;
				standard.AddressW			= D3D11_TEXTURE_ADDRESS_BORDER;
				standard.ComparisonFunc		= D3D11_COMPARISON_NEVER;
				standard.MinLOD				= 0;
				standard.MaxLOD				= D3D11_FLOAT32_MAX;

				DirectX::XMFLOAT4 borderColor{ 0.0f, 0.0f, 0.0f, 0.0f };
				memcpy
				(
					standard.BorderColor,
					&borderColor,
					sizeof( decltype( borderColor ) )
				);

				return standard;
			}

			namespace Instance
			{
				constexpr int DEFAULT_ID	= 0;
				static int idRasterizer		= DEFAULT_ID;
				static int idDepthStencil	= DEFAULT_ID;
				static int idSampler		= DEFAULT_ID;

				static bool wasInitialized	= false;

				struct Shader
				{
					Donya::VertexShader VS{};
					Donya::PixelShader  PS{};
				};

				struct Display
				{
					Shader shader{};
				};
				struct Batch
				{
					Shader shader{};
				};
				struct Rect
				{
					Shader shader{};
				};
				struct Circle
				{
					Shader shader{};
				};

				static Display	display{};
				static Batch	sprite{};
				static Rect		rect{};
				static Circle	circle{};
			}

			bool CreateStates ( ID3D11Device *pDevice )
			{
				auto AssertFailedCreation	= []( const std::wstring &stateName )
				{
					const std::wstring expression = L"Failed : Create a sprite's state of ";
					_ASSERT_EXPR( 0, ( expression + stateName + L"." ).c_str() );
				};
				auto AssertNotFound			= []( const std::wstring &stateName )
				{
					const std::wstring expression = L"Unexpected Error : We can't found a space of a sprite's state of ";
					_ASSERT_EXPR( 0, ( expression + stateName + L"." ).c_str() );
				};

				// Search the valid id.
				{
					// The internal object use minus value to identifier.

					if ( Instance::idDepthStencil	== Instance::DEFAULT_ID )
					{
						for ( int i = -1; -INT_MAX < i; --i )
						{
							if ( Donya::DepthStencil::IsAlreadyExists( i ) ) { continue; }
							// else
							Instance::idDepthStencil = i;
							break;
						}
						if ( Instance::idDepthStencil == Instance::DEFAULT_ID )
						{
							AssertNotFound( L"DepthStencil" );
							return false;
						}
						// else
					}
					if ( Instance::idRasterizer		== Instance::DEFAULT_ID )
					{
						for ( int i = -1; -INT_MAX < i; --i )
						{
							if ( Donya::Rasterizer::IsAlreadyExists( i ) ) { continue; }
							// else
							Instance::idRasterizer = i;
							break;
						}
						if ( Instance::idRasterizer == Instance::DEFAULT_ID )
						{
							AssertNotFound( L"Rasterizer" );
							return false;
						}
						// else
					}
					if ( Instance::idSampler		== Instance::DEFAULT_ID )
					{
						for ( int i = -1; -INT_MAX < i; --i )
						{
							if ( Donya::Sampler::IsAlreadyExists( i ) ) { continue; }
							// else
							Instance::idSampler = i;
							break;
						}
						if ( Instance::idSampler == Instance::DEFAULT_ID )
						{
							AssertNotFound( L"Sampler" );
							return false;
						}
						// else
					}
				}

				// Create the states.
				{
					const auto descDepthStencil	= SpriteDepthStencilDesc();
					const auto descRasterizer	= SpriteRasterizerDesc();
					const auto descSampler		= SpriteSamplerDesc();

					HRESULT hr		= S_OK;
					bool succeeded	= true;

					succeeded = Donya::DepthStencil::CreateState( Instance::idDepthStencil, descDepthStencil );
					if ( !succeeded )
					{
						AssertFailedCreation( L"DepthStencil" );
						return false;
					}
					// else
					succeeded = Donya::Rasterizer::CreateState( Instance::idRasterizer, descRasterizer );
					if ( !succeeded )
					{
						AssertFailedCreation( L"Rasterizer" );
						return false;
					}
					// else
					succeeded = Donya::Sampler::CreateState( Instance::idSampler, descSampler );
					if ( !succeeded )
					{
						AssertFailedCreation( L"Sampler" );
						return false;
					}
					// else
				}

				return true;
			}
			bool CreateShaders( ID3D11Device *pDevice )
			{
				namespace	Source	= EmbeddedSourceCode;
				using		IE_DESC	= D3D11_INPUT_ELEMENT_DESC;

				auto AssertFailedCreation	= []( const std::wstring &shaderName )
				{
					const std::wstring expression = L"Failed : Create a sprite's shader of ";
					_ASSERT_EXPR( 0, ( expression + shaderName + L"." ).c_str() );
				};

				const auto displayIEDescs	= Source::DisplayInputElements();
				const auto spriteIEDescs	= Source::SpriteInputElements();
				const auto rectIEDescs		= Source::RectInputElements();
				const auto circleIEDescs	= Source::CircleInputElements();

				bool succeeded = true;

				// Display.
				{
					succeeded = Instance::display.shader.VS.CreateByEmbededSourceCode
					(
						Source::DisplayNameVS, Source::DisplayCode(), Source::EntryPointVS,
						std::vector<IE_DESC>( displayIEDescs.begin(), displayIEDescs.end() ) // This method requires the descs as std::vector.
					);
					if ( !succeeded )
					{
						AssertFailedCreation( L"Display_VS" );
						return false;
					}
					// else

					succeeded = Instance::display.shader.PS.CreateByEmbededSourceCode
					(
						Source::DisplayNamePS, Source::DisplayCode(), Source::EntryPointPS
					);
					if ( !succeeded )
					{
						AssertFailedCreation( L"Display_PS" );
						return false;
					}
					// else
				}
				// Sprite.
				{
					succeeded = Instance::sprite.shader.VS.CreateByEmbededSourceCode
					(
						Source::SpriteNameVS, Source::SpriteCode(), Source::EntryPointVS,
						std::vector<IE_DESC>( spriteIEDescs.begin(), spriteIEDescs.end() ) // This method requires the descs as std::vector.
					);
					if ( !succeeded )
					{
						AssertFailedCreation( L"Sprite_VS" );
						return false;
					}
					// else

					succeeded = Instance::sprite.shader.PS.CreateByEmbededSourceCode
					(
						Source::SpriteNamePS, Source::SpriteCode(), Source::EntryPointPS
					);
					if ( !succeeded )
					{
						AssertFailedCreation( L"Sprite_PS" );
						return false;
					}
					// else
				}
				// Rect.
				{
					succeeded = Instance::rect.shader.VS.CreateByEmbededSourceCode
					(
						Source::RectNameVS, Source::RectCode(), Source::EntryPointVS,
						std::vector<IE_DESC>( rectIEDescs.begin(), rectIEDescs.end() ) // This method requires the descs as std::vector.
					);
					if ( !succeeded )
					{
						AssertFailedCreation( L"Rect_VS" );
						return false;
					}
					// else

					succeeded = Instance::rect.shader.PS.CreateByEmbededSourceCode
					(
						Source::RectNamePS, Source::RectCode(), Source::EntryPointPS
					);
					if ( !succeeded )
					{
						AssertFailedCreation( L"Rect_PS" );
						return false;
					}
					// else
				}
				// Circle.
				{
					succeeded = Instance::circle.shader.VS.CreateByEmbededSourceCode
					(
						Source::CircleNameVS, Source::CircleCode(), Source::EntryPointVS,
						std::vector<IE_DESC>( circleIEDescs.begin(), circleIEDescs.end() ) // This method requires the descs as std::vector.
					);
					if ( !succeeded )
					{
						AssertFailedCreation( L"Circle_VS" );
						return false;
					}
					// else

					succeeded = Instance::circle.shader.PS.CreateByEmbededSourceCode
					(
						Source::CircleNamePS, Source::CircleCode(), Source::EntryPointPS
					);
					if ( !succeeded )
					{
						AssertFailedCreation( L"Circle_PS" );
						return false;
					}
					// else
				}

				return true;
			}
			bool Init( ID3D11Device *pDevice = nullptr )
			{
				if ( !pDevice ) { pDevice = Donya::GetDevice(); }

				bool succeeded = true;

				succeeded = CreateStates( pDevice );
				if ( !succeeded )
				{
					_ASSERT_EXPR( 0, L"Failed : Create a sprite's states." );
					return false;
				}
				// else
				
				succeeded = CreateShaders( pDevice );
				if ( !succeeded )
				{
					_ASSERT_EXPR( 0, L"Failed : Create a sprite's shaders." );
					return false;
				}
				// else

				Instance::wasInitialized = true;
				return true;
			}
			bool AlreadyInitialized()
			{
				return Instance::wasInitialized;
			}

			void ActivateStates			( ID3D11DeviceContext *pImmidiateContext = nullptr )
			{
				if ( !pImmidiateContext ) { pImmidiateContext = Donya::GetImmediateContext(); }

				Donya::DepthStencil::Activate( Instance::idDepthStencil, pImmidiateContext );
				Donya::Rasterizer::Activate( Instance::idRasterizer, pImmidiateContext );
			}
			void DeactivateStates		( ID3D11DeviceContext *pImmidiateContext = nullptr )
			{
				if ( !pImmidiateContext ) { pImmidiateContext = Donya::GetImmediateContext(); }

				Donya::DepthStencil::Deactivate( pImmidiateContext );
				Donya::Rasterizer::Deactivate( pImmidiateContext );
			}

			void ActivateDisplayShaders( ID3D11DeviceContext *pImmidiateContext = nullptr )
			{
				if ( !pImmidiateContext ) { pImmidiateContext = Donya::GetImmediateContext(); }

				Instance::display.shader.VS.Activate( pImmidiateContext );
				Instance::display.shader.PS.Activate( pImmidiateContext );

				Donya::Sampler::Activate
				(
					Instance::idSampler,
					EmbeddedSourceCode::DisplaySlotSampler,
					/* setVS = */ true, /* setPS = */true,
					pImmidiateContext
				);
			}
			void DeactivateDisplayShaders( ID3D11DeviceContext *pImmidiateContext = nullptr )
			{
				if ( !pImmidiateContext ) { pImmidiateContext = Donya::GetImmediateContext(); }

				Instance::display.shader.VS.Deactivate( pImmidiateContext );
				Instance::display.shader.PS.Deactivate( pImmidiateContext );

				Donya::Sampler::Deactivate( pImmidiateContext );
			}

			void ActivateSpriteShaders	( ID3D11DeviceContext *pImmidiateContext = nullptr )
			{
				if ( !pImmidiateContext ) { pImmidiateContext = Donya::GetImmediateContext(); }

				Instance::sprite.shader.VS.Activate( pImmidiateContext );
				Instance::sprite.shader.PS.Activate( pImmidiateContext );

				Donya::Sampler::Activate
				(
					Instance::idSampler,
					EmbeddedSourceCode::SpriteSlotSampler,
					/* setVS = */ true, /* setPS = */true,
					pImmidiateContext
				);
			}
			void DeactivateSpriteShaders( ID3D11DeviceContext *pImmidiateContext = nullptr )
			{
				if ( !pImmidiateContext ) { pImmidiateContext = Donya::GetImmediateContext(); }

				Instance::sprite.shader.VS.Deactivate( pImmidiateContext );
				Instance::sprite.shader.PS.Deactivate( pImmidiateContext );

				Donya::Sampler::Deactivate ( pImmidiateContext );
			}

			void ActivateRectShaders	( ID3D11DeviceContext *pImmidiateContext = nullptr )
			{
				if ( !pImmidiateContext ) { pImmidiateContext = Donya::GetImmediateContext(); }

				Instance::rect.shader.VS.Activate( pImmidiateContext );
				Instance::rect.shader.PS.Activate( pImmidiateContext );
			}
			void DeactivateRectShaders	( ID3D11DeviceContext *pImmidiateContext = nullptr )
			{
				if ( !pImmidiateContext ) { pImmidiateContext = Donya::GetImmediateContext(); }

				Instance::rect.shader.VS.Deactivate( pImmidiateContext );
				Instance::rect.shader.PS.Deactivate( pImmidiateContext );
			}
			
			void ActivateCircleShaders	( ID3D11DeviceContext *pImmidiateContext = nullptr )
			{
				if ( !pImmidiateContext ) { pImmidiateContext = Donya::GetImmediateContext(); }

				Instance::circle.shader.VS.Activate( pImmidiateContext );
				Instance::circle.shader.PS.Activate( pImmidiateContext );
			}
			void DeactivateCircleShaders( ID3D11DeviceContext *pImmidiateContext = nullptr )
			{
				if ( !pImmidiateContext ) { pImmidiateContext = Donya::GetImmediateContext(); }

				Instance::circle.shader.VS.Deactivate( pImmidiateContext );
				Instance::circle.shader.PS.Deactivate( pImmidiateContext );
			}
		}

		static float drawDepth = 1.0f; // Z position of sprite.
		void  SetDrawDepth( float depth )
		{
			depth = std::max( 0.0f, std::min( 1.0f, depth ) );
			drawDepth = depth;
		}
		float GetDrawDepth()
		{
			return drawDepth;
		}

		void MakeMatrixNDCTransform( XMFLOAT4X4 *pOutput4x4, float scrX, float scrY, float scrW, float scrH, float degree, float rotX, float rotY, float depth = 0.0f )
		{
			// NDC Transform Matrix
			// _11( w * dw * cos ) _12( w * dh * -sin ) _13(  0.0f  ) _14( w * ( -cx * cos + -cy *-sin + cx + dx ) - 1.0f )
			// _21( h * dw * sin ) _22( h * dh *  cos ) _23(  0.0f  ) _24( h * ( -cx * sin + -cy * cos + cy + dy ) + 1.0f )
			// _31(     0.0f     ) _32(     0.0f      ) _33(  1.0f  ) _34(                      depth                     )
			// _41(     0.0f     ) _42(     0.0f      ) _43(  0.0f  ) _44(                      1.0f                      )

			const float radian	= ToRadian( degree );
			const float cos		= cosf( radian );
			const float sin		= sinf( radian );
			const float w		=  2.0f / ::Donya::Private::RegisteredScreenWidthF();
			const float h		= -2.0f / ::Donya::Private::RegisteredScreenHeightF();

			pOutput4x4->_11 = w * scrW * cos;
			pOutput4x4->_21 = h * scrW * sin;
			pOutput4x4->_31 = 0.0f;
			pOutput4x4->_41 = 0.0f;

			pOutput4x4->_12 = w * scrH * -sin;
			pOutput4x4->_22 = h * scrH * cos;
			pOutput4x4->_32 = 0.0f;
			pOutput4x4->_42 = 0.0f;

			pOutput4x4->_13 = 0.0f;
			pOutput4x4->_23 = 0.0f;
			pOutput4x4->_33 = 0.0f; // 1.0f is OK if already the Z is 0.0f when create vertex-buffer.
			pOutput4x4->_43 = 0.0f;

			pOutput4x4->_14 = w * ( ( -rotX * cos ) + ( -rotY * -sin ) + rotX + scrX ) - 1.0f;
			pOutput4x4->_24 = h * ( ( -rotX * sin ) + ( -rotY *  cos ) + rotY + scrY ) + 1.0f;
			pOutput4x4->_34 = depth;
			pOutput4x4->_44 = 1.0f;
		}

	#pragma region Display
	
		bool Display::Init( ID3D11Device *pDevice )
		{
			if ( wasInitialized ) { return true; }
			// else

			HRESULT hr = S_OK;
			if ( !pDevice ) { pDevice = ::Donya::GetDevice(); }

			constexpr XMFLOAT4 DEFAULT_COLOR{ 1.0f, 1.0f, 1.0f, 1.0f };
			constexpr std::array<Display::Vertex, 4> VERTICES
			{
				Display::Vertex{ XMFLOAT3{ 0.0f, 1.0f, 0.0f }, DEFAULT_COLOR, XMFLOAT2{ 0.0f, 1.0f } },
				Display::Vertex{ XMFLOAT3{ 1.0f, 1.0f, 0.0f }, DEFAULT_COLOR, XMFLOAT2{ 1.0f, 1.0f } },
				Display::Vertex{ XMFLOAT3{ 0.0f, 0.0f, 0.0f }, DEFAULT_COLOR, XMFLOAT2{ 0.0f, 0.0f } },
				Display::Vertex{ XMFLOAT3{ 1.0f, 0.0f, 0.0f }, DEFAULT_COLOR, XMFLOAT2{ 1.0f, 0.0f } }
			};

			hr = Donya::CreateVertexBuffer<Display::Vertex>
			(
				pDevice, VERTICES,
				D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE,
				pVertexBuffer.GetAddressOf()
			);
			if ( FAILED( hr ) )
			{
				_ASSERT_EXPR( SUCCEEDED( hr ), L"Failed : Create vertex-buffer." );
				return false;
			}
			// else

			wasInitialized = true;
			return true;
		}
		void Display::Uninit()
		{
			// No op.
		}

	#pragma region Normal
		bool Display::Draw( ID3D11ShaderResourceView *pSRV, const Donya::Vector2 &ssPos, float degree, float alpha, const Donya::Vector2 &origin ) const
		{
			return DrawExt
			(
				pSRV,
				ssPos,
				DEFAULT.scale, degree,
				DEFAULT.MakeColor( alpha ), origin
			);
		}
		bool Display::Draw( const Donya::Surface &surface,  const Donya::Vector2 &ssPos, float degree, float alpha, const Donya::Vector2 &origin ) const
		{
			return DrawExt
			(
				surface,
				ssPos,
				DEFAULT.scale, degree,
				DEFAULT.MakeColor( alpha ), origin
			);
		}
		bool Display::DrawExt( ID3D11ShaderResourceView *pSRV, const Donya::Vector2 &ssPos, const Donya::Vector2 &scale, float degree, const Donya::Vector4 &color, const Donya::Vector2 &origin ) const
		{
			const Donya::Vector2 wholeSize = FetchTextureSizeF( pSRV );
			return DrawGeneralExt
			(
				pSRV,
				ssPos, wholeSize,
				Donya::Vector2::Zero(), wholeSize,
				scale, degree,
				color, origin
			);
		}
		bool Display::DrawExt( const Donya::Surface &surface,  const Donya::Vector2 &ssPos, const Donya::Vector2 &scale, float degree, const Donya::Vector4 &color, const Donya::Vector2 &origin ) const
		{
			const Donya::Vector2 wholeSize = surface.GetSurfaceSizeF();
			ID3D11ShaderResourceView *pSRV = surface.GetShaderResourceView().Get();
			return DrawGeneralExt
			(
				pSRV,
				ssPos, wholeSize,
				Donya::Vector2::Zero(), wholeSize,
				scale, degree,
				color, origin
			);
		}
	#pragma endregion

	#pragma region Stretched
		bool Display::DrawStretched( ID3D11ShaderResourceView *pSRV, const Donya::Vector2 &ssPos, const Donya::Vector2 &ssSize, float degree, float alpha, const Donya::Vector2 &origin ) const
		{
			return DrawStretchedExt
			(
				pSRV,
				ssPos, ssSize,
				DEFAULT.scale, degree,
				DEFAULT.MakeColor( alpha ), origin
			);
		}
		bool Display::DrawStretched( const Donya::Surface &surface,  const Donya::Vector2 &ssPos, const Donya::Vector2 &ssSize, float degree, float alpha, const Donya::Vector2 &origin ) const
		{
			return DrawStretchedExt
			(
				surface,
				ssPos, ssSize,
				DEFAULT.scale, degree,
				DEFAULT.MakeColor( alpha ), origin
			);
		}
		bool Display::DrawStretchedExt( ID3D11ShaderResourceView *pSRV, const Donya::Vector2 &ssPos, const Donya::Vector2 &ssSize, const Donya::Vector2 &scale, float degree, const Donya::Vector4 &color, const Donya::Vector2 &origin ) const
		{
			return DrawGeneralExt
			(
				pSRV,
				ssPos, ssSize,
				Donya::Vector2::Zero(), ssSize,
				scale, degree,
				color, origin
			);
		}
		bool Display::DrawStretchedExt( const Donya::Surface &surface,  const Donya::Vector2 &ssPos, const Donya::Vector2 &ssSize, const Donya::Vector2 &scale, float degree, const Donya::Vector4 &color, const Donya::Vector2 &origin ) const
		{
			ID3D11ShaderResourceView *pSRV = surface.GetShaderResourceView().Get();
			return DrawGeneralExt
			(
				pSRV,
				ssPos, ssSize,
				Donya::Vector2::Zero(), ssSize,
				scale, degree,
				color, origin
			);
		}
	#pragma endregion

	#pragma region Part
		bool Display::DrawPart( ID3D11ShaderResourceView *pSRV, const Donya::Vector2 &ssPos, const Donya::Vector2 &texPos, const Donya::Vector2 &texSize, float degree, float alpha, const Donya::Vector2 &origin ) const
		{
			return DrawPartExt
			(
				pSRV,
				ssPos,
				texPos, texSize,
				DEFAULT.scale, degree,
				DEFAULT.MakeColor( alpha ), origin
			);
		}
		bool Display::DrawPart( const Donya::Surface &surface,  const Donya::Vector2 &ssPos, const Donya::Vector2 &texPos, const Donya::Vector2 &texSize, float degree, float alpha, const Donya::Vector2 &origin ) const
		{
			return DrawPartExt
			(
				surface,
				ssPos,
				texPos, texSize,
				DEFAULT.scale, degree,
				DEFAULT.MakeColor( alpha ), origin
			);
		}
		bool Display::DrawPartExt( ID3D11ShaderResourceView *pSRV, const Donya::Vector2 &ssPos, const Donya::Vector2 &texPos, const Donya::Vector2 &texSize, const Donya::Vector2 &scale, float degree, const Donya::Vector4 &color, const Donya::Vector2 &origin ) const
		{
			return DrawGeneralExt
			(
				pSRV,
				ssPos, FetchTextureSizeF( pSRV ),
				texPos, texSize,
				scale, degree,
				color, origin
			);
		}
		bool Display::DrawPartExt( const Donya::Surface &surface,  const Donya::Vector2 &ssPos, const Donya::Vector2 &texPos, const Donya::Vector2 &texSize, const Donya::Vector2 &scale, float degree, const Donya::Vector4 &color, const Donya::Vector2 &origin ) const
		{
			ID3D11ShaderResourceView *pSRV = surface.GetShaderResourceView().Get();
			return DrawGeneralExt
			(
				pSRV,
				ssPos, surface.GetSurfaceSizeF(),
				texPos, texSize,
				scale, degree,
				color, origin
			);
		}
	#pragma endregion

	#pragma region Tiled

	#pragma endregion

	#pragma region General
		bool Display::DrawGeneral( ID3D11ShaderResourceView *pSRV, const Donya::Vector2 &ssPos, const Donya::Vector2 &ssSize, const Donya::Vector2 &texPos, const Donya::Vector2 &texSize, float degree, float alpha, const Donya::Vector2 &origin ) const
		{
			return DrawGeneralExt
			(
				pSRV,
				ssPos, ssSize,
				texPos, texSize,
				DEFAULT.scale, degree,
				Donya::Vector4{ DEFAULT.color, alpha }, origin
			);
		}
		bool Display::DrawGeneral( const Donya::Surface &surface,  const Donya::Vector2 &ssPos, const Donya::Vector2 &ssSize, const Donya::Vector2 &texPos, const Donya::Vector2 &texSize, float degree, float alpha, const Donya::Vector2 &origin ) const
		{
			return DrawGeneralExt
			(
				surface.GetShaderResourceView().Get(),
				ssPos, ssSize,
				texPos, texSize,
				DEFAULT.scale, degree,
				DEFAULT.MakeColor( alpha ), origin
			);
		}
		bool Display::DrawGeneralExt( ID3D11ShaderResourceView *pSRV, const Donya::Vector2 &ssPos, const Donya::Vector2 &ssSize, const Donya::Vector2 &texPos, const Donya::Vector2 &texSize, const Donya::Vector2 &scale, float degree, const Donya::Vector4 &color, const Donya::Vector2 &origin ) const
		{
			if ( !wasInitialized )
			{
				_ASSERT_EXPR( 0, L"Error : The display does not initialized!" );
				return false;
			}
			// else

			ID3D11DeviceContext *pImmediateContext = Donya::GetImmediateContext();

			const Donya::Vector2 scrSize{ ssSize.x * scale.x, ssSize.y * scale.y };
			Donya::Vector2 scrPos = ssPos;

			// Adjust the origin of rotation.
			scrPos.x -= scrSize.x * origin.x;
			scrPos.y -= scrSize.y * origin.y;

			if ( Donya::ScreenShake::GetEnableState() )
			{
				scrPos.x -= Donya::ScreenShake::GetX();
				scrPos.y -= Donya::ScreenShake::GetY();
			}

			const auto NDCVertices = MakeNDCVertices
			(
				FetchTextureSizeF( pSRV ),
				scrPos, scrSize,
				texPos, texSize,
				degree, color, origin
			);

			if ( !MapVertices( NDCVertices, pImmediateContext ) )
			{
				return false;
			}
			// else
			
			// PreProcess.
			{
				const UINT stride = sizeof( Display::Vertex );
				const UINT offset = 0;
				pImmediateContext->IASetVertexBuffers( 0, 1, pVertexBuffer.GetAddressOf(), &stride, &offset );
				pImmediateContext->IASetPrimitiveTopology( D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP );

				pImmediateContext->PSSetShaderResources( EmbeddedSourceCode::DisplaySlotSRV, 1, &pSRV );

				Shared::ActivateStates();
				Shared::ActivateDisplayShaders();
			}

			pImmediateContext->Draw( NDCVertices.size(), 0 );

			// PostProcess.
			{
				ID3D11ShaderResourceView *NullSRV = nullptr;
				pImmediateContext->PSSetShaderResources( EmbeddedSourceCode::DisplaySlotSRV, 1, &NullSRV );

				Shared::DeactivateStates();
				Shared::DeactivateDisplayShaders();
			}

			return true;
		}
		bool Display::DrawGeneralExt( const Donya::Surface &surface,  const Donya::Vector2 &ssPos, const Donya::Vector2 &ssSize, const Donya::Vector2 &texPos, const Donya::Vector2 &texSize, const Donya::Vector2 &scale, float degree, const Donya::Vector4 &color, const Donya::Vector2 &origin ) const
		{
			return DrawGeneralExt
			(
				surface.GetShaderResourceView().Get(),
				ssPos, ssSize,
				texPos, texSize,
				scale, degree,
				color, origin
			);
		}
	#pragma endregion

		Donya::Int2		Display::FetchTextureSize ( ID3D11ShaderResourceView *pSRV ) const
		{
			D3D11_TEXTURE2D_DESC desc{};
			ID3D11Texture2D *pTexture = nullptr;

			pSRV->GetResource( reinterpret_cast<ID3D11Resource **>( &pTexture ) );
			pTexture->GetDesc( &desc );

			Donya::Int2 wholeSize
			{
				scast<int>( desc.Width  ),
				scast<int>( desc.Height )
			};

			pTexture->Release();
			return wholeSize;
		}
		Donya::Vector2	Display::FetchTextureSizeF( ID3D11ShaderResourceView *pSRV ) const
		{
			return FetchTextureSize( pSRV ).Float();
		}

		std::array<Display::Vertex, 4U> Display::MakeNDCVertices( const Donya::Vector2 &originalTexSize, const Donya::Vector2 &ssPos, const Donya::Vector2 &ssSize, const Donya::Vector2 &texPos, const Donya::Vector2 &texSize, float degree, const Donya::Vector4 &color, const Donya::Vector2 &origin ) const
		{
			constexpr size_t VERTICES_COUNT = 4U;

			const float drawDepth = GetDrawDepth();
			std::array<Display::Vertex, VERTICES_COUNT> vertices
			{
				/* LT */ Display::Vertex{ XMFLOAT3{ ssPos.x,			ssPos.y,			drawDepth }, color, DirectX::XMFLOAT2{ texPos.x,				texPos.y				} },
				/* RT */ Display::Vertex{ XMFLOAT3{ ssPos.x + ssSize.x,	ssPos.y,			drawDepth }, color, DirectX::XMFLOAT2{ texPos.x + texSize.x,	texPos.y				} },
				/* LB */ Display::Vertex{ XMFLOAT3{ ssPos.x,			ssPos.y + ssSize.y,	drawDepth }, color, DirectX::XMFLOAT2{ texPos.x,				texPos.y + texSize.y	} },
				/* RB */ Display::Vertex{ XMFLOAT3{ ssPos.x + ssSize.x,	ssPos.y + ssSize.y,	drawDepth }, color, DirectX::XMFLOAT2{ texPos.x + texSize.x,	texPos.y + texSize.y	} },
			};

			// Rotate vertices at origin.
			{
				auto RotateZAxis = []( XMFLOAT3 *pVector, float cos, float sin )
				{
					const Donya::Vector2 relative{ pVector->x, pVector->y };

					pVector->x = ( relative.x * cos ) - ( relative.y * sin );
					pVector->y = ( relative.y * cos ) + ( relative.x * sin );
				};

				// Translate sprite's centre to origin ( rotate centre )
				const Donya::Vector2 distFromOrigin
				{
					ssPos.x + ( ssSize.x * origin.x ),
					ssPos.y + ( ssSize.y * origin.y ),
				};

				for ( size_t i = 0; i < VERTICES_COUNT; ++i )
				{
					vertices[i].pos.x -= distFromOrigin.x;
					vertices[i].pos.y -= distFromOrigin.y;
				}

				const float radian = ToRadian( degree );
				const float cos = cosf( radian );
				const float sin = sinf( radian );
				for ( size_t i = 0; i < VERTICES_COUNT; ++i )
				{
					RotateZAxis( &vertices[i].pos, cos, sin );
				}

				for ( size_t i = 0; i < VERTICES_COUNT; ++i )
				{
					vertices[i].pos.x += distFromOrigin.x;
					vertices[i].pos.y += distFromOrigin.y;
				}
			}

			// Convert to NDC space
			{
				/*
				2/xMax-xMin     0               0            -((xMax+xMin)/(xMax-xMin))
				0               2/yMax-yMin     0            -((yMax+yMin)/(yMax-yMin))
				0               0               1/zMax-zMin  -(    zMin   /(zMax-zMin))
				0               0               0            1
				*/
				const float xMax = Donya::Private::RegisteredScreenWidthF(),  xMin = -1.0f;
				const float yMax = Donya::Private::RegisteredScreenHeightF(), yMin = -1.0f;

				for ( size_t i = 0; i < VERTICES_COUNT; ++i )
				{
					vertices[i].pos.x =        ( ( 2.0f * vertices[i].pos.x ) / ( xMax ) ) - 1.0f;
					vertices[i].pos.y = 1.0f - ( ( 2.0f * vertices[i].pos.y ) / ( yMax ) );

					vertices[i].texCoord.x /= originalTexSize.x;
					vertices[i].texCoord.y /= originalTexSize.y;
				}
			}

			return vertices;
		}

		bool Display::MapVertices( const std::array<Display::Vertex, 4U> &NDCVertices, ID3D11DeviceContext *pImmediateContext ) const
		{
			if ( !pImmediateContext ) { pImmediateContext = Donya::GetImmediateContext(); }

			D3D11_MAPPED_SUBRESOURCE msr{};

			HRESULT hr = pImmediateContext->Map( pVertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &msr );
			if ( FAILED( hr ) )
			{
				_ASSERT_EXPR( 0, L"Failed : Mapping a vertices at Display." );
				return false;
			}
			// else

			memcpy_s( msr.pData, sizeof( Display::Vertex ) * NDCVertices.size(), &NDCVertices, msr.RowPitch );

			pImmediateContext->Unmap( pVertexBuffer.Get(), 0 );

			return true;
		}

	#pragma endregion

	#pragma region Batch

		Batch::Batch( const std::wstring filename, size_t maxInstancesCount ) :
			MAX_INSTANCES( maxInstancesCount ), reserveCount( NULL ), instances(),
			texture2DDesc(), pInstanceBuffer(), pVertexBuffer(), pShaderResourceView()
		{
			HRESULT hr = S_OK;
			ID3D11Device *pDevice = ::Donya::GetDevice();

			constexpr std::array<Batch::Vertex, 4> VERTICES =
			{
				Batch::Vertex{ XMFLOAT3( 0.0f, 1.0f, 0.0f ), XMFLOAT2{ 0.0f, 1.0f } },
				Batch::Vertex{ XMFLOAT3( 1.0f, 1.0f, 0.0f ), XMFLOAT2{ 1.0f, 1.0f } },
				Batch::Vertex{ XMFLOAT3( 0.0f, 0.0f, 0.0f ), XMFLOAT2{ 0.0f, 0.0f } },
				Batch::Vertex{ XMFLOAT3( 1.0f, 0.0f, 0.0f ), XMFLOAT2{ 1.0f, 0.0f } }
			};

			// Create VertexBuffer
			{
				hr = ::Donya::CreateVertexBuffer<Batch::Vertex>
				(
					pDevice, VERTICES,
					D3D11_USAGE_IMMUTABLE, 0,
					pVertexBuffer.GetAddressOf()
				);
				_ASSERT_EXPR( SUCCEEDED( hr ), L"Failed : Create vertex-buffer()" );
			}
			// Create Instances and InstanceBuffer
			{
				instances.resize( MAX_INSTANCES );

				hr = ::Donya::CreateVertexBuffer<Batch::Instance>
				(
					pDevice, instances,
					D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE,
					pInstanceBuffer.GetAddressOf()
				);
				_ASSERT_EXPR( SUCCEEDED( hr ), L"Failed : Create instance-buffer()" );
			}
			
			// Read Texture
			{
				bool succeeded = Resource::CreateTexture2DFromFile
				(
					pDevice,
					filename,
					pShaderResourceView.GetAddressOf(),
					&texture2DDesc
				);
				if ( !succeeded )
				{
					_ASSERT_EXPR( 0, L"Failed : Create a texture of sprite." );
				}
			}
		}
		Batch::~Batch()
		{
			instances.clear();
			instances.shrink_to_fit();
		}

		int				Batch::GetTextureWidth() const
		{
			return texture2DDesc.Width;
		}
		int				Batch::GetTextureHeight() const
		{
			return texture2DDesc.Height;
		}
		float			Batch::GetTextureWidthF() const
		{
			return scast<float>( GetTextureWidth() );
		}
		float			Batch::GetTextureHeightF() const
		{
			return scast<float>( GetTextureHeight() );
		}
		Donya::Int2		Batch::GetTextureSize() const
		{
			return Donya::Int2
			{
				GetTextureWidth(),
				GetTextureHeight()
			};
		}
		Donya::Vector2	Batch::GetTextureSizeF() const
		{
			return GetTextureSize().Float();
		}
		void			Batch::GetTextureSize( int *width, int *height ) const
		{
			if ( width  ) { *width  = GetTextureWidth();  }
			if ( height ) { *height = GetTextureHeight(); }
		}
		void			Batch::GetTextureSize( float *width, float *height ) const
		{
			if ( width  ) { *width  = GetTextureWidthF();  }
			if ( height ) { *height = GetTextureHeightF(); }
		}

		XMFLOAT2 Batch::MakeSpriteCenter( Origin center, float scaleX, float scaleY ) const
		{
			XMFLOAT2 size{};
			GetTextureSize( &size.x, &size.y );
			return MakeSpecifiedCenter( center, size.x, size.y, scaleX, scaleY );
		}
		XMFLOAT2 Batch::MakeSpecifiedCenter( Origin center, float texW, float texH, float scaleX, float scaleY ) const
		{
			XMFLOAT2 rv{};
			texW *= scaleX;
			texH *= scaleY;

			if ( center & X_LEFT )
			{
				rv.x = 0;
			}
			else
			if ( center & X_RIGHT )
			{
				rv.x = texW;
			}
			else // This contain not specified case.
			{
				rv.x = texW * 0.5f;
			}


			if ( center & Y_TOP )
			{
				rv.y = 0;
			}
			else
			if ( center & Y_BOTTOM )
			{
				rv.y = texH;
			}
			else // This contain not specified case.
			{
				rv.y = texH * 0.5f;
			}


			return rv;
		}

	#pragma region Normal
		bool Batch::Reserve( float scrX, float scrY, float degree, XMFLOAT2 center, float alpha )
		{
			return ReserveExt
			(
				scrX, scrY,
				1.0f, 1.0f,
				degree, center,
				alpha, 1.0f, 1.0f, 1.0f
			);
		}
		bool Batch::Reserve( float scrX, float scrY, float degree, Origin center, float alpha )
		{
			return Reserve
			(
				scrX, scrY,
				degree,
				MakeSpriteCenter( center, 1.0f, 1.0f ),
				alpha
			);
		}
		bool Batch::Reserve( float scrX, float scrY, float degree, float alpha )
		{
			return Reserve
			(
				scrX, scrY,
				degree,
				X_MIDDLE | Y_MIDDLE,
				alpha
			);
		}
		bool Batch::ReserveExt( float scrX, float scrY, float scaleX, float scaleY, float degree, XMFLOAT2 center, float alpha, float R, float G, float B )
		{
			XMFLOAT2 size{};
			GetTextureSize( &size.x, &size.y );

			return ReserveGeneralExt
			(
				scrX, scrY,
				size.x, size.y,
				0, 0,
				size.x, size.y,
				scaleX, scaleY,
				degree, center,
				alpha, R, G, B
			);
		}
		bool Batch::ReserveExt( float scrX, float scrY, float scaleX, float scaleY, float degree, Origin center, float alpha, float R, float G, float B )
		{
			return ReserveExt
			(
				scrX, scrY,
				scaleX, scaleY,
				degree,
				MakeSpriteCenter( center, 1.0f, 1.0f ),
				alpha, R, G, B
			);
		}
		bool Batch::ReserveExt( float scrX, float scrY, float scaleX, float scaleY, float degree, float alpha, float R, float G, float B )
		{
			return ReserveExt
			(
				scrX, scrY,
				scaleX, scaleY,
				degree,
				X_MIDDLE | Y_MIDDLE,
				alpha, R, G, B
			);
		}
	#pragma endregion

	#pragma region Stretched
		bool Batch::ReserveStretched( float scrX, float scrY, float scrW, float scrH, float degree, XMFLOAT2 center, float alpha )
		{
			return ReserveStretchedExt
			(
				scrX, scrY, scrW, scrH,
				1.0f, 1.0f,
				degree,
				center,
				alpha, 1.0f, 1.0f, 1.0f
			);
		}
		bool Batch::ReserveStretched( float scrX, float scrY, float scrW, float scrH, float degree, Origin center, float alpha )
		{
			return ReserveStretched
			(
				scrX, scrY, scrW, scrH,
				degree,
				MakeSpriteCenter( center, 1.0f, 1.0f ),
				alpha
			);
		}
		bool Batch::ReserveStretched( float scrX, float scrY, float scrW, float scrH, float degree, float alpha )
		{
			return ReserveStretched
			(
				scrX, scrY, scrW, scrH,
				degree,
				X_MIDDLE | Y_MIDDLE,
				alpha
			);
		}
		bool Batch::ReserveStretchedExt( float scrX, float scrY, float scrW, float scrH, float scaleX, float scaleY, float degree, XMFLOAT2 center, float alpha, float R, float G, float B )
		{
			XMFLOAT2 size{};
			GetTextureSize( &size.x, &size.y );

			return ReserveGeneralExt
			(
				scrX, scrY,
				scrW, scrH,
				0, 0,
				size.x, size.y,
				scaleX, scaleY,
				degree, center,
				alpha, R, G, B
			);
		}
		bool Batch::ReserveStretchedExt( float scrX, float scrY, float scrW, float scrH, float scaleX, float scaleY, float degree, Origin center, float alpha, float R, float G, float B )
		{
			return ReserveStretchedExt
			(
				scrX, scrY, scrW, scrH,
				scaleX, scaleY,
				degree,
				MakeSpriteCenter( center, scaleX, scaleY ),
				alpha, R, G, B
			);
		}
		bool Batch::ReserveStretchedExt( float scrX, float scrY, float scrW, float scrH, float scaleX, float scaleY, float degree, float alpha, float R, float G, float B )
		{
			return ReserveStretchedExt
			(
				scrX, scrY, scrW, scrH,
				scaleX, scaleY,
				degree,
				X_MIDDLE | Y_MIDDLE,
				alpha, R, G, B
			);
		}
	#pragma endregion

	#pragma region Part
		bool Batch::ReservePart( float scrX, float scrY, float texX, float texY, float texW, float texH, float degree, XMFLOAT2 center, float alpha )
		{
			return ReservePartExt
			(
				scrX, scrY,
				texX, texY, texW, texH,
				1.0f, 1.0f,
				degree, center,
				alpha, 1.0f, 1.0f, 1.0f
			);
		}
		bool Batch::ReservePart( float scrX, float scrY, float texX, float texY, float texW, float texH, float degree, Origin center, float alpha )
		{
			return ReservePart
			(
				scrX, scrY,
				texX, texY, texW, texH,
				degree,
				MakeSpecifiedCenter( center, texW, texH, 1.0f, 1.0f ),
				alpha
			);
		}
		bool Batch::ReservePart( float scrX, float scrY, float texX, float texY, float texW, float texH, float degree, float alpha )
		{
			return ReservePart
			(
				scrX, scrY,
				texX, texY, texW, texH,
				degree,
				X_MIDDLE | Y_MIDDLE,
				alpha
			);
		}
		bool Batch::ReservePartExt( float scrX, float scrY, float texX, float texY, float texW, float texH, float scaleX, float scaleY, float degree, XMFLOAT2 center, float alpha, float R, float G, float B )
		{
			return ReserveGeneralExt
			(
				scrX, scrY,
				texW, texH,
				texX, texY,
				texW, texH,
				scaleX, scaleY,
				degree, center,
				alpha, R, G, B
			);
		}
		bool Batch::ReservePartExt( float scrX, float scrY, float texX, float texY, float texW, float texH, float scaleX, float scaleY, float degree, Origin center, float alpha, float R, float G, float B )
		{
			return ReservePartExt
			(
				scrX, scrY,
				texX, texY, texW, texH,
				scaleX, scaleY,
				degree,
				MakeSpecifiedCenter( center, texW, texH, scaleX, scaleY ),
				alpha, R, G, B
			);
		}
		bool Batch::ReservePartExt( float scrX, float scrY, float texX, float texY, float texW, float texH, float scaleX, float scaleY, float degree, float alpha, float R, float G, float B )
		{
			return ReservePartExt
			(
				scrX, scrY,
				texX, texY, texW, texH,
				scaleX, scaleY,
				degree,
				X_MIDDLE | Y_MIDDLE,
				alpha, R, G, B
			);
		}
	#pragma endregion

	#pragma region Tiled

	#pragma endregion

	#pragma region General
		bool Batch::ReserveGeneral( float scrX, float scrY, float scrW, float scrH, float texX, float texY, float texW, float texH, float degree, XMFLOAT2 center, float alpha, float R, float G, float B )
		{
			return ReserveGeneralExt
			(
				scrX, scrY, scrW, scrH,
				texX, texY, texW, texH,
				1.0f, 1.0f,
				degree, center,
				alpha, R, G, B
			);
		}
		bool Batch::ReserveGeneral( float scrX, float scrY, float scrW, float scrH, float texX, float texY, float texW, float texH, float degree, Origin center, float alpha, float R, float G, float B )
		{
			return ReserveGeneral
			(
				scrX, scrY, scrW, scrH,
				texX, texY, texW, texH,
				degree, MakeSpecifiedCenter( center, scrW, scrH, 1.0f, 1.0f ),
				alpha, R, G, B
			);
		}
		bool Batch::ReserveGeneralExt( float scrX, float scrY, float scrW, float scrH, float texX, float texY, float texW, float texH, float scaleX, float scaleY, float degree, DirectX::XMFLOAT2 center, float alpha, float R, float G, float B )
		{
			if ( MAX_INSTANCES <= reserveCount ) { return false; }

			// Set rotation center to origin.
			// If don't set, origin is fixed to left-top.
			scrX -= center.x;
			scrY -= center.y;

			if ( ::Donya::ScreenShake::GetEnableState() )
			{
				scrX -= ::Donya::ScreenShake::GetX();
				scrY -= ::Donya::ScreenShake::GetY();
			}

			scrW *= scaleX;
			scrH *= scaleY;

			MakeMatrixNDCTransform
			(
				&instances[reserveCount].NDCTransform,
				scrX, scrY,
				scrW, scrH,
				degree, center.x, center.y,
				GetDrawDepth()
			);

			instances[reserveCount].color.x = R;
			instances[reserveCount].color.y = G;
			instances[reserveCount].color.z = B;
			instances[reserveCount].color.w = Donya::Color::FilteringAlpha( alpha );

			const float TEX_WIDTH  = GetTextureWidthF();
			const float TEX_HEIGHT = GetTextureHeightF();
			instances[reserveCount].texCoordTransform.x = texX / TEX_WIDTH;
			instances[reserveCount].texCoordTransform.y = texY / TEX_HEIGHT;
			instances[reserveCount].texCoordTransform.z = texW / TEX_WIDTH;
			instances[reserveCount].texCoordTransform.w = texH / TEX_HEIGHT;

			reserveCount++;
			return true;

		}
		bool Batch::ReserveGeneralExt( float scrX, float scrY, float scrW, float scrH, float texX, float texY, float texW, float texH, float scaleX, float scaleY, float degree, Origin center, float alpha, float R, float G, float B )
		{
			return ReserveGeneralExt
			(
				scrX, scrY, scrW, scrH,
				texX, texY, texW, texH,
				scaleX, scaleY,
				degree, MakeSpecifiedCenter( center, scrW, scrH, scaleX, scaleX ),
				alpha, R, G, B
			);
		}
	#pragma endregion

		void Batch::Render()
		{
			if ( !reserveCount ) { return; }
			// else

			HRESULT hr = S_OK;
			ID3D11DeviceContext *pImmediateContext = ::Donya::GetImmediateContext();

			// Map
			{
				D3D11_MAPPED_SUBRESOURCE mappedSubresource{};

				hr = pImmediateContext->Map( pInstanceBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubresource );
				_ASSERT_EXPR( SUCCEEDED( hr ), L"Failed : Map()" );

				memcpy( mappedSubresource.pData, instances.data(), sizeof( Instance ) * reserveCount );

				pImmediateContext->Unmap( pInstanceBuffer.Get(), 0 );
			}

			// Setting
			{
				/*
				IA...InputAssembler
				VS...VertexShader
				HS...HullShader
				DS...DomainShader
				GS...GeometryShader
				SO...StreamOutput
				RS...RasterizerState
				PS...PixelShader
				OM...OutputMerger

				CS...ComputeShader
				*/

				constexpr size_t BUFFER_NUM = 2;

				UINT strides[BUFFER_NUM] = { sizeof( Vertex ), sizeof( Instance ) };
				UINT offsets[BUFFER_NUM] = { 0, 0 };
				ID3D11Buffer *pBuffers[BUFFER_NUM] = { pVertexBuffer.Get(), pInstanceBuffer.Get() };
				pImmediateContext->IASetVertexBuffers( 0, BUFFER_NUM, pBuffers, strides, offsets );
				pImmediateContext->IASetPrimitiveTopology( D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP );

				pImmediateContext->PSSetShaderResources( EmbeddedSourceCode::SpriteSlotSRV, 1, pShaderResourceView.GetAddressOf() );

				Shared::ActivateStates();
				Shared::ActivateSpriteShaders();
			}

			pImmediateContext->DrawInstanced( 4, reserveCount, 0, 0 );

			// PostProcessing
			{
				reserveCount = 0;
				instances.clear();
				instances.resize( MAX_INSTANCES );

				ID3D11ShaderResourceView *NullSRV = nullptr;
				pImmediateContext->PSSetShaderResources( EmbeddedSourceCode::SpriteSlotSRV, 1, &NullSRV );

				Shared::DeactivateStates();
				Shared::DeactivateSpriteShaders();
			}
		}

	#pragma endregion

	#pragma region Rect

		Rect::Rect( size_t maxInstances ) :
			MAX_INSTANCES( maxInstances ),
			reserveCount( NULL ), instances(),
			pInstanceBuffer(), pVertexBuffer()
		{
			HRESULT hr = S_OK;
			ID3D11Device *pDevice = ::Donya::GetDevice();

			std::array<Rect::Vertex, 4> vertices =
			{
				Rect::Vertex{ XMFLOAT3( 0.0f, 1.0f, 0.0f ) },
				Rect::Vertex{ XMFLOAT3( 1.0f, 1.0f, 0.0f ) },
				Rect::Vertex{ XMFLOAT3( 0.0f, 0.0f, 0.0f ) },
				Rect::Vertex{ XMFLOAT3( 1.0f, 0.0f, 0.0f ) }
			};

			// Create VertexBuffer
			{
				hr = ::Donya::CreateVertexBuffer<Rect::Vertex>
				(
					pDevice, vertices,
					D3D11_USAGE_IMMUTABLE, 0,
					pVertexBuffer.GetAddressOf()
				);
				_ASSERT_EXPR( SUCCEEDED( hr ), L"Failed : Create vertex-buffer()" );
			}
			// Create Instances and InstanceBuffer
			{
				instances.resize( MAX_INSTANCES );

				hr = ::Donya::CreateVertexBuffer<Rect::Instance>
				(
					pDevice, instances,
					D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE,
					pInstanceBuffer.GetAddressOf()
				);
				_ASSERT_EXPR( SUCCEEDED( hr ), L"Failed : Create instance-buffer()" );
			}
		}
		Rect::~Rect()
		{
			instances.clear();
			instances.shrink_to_fit();
		}

		XMFLOAT2 Rect::MakeCenter( Origin center, float width, float height ) const
		{
			XMFLOAT2 rv{};

			if ( center & X_LEFT )
			{
				rv.x = 0;
			}
			else
			if ( center & X_RIGHT )
			{
				rv.x = width;
			}
			else // This contain not specified case.
			{
				rv.x = width * 0.5f;
			}

			if ( center & Y_TOP )
			{
				rv.y = 0;
			}
			else
			if ( center & Y_BOTTOM )
			{
				rv.y = height;
			}
			else // This contain not specified case.
			{
				rv.y = height * 0.5f;
			}

			return rv;
		}

	#pragma region Reserves
		bool Rect::Reserve( float scrX, float scrY, float scrW, float scrH, float R, float G, float B, float alpha, float degree, DirectX::XMFLOAT2 center )
		{
			if ( MAX_INSTANCES <= reserveCount ) { return false; }

			// Set rotation center to origin.
			// If don't set, origin is fixed to left-top.
			scrX -= center.x;
			scrY -= center.y;

			if ( ::Donya::ScreenShake::GetEnableState() )
			{
				scrX -= ::Donya::ScreenShake::GetX();
				scrY -= ::Donya::ScreenShake::GetY();
			}

			instances[reserveCount].color.x = R;
			instances[reserveCount].color.y = G;
			instances[reserveCount].color.z = B;
			instances[reserveCount].color.w = Donya::Color::FilteringAlpha( alpha );

			MakeMatrixNDCTransform
			(
				&instances[reserveCount].NDCTransform,
				scrX, scrY,
				scrW, scrH,
				degree, center.x, center.y,
				GetDrawDepth()
			);

			reserveCount++;
			return true;
		}
		bool Rect::Reserve( float scrX, float scrY, float scrW, float scrH, Donya::Color::Code color, float alpha, float degree, DirectX::XMFLOAT2 center )
		{
			XMFLOAT3 RGB = MakeColor( color );
			return Reserve
			(
				scrX, scrY, scrW, scrH,
				RGB.x, RGB.y, RGB.z, alpha,
				degree, center
			);
		}
		bool Rect::Reserve( float scrX, float scrY, float scrW, float scrH, float R, float G, float B, float alpha, float degree, Origin center )
		{
			return Reserve
			(
				scrX, scrY, scrW, scrH,
				R, G, B, alpha,
				degree, MakeCenter( center, scrW, scrH )
			);
		}
		bool Rect::Reserve( float scrX, float scrY, float scrW, float scrH, Donya::Color::Code color, float alpha, float degree, Origin center )
		{
			XMFLOAT3 RGB = MakeColor( color );
			return Reserve
			(
				scrX, scrY, scrW, scrH,
				RGB.x, RGB.y, RGB.z, alpha,
				degree, MakeCenter( center, scrW, scrH )
			);
		}
		bool Rect::Reserve( float scrX, float scrY, float scrW, float scrH, float R, float G, float B, float alpha, float degree )
		{
			return Reserve
			(
				scrX, scrY, scrW, scrH,
				R, G, B, alpha,
				degree, X_MIDDLE | Y_MIDDLE
			);
		}
		bool Rect::Reserve( float scrX, float scrY, float scrW, float scrH, Donya::Color::Code color, float alpha, float degree )
		{
			XMFLOAT3 RGB = MakeColor( color );
			return Reserve
			(
				scrX, scrY, scrW, scrH,
				RGB.x, RGB.y, RGB.z, alpha,
				degree
			);
		}
	#pragma endregion

		void Rect::Render()
		{
			if ( !reserveCount ) { return; }
			// else

			HRESULT hr = S_OK;
			ID3D11DeviceContext *pImmediateContext = ::Donya::GetImmediateContext();

			// Map
			{
				D3D11_MAPPED_SUBRESOURCE mappedSubresource{};

				hr = pImmediateContext->Map( pInstanceBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubresource );
				_ASSERT_EXPR( SUCCEEDED( hr ), L"Failed : Map()" );

				memcpy( mappedSubresource.pData, instances.data(), sizeof( Rect::Instance ) * reserveCount );

				pImmediateContext->Unmap( pInstanceBuffer.Get(), 0 );
			}

			// Setting
			{
				/*
				IA...InputAssembler
				VS...VertexShader
				HS...HullShader
				DS...DomainShader
				GS...GeometryShader
				SO...StreamOutput
				RS...RasterizerState
				PS...PixelShader
				OM...OutputMerger

				CS...ComputeShader
				*/

				constexpr size_t BUFFER_NUM = 2;

				UINT strides[BUFFER_NUM] = { sizeof( Rect::Vertex ), sizeof( Rect::Instance ) };
				UINT offsets[BUFFER_NUM] = { 0, 0 };
				ID3D11Buffer *pBuffers[BUFFER_NUM] = { pVertexBuffer.Get(), pInstanceBuffer.Get() };
				pImmediateContext->IASetVertexBuffers( 0, BUFFER_NUM, pBuffers, strides, offsets );
				pImmediateContext->IASetPrimitiveTopology( D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP );

				Shared::ActivateStates();
				Shared::ActivateRectShaders();
			}

			pImmediateContext->DrawInstanced( 4, reserveCount, 0, 0 );

			// PostProcessing
			{
				reserveCount = 0;
				instances.clear();
				instances.resize( MAX_INSTANCES );

				Shared::DeactivateStates();
				Shared::DeactivateRectShaders();
			}
		}

	#pragma endregion

	#pragma region Circle

		struct VerticesAndIndices
		{
			std::vector<Circle::Vertex>	vertices{};
			std::vector<size_t>			indices{};
		};
		VerticesAndIndices CalcVertices( unsigned int vertexCountPerQuadrant )
		{
			// The count does not include last vertex.
			unsigned int &vertCount = vertexCountPerQuadrant;

			constexpr unsigned int MAX_SUPPORT_VERTEX_COUNT = 32U;
			vertCount = std::min( MAX_SUPPORT_VERTEX_COUNT, vertCount );
			vertCount = std::max( 1U, vertCount );
			
			float radian{}, cos{}, sin{};
			float divRadian = ToRadian( 90.0f / vertCount );

			const size_t VERTICES_SIZE = vertCount * 4U;
			VerticesAndIndices bundle{};
			bundle.vertices.resize( VERTICES_SIZE + 1/*The center*/ );

			bundle.vertices[0].pos = { 0.5f, 0.5f, 0.0f };// [0] is the center of circle.
			for ( size_t i = 1; i < VERTICES_SIZE + 1/*The center*/; ++i )
			{
				// The positions within range of [0.0f ~ 1.0f].

				cos = cosf( radian ) * 0.5f;
				sin = sinf( radian ) * 0.5f;

				bundle.vertices[i].pos.x = 0.5f + cos;
				bundle.vertices[i].pos.y = 0.5f + sin;
				bundle.vertices[i].pos.z = 0.0f;

				radian += divRadian;
			}

			const size_t INDICES_SIZE = VERTICES_SIZE * 3;
			bundle.indices.resize( INDICES_SIZE );
			for ( size_t i = 1, v = 1; i < INDICES_SIZE; i += 3, ++v )
			{
				if ( INDICES_SIZE - 3 < i + 1 )
				{
					bundle.indices[i - 1] = 0;
					bundle.indices[i + 0] = 1;
					bundle.indices[i + 1] = v;
				}
				else
				{
					bundle.indices[i - 1]	= 0;
					bundle.indices[i + 0]	= v + 1;
					bundle.indices[i + 1]	= v;
				}
			}

			return bundle;
		}

		Circle::Circle( size_t vertexCount, size_t maxInstances ) :
			MAX_INSTANCES( maxInstances ),
			reserveCount( NULL ), currentDetail( vertexCount ), instances(),
			pInstanceBuffer(), pIndexBuffer(), pVertexBuffer()
		{
			HRESULT hr = S_OK;
			ID3D11Device *pDevice = ::Donya::GetDevice();

			auto buffers = CalcVertices( currentDetail );

			// Create VertexBuffer
			{
				hr = Donya::CreateVertexBuffer<Circle::Vertex>
				(
					pDevice, buffers.vertices,
					D3D11_USAGE_IMMUTABLE, 0,
					pVertexBuffer.GetAddressOf()
				);
				_ASSERT_EXPR( SUCCEEDED( hr ), L"Failed : Create vertex-buffer()" );
			}
			// Create IndexBuffer
			{
				hr = Donya::CreateIndexBuffer
				(
					pDevice,
					buffers.indices,
					pIndexBuffer.GetAddressOf()
				);
				_ASSERT_EXPR( SUCCEEDED( hr ), L"Failed : Create index-buffer()" );
			}
			// Create Instances and InstanceBuffer
			{
				instances.resize( MAX_INSTANCES );
				hr = Donya::CreateVertexBuffer<Circle::Instance>
				(
					pDevice, instances,
					D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE,
					pInstanceBuffer.GetAddressOf()
				);
				_ASSERT_EXPR( SUCCEEDED( hr ), L"Failed : Create instance-buffer()" );
			}
		}
		Circle::~Circle()
		{
			instances.clear();
			instances.shrink_to_fit();
		}

		XMFLOAT2 Circle::MakeCenter( Origin center, float diameter ) const
		{
			XMFLOAT2 rv{};

			if ( center & X_LEFT )
			{
				rv.x = 0;
			}
			else
			if ( center & X_RIGHT )
			{
				rv.x = diameter;
			}
			else // This contain not specified case.
			{
				rv.x = diameter * 0.5f;
			}

			if ( center & Y_TOP )
			{
				rv.y = 0;
			}
			else
			if ( center & Y_BOTTOM )
			{
				rv.y = diameter;
			}
			else // This contain not specified case.
			{
				rv.y = diameter * 0.5f;
			}

			return rv;
		}

	#pragma region Reserves
		bool Circle::Reserve( float scrX, float scrY, float diameter, float R, float G, float B, float alpha, Origin center )
		{
			return Reserve
			(
				scrX, scrY,
				diameter,
				R, G, B, alpha,
				MakeCenter( center, diameter )
			);
		}
		bool Circle::Reserve( float scrX, float scrY, float diameter, Donya::Color::Code color, float alpha, Origin center )
		{
			XMFLOAT3 RGB = MakeColor( color );
			return Reserve
			(
				scrX, scrY,
				diameter,
				RGB.x, RGB.y, RGB.z, alpha,
				MakeCenter( center, diameter )
			);
		}
		bool Circle::Reserve( float scrX, float scrY, float diameter, float R, float G, float B, float alpha, XMFLOAT2 center )
		{
			if ( MAX_INSTANCES <= reserveCount ) { return false; }

			// Set rotation center to origin.
			// If don't set, origin is fixed to left-top.
			scrX -= center.x;
			scrY -= center.y;

			if ( ::Donya::ScreenShake::GetEnableState() )
			{
				scrX -= ::Donya::ScreenShake::GetX();
				scrY -= ::Donya::ScreenShake::GetY();
			}

			instances[reserveCount].color.x = R;
			instances[reserveCount].color.y = G;
			instances[reserveCount].color.z = B;
			instances[reserveCount].color.w = Donya::Color::FilteringAlpha( alpha );

			MakeMatrixNDCTransform
			(
				&instances[reserveCount].NDCTransform,
				scrX, scrY,
				diameter, diameter,
				0.0f, center.x, center.y,
				GetDrawDepth()
			);

			reserveCount++;
			return true;
		}
	#pragma endregion

		void Circle::Render()
		{
			if ( !reserveCount ) { return; }
			// else

			HRESULT hr = S_OK;
			ID3D11DeviceContext *pImmediateContext = ::Donya::GetImmediateContext();

			// Map
			{
				D3D11_MAPPED_SUBRESOURCE mappedSubresource{};

				hr = pImmediateContext->Map( pInstanceBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubresource );
				_ASSERT_EXPR( SUCCEEDED( hr ), L"Failed : Map()" );

				memcpy( mappedSubresource.pData, instances.data(), sizeof( Circle::Instance ) * reserveCount );

				pImmediateContext->Unmap( pInstanceBuffer.Get(), 0 );
			}

			// Setting
			{
				/*
				IA...InputAssembler
				VS...VertexShader
				HS...HullShader
				DS...DomainShader
				GS...GeometryShader
				SO...StreamOutput
				RS...RasterizerState
				PS...PixelShader
				OM...OutputMerger

				CS...ComputeShader
				*/

				constexpr size_t BUFFER_NUM = 2;

				UINT strides[BUFFER_NUM] = { sizeof( Circle::Vertex ), sizeof( Circle::Instance ) };
				UINT offsets[BUFFER_NUM] = { 0, 0 };
				ID3D11Buffer *pBuffers[BUFFER_NUM] = { pVertexBuffer.Get(), pInstanceBuffer.Get() };
				pImmediateContext->IASetIndexBuffer( pIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0 );
				pImmediateContext->IASetVertexBuffers( 0, BUFFER_NUM, pBuffers, strides, offsets );
				pImmediateContext->IASetPrimitiveTopology( D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

				Shared::ActivateStates();
				Shared::ActivateRectShaders();
			}

			for ( size_t i = 0; i < currentDetail * 4U; ++i )
			{
				pImmediateContext->DrawIndexedInstanced
				(
					3U,
					reserveCount,
					i * 3,
					0, 0
				);
			}
			
			// PostProcessing
			{
				reserveCount = 0;
				instances.clear();
				instances.resize( MAX_INSTANCES );

				Shared::DeactivateStates();
				Shared::DeactivateRectShaders();
			}
		}

	#pragma endregion

	#pragma region Agent

		struct Agent
		{
			// Note: container's type isn't need std::unique_ptr, but Sprite::Batch can not copy, so I wrapped by pointer.

			size_t lastReservedIdentifier;

			std::unique_ptr<Sprite::Rect>	pRect;
			std::unique_ptr<Sprite::Circle>	pCircle;

			std::vector<std::unique_ptr<Sprite::Batch> *> ppDrawList;

			std::unordered_map<size_t, std::unique_ptr<Sprite::Batch>> pSprites;

			bool nowBatchingPrimitive;	// Used to associate Rect and Batch.
		public:
			Agent( unsigned int maxInstanceCntOfPrim, unsigned int vertexCntOfCirclePerQuad ) :
				lastReservedIdentifier( NULL ),
				pRect( std::make_unique<Sprite::Rect>( maxInstanceCntOfPrim ) ),
				pCircle( std::make_unique<Sprite::Circle>( vertexCntOfCirclePerQuad, maxInstanceCntOfPrim ) ),
				ppDrawList(), pSprites(),
				nowBatchingPrimitive( false )
			{
			
			}
			~Agent()
			{
				pRect.reset( nullptr );
				ppDrawList.clear();
				pSprites.clear();
			}
		};
		static std::unique_ptr<Agent> pAgent;

		bool Init( unsigned int maxInstanceCntOfPrim, unsigned int vertexCntOfCirclePerQuad )
		{
			if ( Shared::AlreadyInitialized() ) { return true; }
			// else

			pAgent = std::make_unique<Agent>( maxInstanceCntOfPrim, vertexCntOfCirclePerQuad );

			bool   succeeded = Shared::Init();
			return succeeded;
		}

		void Uninit()
		{
			pAgent.reset( nullptr );
		}

		bool AssertIfNotInitialized()
		{
			if ( pAgent == nullptr )
			{
				_ASSERT_EXPR( 0, L"Error : has not called Donya::Sprite::Init()." );
				return true;
			}
			// else
			return false;
		}

		size_t Load( const std::wstring &spriteFileName, size_t maxInstancesCount )
		{
			if ( AssertIfNotInitialized() ) { return NULL; }
			// else

			if ( !::Donya::IsExistFile( spriteFileName ) ) { return NULL; }
			// else

			size_t hash = std::hash<std::wstring>()( spriteFileName );
			if ( hash == NULL )	// NULL using error code.
			{
				hash = 1;
			}

			// already loaded.
			{
				decltype( pAgent->pSprites )::iterator it = pAgent->pSprites.find( hash );
				if ( it != pAgent->pSprites.end() )
				{
					return hash;
				}
			}

			pAgent->pSprites.insert
			(
				std::make_pair
				(
					hash,
					std::make_unique<Sprite::Batch>
					(
						spriteFileName,
						maxInstancesCount
					)
				)
			);

			return hash;
		}

		/// <summary>
		/// If this function returns valid iterator, that iterator is guarantee to valid.
		/// </summary>
		decltype( pAgent->pSprites )::iterator FindSpriteOrEnd( size_t spriteIdentifier )
		{
			if ( AssertIfNotInitialized() ) { return pAgent->pSprites.end(); }
			if ( spriteIdentifier == NULL ) { return pAgent->pSprites.end(); }
			// else

			return pAgent->pSprites.find( spriteIdentifier );
		}

	#pragma region GetTextureSizes

		int GetTexturWidth( size_t spriteIdentifier )
		{
			auto it = FindSpriteOrEnd( spriteIdentifier );
			if ( it == pAgent->pSprites.end() ) { return -1; }
			// else

			return it->second->GetTextureWidth();
		}
		int GetTextureHeight( size_t spriteIdentifier )
		{
			auto it = FindSpriteOrEnd( spriteIdentifier );
			if ( it == pAgent->pSprites.end() ) { return -1; }
			// else

			return it->second->GetTextureHeight();
		}
		float GetTextureWidthF( size_t spriteIdentifier )
		{
			auto it = FindSpriteOrEnd( spriteIdentifier );
			if ( it == pAgent->pSprites.end() ) { return -1.0f; }
			// else

			return it->second->GetTextureWidthF();
		}
		float GetTextureHeightF( size_t spriteIdentifier )
		{
			auto it = FindSpriteOrEnd( spriteIdentifier );
			if ( it == pAgent->pSprites.end() ) { return -1.0f; }
			// else

			return it->second->GetTextureHeightF();
		}
		Donya::Int2 GetTextureSize( size_t spriteIdentifier )
		{
			auto it = FindSpriteOrEnd( spriteIdentifier );
			if ( it == pAgent->pSprites.end() ) { return Donya::Int2{ -1, -1 }; }
			// else

			return it->second->GetTextureSize();
		}
		Donya::Vector2 GetTextureSizeF( size_t spriteIdentifier )
		{
			auto it = FindSpriteOrEnd( spriteIdentifier );
			if ( it == pAgent->pSprites.end() ) { return Donya::Vector2{ -1.0f, -1.0f }; }
			// else

			return it->second->GetTextureSizeF();
		}
		bool GetTextureSize( size_t spriteIdentifier, int *width, int *height )
		{
			decltype( pAgent->pSprites )::iterator it = FindSpriteOrEnd( spriteIdentifier );
			if ( it == pAgent->pSprites.end() ) { return false; }
			// else

			it->second->GetTextureSize( width, height );
			return true;
		}
		bool GetTextureSize( size_t spriteIdentifier, float *width, float *height )
		{
			decltype( pAgent->pSprites )::iterator it = FindSpriteOrEnd( spriteIdentifier );
			if ( it == pAgent->pSprites.end() ) { return false; }
			// else

			it->second->GetTextureSize( width, height );
			return true;
		}

	#pragma endregion

	#pragma region Draw Functions

		/*
		First, if continue draw same sprite, only reserving.
		Second, if exists batching sprite, rendering it.
		Finally, register id of batching sprite, and reserve.
		*/

		void FlushBatch()
		{
			if ( pAgent->lastReservedIdentifier != NULL )
			{
				( *pAgent->ppDrawList.back() )->Render();
			}

			pAgent->lastReservedIdentifier = NULL;
			pAgent->ppDrawList.clear();
		}
		void FlushPrimitive()
		{
			pAgent->pRect->Render();
			pAgent->pCircle->Render();
		}

		void SwitchBatchFromPrimitive()
		{
			FlushPrimitive();
			pAgent->nowBatchingPrimitive = false;
		}
		void SwitchPrimitiveFromBatch()
		{
			FlushBatch();
			pAgent->nowBatchingPrimitive = true;
		}

	#pragma region Normal
		bool Draw( size_t sprId, float scrX, float scrY, float degree, DirectX::XMFLOAT2 center, float alpha )
		{
			return DrawExt
			(
				sprId,
				scrX, scrY, 1.0f, 1.0f,
				degree, center,
				alpha, 1.0f, 1.0f, 1.0f
			);
		}
		bool Draw( size_t sprId, float scrX, float scrY, float degree, Origin center, float alpha )
		{
			auto it = FindSpriteOrEnd( sprId );
			if ( it == pAgent->pSprites.end() ) { return false; }
			// else
			return Draw( sprId, scrX, scrY, degree, it->second->MakeSpriteCenter( center, 1.0f, 1.0f ), alpha );
		}
		bool Draw( size_t sprId, float scrX, float scrY, float degree, float alpha )
		{
			return Draw( sprId, scrX, scrY, degree, X_MIDDLE | Y_MIDDLE, alpha );
		}
		bool DrawExt( size_t sprId, float scrX, float scrY, float scaleX, float scaleY, float degree, DirectX::XMFLOAT2 center, float alpha, float R, float G, float B )
		{
			auto it = FindSpriteOrEnd( sprId );
			if ( it == pAgent->pSprites.end() ) { return false; }
			// else

			XMFLOAT2 size{};
			it->second->GetTextureSize( &size.x, &size.y );

			return DrawGeneral
			(
				sprId,
				scrX, scrY,
				size.x * scaleX, size.y * scaleY,
				0, 0,
				size.x, size.y,
				degree, center,
				alpha, R, G, B
			);
		}
		bool DrawExt( size_t sprId, float scrX, float scrY, float scaleX, float scaleY, float degree, Origin center, float alpha, float R, float G, float B )
		{
			auto it = FindSpriteOrEnd( sprId );
			if ( it == pAgent->pSprites.end() ) { return false; }
			// else
			return DrawExt
			(
				sprId,
				scrX, scrY,
				scaleX, scaleY,
				degree, it->second->MakeSpriteCenter( center, scaleX, scaleY ),
				alpha, R, G, B
			);
		}
		bool DrawExt( size_t sprId, float scrX, float scrY, float scaleX, float scaleY, float degree, float alpha, float R, float G, float B )
		{
			return DrawExt
			(
				sprId,
				scrX, scrY,
				scaleX, scaleY,
				degree,
				X_MIDDLE | Y_MIDDLE,
				alpha, R, G, B
			);
		}
	#pragma endregion

	#pragma region Stretched
		bool DrawStretched( size_t sprId, float scrX, float scrY, float scrW, float scrH, float degree, DirectX::XMFLOAT2 center, float alpha )
		{
			auto it = FindSpriteOrEnd( sprId );
			if ( it == pAgent->pSprites.end() ) { return false; }
			// else
			return DrawStretchedExt
			(
				sprId,
				scrX, scrY, scrW, scrH,
				1.0f, 1.0f,
				degree, center,
				alpha, 1.0f, 1.0f, 1.0f
			);
		}
		bool DrawStretched( size_t sprId, float scrX, float scrY, float scrW, float scrH, float degree, Origin center, float alpha )
		{
			auto it = FindSpriteOrEnd( sprId );
			if ( it == pAgent->pSprites.end() ) { return false; }
			// else
			return DrawStretched
			(
				sprId,
				scrX, scrY, scrW, scrH,
				degree, it->second->MakeSpriteCenter( center, 1.0f, 1.0f ),
				alpha
			);
		}
		bool DrawStretched( size_t sprId, float scrX, float scrY, float scrW, float scrH, float degree, float alpha )
		{
			return DrawStretched( sprId, scrX, scrY, scrW, scrH, degree, X_MIDDLE | Y_MIDDLE, alpha );
		}
		bool DrawStretchedExt( size_t sprId, float scrX, float scrY, float scrW, float scrH, float scaleX, float scaleY, float degree, DirectX::XMFLOAT2 center, float alpha, float R, float G, float B )
		{
			auto it = FindSpriteOrEnd( sprId );
			if ( it == pAgent->pSprites.end() ) { return false; }
			// else

			XMFLOAT2 size{};
			it->second->GetTextureSize( &size.x, &size.y );

			return DrawGeneralExt
			(
				sprId,
				scrX, scrY,
				scrW, scrH,
				0, 0,
				size.x, size.y,
				scaleX, scaleY,
				degree, center,
				alpha, R, G, B
			);
		}
		bool DrawStretchedExt( size_t sprId, float scrX, float scrY, float scrW, float scrH, float scaleX, float scaleY, float degree, Origin center, float alpha, float R, float G, float B )
		{
			auto it = FindSpriteOrEnd( sprId );
			if ( it == pAgent->pSprites.end() ) { return false; }
			// else
			return DrawStretchedExt
			(
				sprId,
				scrX, scrY, scrW, scrH,
				scaleX, scaleY,
				degree, it->second->MakeSpriteCenter( center, scaleX, scaleY ),
				alpha, R, G, B
			);
		}
		bool DrawStretchedExt( size_t sprId, float scrX, float scrY, float scrW, float scrH, float scaleX, float scaleY, float degree, float alpha, float R, float G, float B )
		{
			return DrawStretchedExt
			(
				sprId,
				scrX, scrY, scrW, scrH,
				scaleX, scaleY,
				degree,
				X_MIDDLE | Y_MIDDLE,
				alpha, R, G, B
			);
		}
	#pragma endregion

	#pragma region Part
		bool DrawPart( size_t sprId, float scrX, float scrY, float texX, float texY, float texW, float texH, float degree, DirectX::XMFLOAT2 center, float alpha )
		{
			auto it = FindSpriteOrEnd( sprId );
			if ( it == pAgent->pSprites.end() ) { return false; }
			// else
			return DrawPartExt
			(
				sprId,
				scrX, scrY,
				texX, texY, texW, texH,
				1.0f, 1.0f,
				degree, center,
				alpha, 1.0f, 1.0f, 1.0f
			);
		}
		bool DrawPart( size_t sprId, float scrX, float scrY, float texX, float texY, float texW, float texH, float degree, Origin center, float alpha )
		{
			auto it = FindSpriteOrEnd( sprId );
			if ( it == pAgent->pSprites.end() ) { return false; }
			// else
			return DrawPart
			(
				sprId,
				scrX, scrY,
				texX, texY, texW, texH,
				degree, it->second->MakeSpecifiedCenter( center, texW, texH, 1.0f, 1.0f ),
				alpha
			);
		}
		bool DrawPart( size_t sprId, float scrX, float scrY, float texX, float texY, float texW, float texH, float degree, float alpha )
		{
			return DrawPart
			(
				sprId,
				scrX, scrY,
				texX, texY, texW, texH,
				degree, X_MIDDLE | Y_MIDDLE,
				alpha
			);
		}
		bool DrawPartExt( size_t sprId, float scrX, float scrY, float texX, float texY, float texW, float texH, float scaleX, float scaleY, float degree, DirectX::XMFLOAT2 center, float alpha, float R, float G, float B )
		{
			return DrawGeneral
			(
				sprId,
				scrX, scrY,
				texW * scaleX,
				texH * scaleY,
				texX, texY, texW, texH,
				degree, center,
				alpha, R, G, B
			);
		}
		bool DrawPartExt( size_t sprId, float scrX, float scrY, float texX, float texY, float texW, float texH, float scaleX, float scaleY, float degree, Origin center, float alpha, float R, float G, float B )
		{
			auto it = FindSpriteOrEnd( sprId );
			if ( it == pAgent->pSprites.end() ) { return false; }
			// else
			return DrawPartExt
			(
				sprId,
				scrX, scrY,
				texX, texY, texW, texH,
				scaleX, scaleY,
				degree, it->second->MakeSpecifiedCenter( center, texW, texH, scaleX, scaleY ),
				alpha, R, G, B
			);
		}
		bool DrawPartExt( size_t sprId, float scrX, float scrY, float texX, float texY, float texW, float texH, float scaleX, float scaleY, float degree, float alpha, float R, float G, float B )
		{
			return DrawPartExt
			(
				sprId,
				scrX, scrY,
				texX, texY, texW, texH,
				scaleX, scaleY,
				degree, X_MIDDLE | Y_MIDDLE,
				alpha, R, G, B
			);
		}
	#pragma endregion

	#pragma region Tiled

	#pragma endregion

	#pragma region General
		bool DrawGeneral( size_t sprId, float scrX, float scrY, float scrW, float scrH, float texX, float texY, float texW, float texH, float degree, DirectX::XMFLOAT2 center, float alpha, float R, float G, float B )
		{
			return DrawGeneralExt
			(
				sprId,
				scrX, scrY, scrW, scrH,
				texX, texY, texW, texH,
				1.0f, 1.0f,
				degree, center,
				alpha, R, G, B
			);
		}
		bool DrawGeneral( size_t sprId, float scrX, float scrY, float scrW, float scrH, float texX, float texY, float texW, float texH, float degree, Origin center, float alpha, float R, float G, float B )
		{
			auto it = FindSpriteOrEnd( sprId );
			if ( it == pAgent->pSprites.end() ) { return false; }
			// else
			return DrawGeneral
			(
				sprId,
				scrX, scrY, scrW, scrH,
				texX, texY, texW, texH,
				degree, it->second->MakeSpecifiedCenter( center, scrW, scrH, 1.0f, 1.0f ),
				alpha, R, G, B
			);
		}
		bool DrawGeneralExt( size_t sprId, float scrX, float scrY, float scrW, float scrH, float texX, float texY, float texW, float texH, float scaleX, float scaleY, float degree, DirectX::XMFLOAT2 center, float alpha, float R, float G, float B )
		{
			auto it = FindSpriteOrEnd( sprId );
			if ( it == pAgent->pSprites.end() ) { return false; }
			// else

			if ( pAgent->nowBatchingPrimitive )
			{
				SwitchBatchFromPrimitive();
			}

			if ( pAgent->lastReservedIdentifier != sprId )
			{
				if ( pAgent->lastReservedIdentifier != NULL )
				{
					( *pAgent->ppDrawList.back() )->Render();
				}

				pAgent->lastReservedIdentifier = sprId;

				pAgent->ppDrawList.push_back( &it->second );
			}

			return ( *pAgent->ppDrawList.back() )->ReserveGeneralExt
			(
				scrX, scrY, scrW, scrH,
				texX, texY, texW, texH,
				scaleX, scaleY,
				degree, center,
				alpha, R, G, B
			);
		}
		bool DrawGeneralExt( size_t sprId, float scrX, float scrY, float scrW, float scrH, float texX, float texY, float texW, float texH, float scaleX, float scaleY, float degree, Origin center, float alpha, float R, float G, float B )
		{
			auto it = FindSpriteOrEnd( sprId );
			if ( it == pAgent->pSprites.end() ) { return false; }
			// else
			return DrawGeneralExt
			(
				sprId,
				scrX, scrY, scrW, scrH,
				texX, texY, texW, texH,
				scaleX, scaleY,
				degree, it->second->MakeSpecifiedCenter( center, scrW, scrH, scaleX, scaleY ),
				alpha, R, G, B
			);
		}
	#pragma endregion

	#pragma region String
		Donya::Int2 CalcTextCharPlace( char character )
		{
			constexpr int ASCII_ROW		= 16;
			constexpr int ASCII_COLUMN	= 8;

			int x = character % ASCII_ROW;
			int y = character / ASCII_ROW;

			return Donya::Int2{ x, y };
		}
		bool DrawString( size_t sprId, std::string str, float scrX, float scrY, float scrW, float scrH, float texW, float texH, float degree, DirectX::XMFLOAT2 center, float alpha, float R, float G, float B )
		{
			return DrawStringExt
			(
				sprId, str,
				scrX, scrY, scrW, scrH,
				texW, texH,
				1.0f, 1.0f,
				degree, center,
				alpha, R, G, B
			);
		}
		bool DrawString( size_t sprId, std::string str, float scrX, float scrY, float scrW, float scrH, float texW, float texH, float degree, Origin center, float alpha, float R, float G, float B )
		{
			auto it = FindSpriteOrEnd( sprId );
			if ( it == pAgent->pSprites.end() ) { return false; }
			// else
			return DrawStringExt
			(
				sprId, str,
				scrX, scrY, scrW, scrH,
				texW, texH,
				1.0f, 1.0f,
				degree, it->second->MakeSpecifiedCenter( center, scrW, scrH, 1.0f, 1.0f ),
				alpha, R, G, B
			);
		}
		bool DrawStringExt( size_t sprId, std::string str, float scrX, float scrY, float scrW, float scrH, float texW, float texH, float scaleX, float scaleY, float degree, DirectX::XMFLOAT2 center, float alpha, float R, float G, float B )
		{
			auto it = FindSpriteOrEnd( sprId );
			if ( it == pAgent->pSprites.end() ) { return false; }
			// else

			if ( pAgent->nowBatchingPrimitive )
			{
				SwitchBatchFromPrimitive();
			}

			if ( pAgent->lastReservedIdentifier != sprId )
			{
				if ( pAgent->lastReservedIdentifier != NULL )
				{
					( *pAgent->ppDrawList.back() )->Render();
				}

				pAgent->lastReservedIdentifier = sprId;

				pAgent->ppDrawList.push_back( &it->second );
			}

			Donya::Vector2 texPos{};
			bool succeeded = true;
			auto &sprite = ( *pAgent->ppDrawList.back() );

			size_t end = str.size();
			for ( size_t i = 0; i < end; ++i )
			{
				texPos = CalcTextCharPlace( str[i] ).Float();
				texPos.x *= texW;
				texPos.y *= texH;

				bool result = sprite->ReserveGeneralExt
				(
					scrX + ( scrW * scaleX * i ),
					scrY,
					scrW, scrH,
					texPos.x, texPos.y,
					texW, texH,
					scaleX, scaleY,
					degree, center,
					alpha, R, G, B
				);

				if ( !result )
				{
					succeeded = false;
				}
			}

			return succeeded;
		}
		bool DrawStringExt( size_t sprId, std::string str, float scrX, float scrY, float scrW, float scrH, float texW, float texH, float scaleX, float scaleY, float degree, Origin center, float alpha, float R, float G, float B )
		{
			auto it = FindSpriteOrEnd( sprId );
			if ( it == pAgent->pSprites.end() ) { return false; }
			// else
			return DrawStringExt
			(
				sprId, str,
				scrX, scrY, scrW, scrH,
				texW, texH,
				scaleX, scaleY,
				degree, it->second->MakeSpecifiedCenter( center, scrW, scrH, scaleX, scaleY ),
				alpha, R, G, B
			);
		}
	#pragma endregion

	#pragma region Rect
		bool DrawRect( float scrX, float scrY, float scrW, float scrH, float R, float G, float B, float alpha, float degree, DirectX::XMFLOAT2 center )
		{
			if ( AssertIfNotInitialized() ) { return false; }
			// else

			if ( !pAgent->nowBatchingPrimitive )
			{
				SwitchPrimitiveFromBatch();
			}

			return pAgent->pRect->Reserve
			(
				scrX, scrY, scrW, scrH,
				R, G, B, alpha,
				degree, center
			);
		}
		bool DrawRect( float scrX, float scrY, float scrW, float scrH, Donya::Color::Code color, float alpha, float degree, DirectX::XMFLOAT2 center )
		{
			if ( AssertIfNotInitialized() ) { return false; }
			// else
			XMFLOAT3 RGB = Donya::Color::MakeColor( color );
			return DrawRect
			(
				scrX, scrY, scrW, scrH,
				RGB.x, RGB.y, RGB.z, alpha,
				degree, center
			);
		}
		bool DrawRect( float scrX, float scrY, float scrW, float scrH, float R, float G, float B, float alpha, float degree, Origin center )
		{
			if ( AssertIfNotInitialized() ) { return false; }
			// else
			return DrawRect
			(
				scrX, scrY, scrW, scrH,
				R, G, B, alpha,
				degree, pAgent->pRect->MakeCenter( center, scrW, scrH )
			);
		}
		bool DrawRect( float scrX, float scrY, float scrW, float scrH, Donya::Color::Code color, float alpha, float degree, Origin center )
		{
			if ( AssertIfNotInitialized() ) { return false; }
			// else
			XMFLOAT3 RGB = Donya::Color::MakeColor( color );
			return DrawRect
			(
				scrX, scrY, scrW, scrH,
				RGB.x, RGB.y, RGB.z, alpha,
				degree, pAgent->pRect->MakeCenter( center, scrW, scrH )
			);
		}
		bool DrawRect( float scrX, float scrY, float scrW, float scrH, float R, float G, float B, float alpha, float degree )
		{
			if ( AssertIfNotInitialized() ) { return false; }
			// else
			return DrawRect
			(
				scrX, scrY, scrW, scrH,
				R, G, B, alpha,
				degree, X_MIDDLE | Y_MIDDLE
			);
		}
		bool DrawRect( float scrX, float scrY, float scrW, float scrH, Donya::Color::Code color, float alpha, float degree )
		{
			if ( AssertIfNotInitialized() ) { return false; }
			// else
			XMFLOAT3 RGB = Donya::Color::MakeColor( color );
			return DrawRect
			(
				scrX, scrY, scrW, scrH,
				RGB.x, RGB.y, RGB.z, alpha,
				degree
			);
		}
	#pragma endregion

	#pragma region Circle
		bool DrawCircle( float scrX, float scrY, float diameter, float R, float G, float B, float alpha, Origin center )
		{
			if ( AssertIfNotInitialized() ) { return false; }
			// else

			if ( !pAgent->nowBatchingPrimitive )
			{
				SwitchPrimitiveFromBatch();
			}

			return pAgent->pCircle->Reserve
			(
				scrX, scrY, diameter,
				R, G, B, alpha,
				center
			);
		}
		bool DrawCircle( float scrX, float scrY, float diameter, Donya::Color::Code color, float alpha, Origin center )
		{
			if ( AssertIfNotInitialized() ) { return false; }
			// else
			XMFLOAT3 RGB = Donya::Color::MakeColor( color );
			return DrawCircle
			(
				scrX, scrY, diameter,
				RGB.x, RGB.y, RGB.z, alpha,
				center
			);
		}
	#pragma endregion

	#pragma endregion

		void Flush()
		{
			if ( AssertIfNotInitialized() ) { return; }
			// else

			FlushBatch();
			FlushPrimitive();

			pAgent->lastReservedIdentifier = NULL;
			pAgent->ppDrawList.clear();
		}

		void PostDraw()
		{
			if ( AssertIfNotInitialized() ) { return; }
			// else

			Flush();
		}

	#pragma endregion

		namespace Deprecated
		{

		#pragma region Single

			constexpr const char *SpriteShaderSource()
			{
				return
					"Texture2D		diffuseMap			: register( t0 );\n"
					"SamplerState	diffuseMapSampler	: register( s0 );\n"
					"struct VS_OUT\n"
					"{\n"
					"	float4 pos		: SV_POSITION;\n"
					"	float4 color	: COLOR;\n"
					"	float2 texCoord	: TEXCOORD;\n"
					"};\n"
					"struct VS_IN\n"
					"{\n"
					"	float4 pos		: POSITION;\n"
					"	float4 color	: COLOR;\n"
					"	float2 texCoord	: TEXCOORD;\n"
					"};\n"
					"VS_OUT VSMain( VS_IN vin )\n"
					"{\n"
					"	VS_OUT vout		= (VS_OUT)0;\n"
					"	vout.pos		= vin.pos;\n"
					"	vout.color		= vin.color;\n"
					"	vout.texCoord	= vin.texCoord;\n"
					"	return vout;\n"
					"}\n"
					"float4 PSMain( VS_OUT pin ) : SV_TARGET\n"
					"{\n"
					"	return diffuseMap.Sample( diffuseMapSampler, pin.texCoord ) * pin.color;\n"
					"}\n"
					;
			}

			Single::Single( const std::wstring spriteFilename )
			{
				HRESULT hr = S_OK;
				ID3D11Device *pDevice = ::Donya::GetDevice();

				std::array<Single::Vertex, 4> vertices =
				{
					Single::Vertex{ XMFLOAT3( 0.0, 1.0, 0 ), XMFLOAT4( 1, 1, 0, 1 ), XMFLOAT2{ 0, 0 } },
					Single::Vertex{ XMFLOAT3( 1.0, 1.0, 0 ), XMFLOAT4( 1, 0, 0, 1 ), XMFLOAT2{ 0, 0 } },
					Single::Vertex{ XMFLOAT3( 0.0, 0.0, 0 ), XMFLOAT4( 0, 1, 0, 1 ), XMFLOAT2{ 0, 0 } },
					Single::Vertex{ XMFLOAT3( 1.0, 0.0, 0 ), XMFLOAT4( 0, 0, 1, 1 ), XMFLOAT2{ 0, 0 } }
				};

				// Create VertexBuffer
				{
					D3D11_BUFFER_DESC d3dBufferDesc{};
					d3dBufferDesc.ByteWidth = sizeof( Vertex ) * vertices.size();
					d3dBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
					d3dBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
					d3dBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
					d3dBufferDesc.MiscFlags = 0;
					d3dBufferDesc.StructureByteStride = 0;

					D3D11_SUBRESOURCE_DATA d3dSubResourceData{};
					d3dSubResourceData.pSysMem = vertices.data();
					d3dSubResourceData.SysMemPitch = 0;
					d3dSubResourceData.SysMemSlicePitch = 0;

					hr = pDevice->CreateBuffer( &d3dBufferDesc, &d3dSubResourceData, d3dVertexBuffer.GetAddressOf() );
					_ASSERT_EXPR( SUCCEEDED( hr ), _TEXT( "Failed : CreateBuffer()" ) );
				}
				// Create VertexShader and InputLayout
				{
					D3D11_INPUT_ELEMENT_DESC d3dInputElementsDesc[] =
					{
						{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,		0, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_VERTEX_DATA, 0 },
						{ "COLOR"	, 0, DXGI_FORMAT_R32G32B32A32_FLOAT,	0, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_VERTEX_DATA, 0 },
						{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,			0, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_VERTEX_DATA, 0 },
					};

					Resource::CreateVertexShaderFromSource
					(
						pDevice,
						"SpriteVS",
						SpriteShaderSource(),
						"VSMain",
						d3dVertexShader.GetAddressOf(),
						d3dInputLayout.GetAddressOf(),
						d3dInputElementsDesc,
						_countof( d3dInputElementsDesc )
					);
				}
				// Create PixelShader
				{
					Resource::CreatePixelShaderFromSource
					(
						pDevice,
						"SpritePS",
						SpriteShaderSource(),
						"PSMain",
						d3dPixelShader.GetAddressOf()
					);
				}
				// Create RasterizerState
				{
					D3D11_RASTERIZER_DESC d3dResterizerDesc = Shared::SpriteRasterizerDesc();

					hr = pDevice->CreateRasterizerState( &d3dResterizerDesc, d3dRasterizerState.GetAddressOf() );
					_ASSERT_EXPR( SUCCEEDED( hr ), _TEXT( "Failed : CreateRasterizerState()" ) );
				}
				// Create DepthsStencilState
				{
					D3D11_DEPTH_STENCIL_DESC d3dDepthStencilDesc = Shared::SpriteDepthStencilDesc();

					hr = pDevice->CreateDepthStencilState
					(
						&d3dDepthStencilDesc,
						d3dDepthStencilState.GetAddressOf()
					);
					_ASSERT_EXPR( SUCCEEDED( hr ), "Failed : CreateDepthStencilState" );
				}
				// Read Texture
				{
					D3D11_SAMPLER_DESC d3dSamplerDesc = Shared::SpriteSamplerDesc();

					Resource::CreateSamplerState
					(
						pDevice,
						&d3dSamplerState,
						d3dSamplerDesc
					);

					Resource::CreateTexture2DFromFile
					(
						pDevice,
						spriteFilename,
						d3dShaderResourceView.GetAddressOf(),
						&d3dTexture2DDesc,
						&d3dSamplerDesc
					);
				}
			}
			Single::~Single()
			{

			}

			void Single::GetTextureSize( int *width, int *height )
			{
				if ( width ) { *width = d3dTexture2DDesc.Width; }
				if ( height ) { *height = d3dTexture2DDesc.Height; }
			}
			void Single::GetTextureSize( float *width, float *height )
			{
				if ( width ) { *width = scast<float>( d3dTexture2DDesc.Width ); }
				if ( height ) { *height = scast<float>( d3dTexture2DDesc.Height ); }
			}

			void ConvertNDCVertices( std::array<Single::Vertex, 4> * pOutputNDC, int wholeTextureWidth, int wholeTextureHeight, float scrX, float scrY, float scrW, float scrH, float texX, float texY, float texW, float texH, float degree, float rotX, float rotY )
			{
				constexpr size_t VERTICES_SIZE = 4;

				// Set each sprite's vertices coordinate to screen space
				constexpr XMFLOAT4 nullColor{ NULL, NULL, NULL, NULL };
				std::array<Single::Vertex, VERTICES_SIZE> screenVertices =
				{
					/* LT */ Single::Vertex{ XMFLOAT3{ scrX,		scrY,		 0 }, nullColor, XMFLOAT2{ texX,		texY		} },
					/* RT */ Single::Vertex{ XMFLOAT3{ scrX + scrW,	scrY,		 0 }, nullColor, XMFLOAT2{ texX + texW,	texY		} },
					/* LB */ Single::Vertex{ XMFLOAT3{ scrX,		scrY + scrH, 0 }, nullColor, XMFLOAT2{ texX,		texY + texH	} },
					/* RB */ Single::Vertex{ XMFLOAT3{ scrX + scrW,	scrY + scrH, 0 }, nullColor, XMFLOAT2{ texX + texW,	texY + texH	} },
				};

				// Translate sprite's centre to origin ( rotate centre )
				XMFLOAT2 distFromOrigin{ scrX + rotX, scrY + rotY };

				for ( size_t i = 0; i < VERTICES_SIZE; ++i )
				{
					screenVertices[i].pos.x -= distFromOrigin.x;
					screenVertices[i].pos.y -= distFromOrigin.y;
				}

				// Rotate each sprite's vertices by angle
				{
					const float radian = ToRadian( degree );
					const float cos = cosf( radian );
					const float sin = sinf( radian );

					float rx = 0, ry = 0;	// relative coordinate
					for ( size_t i = 0; i < VERTICES_SIZE; ++i )
					{
						rx = screenVertices[i].pos.x;
						ry = screenVertices[i].pos.y;

						screenVertices[i].pos.x = ( rx * cos ) - ( ry * sin );
						screenVertices[i].pos.y = ( ry * cos ) + ( rx * sin );
					}
				}

				// Translate sprite's centre to original position
				for ( size_t i = 0; i < VERTICES_SIZE; ++i )
				{
					screenVertices[i].pos.x += distFromOrigin.x;
					screenVertices[i].pos.y += distFromOrigin.y;
				}

				// Convert to NDC space
				{
					/*
					2/xMax-xMin     0               0            -((xMax+xMin)/(xMax-xMin))
					0               2/yMax-yMin     0            -((yMax+yMin)/(yMax-yMin))
					0               0               1/zMax-zMin  -(    zMin   /(zMax-zMin))
					0               0               0            1
					*/
					float xMax = ::Donya::Private::RegisteredScreenWidthF(), xMin = -1;
					float yMax = ::Donya::Private::RegisteredScreenHeightF(), yMin = -1;

					for ( size_t i = 0; i < VERTICES_SIZE; ++i )
					{
						( *pOutputNDC )[i].pos.x = ( ( 2.0f * screenVertices[i].pos.x ) / ( xMax ) ) - 1.0f;
						( *pOutputNDC )[i].pos.y = 1.0f - ( ( 2.0f * screenVertices[i].pos.y ) / ( yMax ) );

						( *pOutputNDC )[i].texCoord.x = screenVertices[i].texCoord.x / wholeTextureWidth;
						( *pOutputNDC )[i].texCoord.y = screenVertices[i].texCoord.y / wholeTextureHeight;
					}
				}

				// Apply drawing depth.
				{
					const float scrZ = GetDrawDepth();
					for ( size_t i = 0; i < VERTICES_SIZE; ++i )
					{
						( *pOutputNDC )[i].pos.z = scrZ;
					}
				}
			}
			void Single::RenderExt( float scrX, float scrY, float scrW, float scrH, float texX, float texY, float texW, float texH, float degree, float rotX, float rotY, float R, float G, float B, float A )
			{
				HRESULT hr = S_OK;
				ID3D11DeviceContext *pImmediateContext = ::Donya::GetImmediateContext();

				// Set rotation center to origin.
				// If don't set, origin is fixed to left-top.
				scrX -= rotX;
				scrY -= rotY;

				if ( ::Donya::ScreenShake::GetEnableState() )
				{
					scrX -= ::Donya::ScreenShake::GetX();
					scrY -= ::Donya::ScreenShake::GetY();
				}

				std::array<Single::Vertex, 4> NDCVertices =
				{
					/* LT */ Single::Vertex{ XMFLOAT3{ 0, 0, 0 }, XMFLOAT4{ R, G, B, Donya::Color::FilteringAlpha( A ) }, DirectX::XMFLOAT2{ 0, 0 } },
					/* RT */ Single::Vertex{ XMFLOAT3{ 0, 0, 0 }, XMFLOAT4{ R, G, B, Donya::Color::FilteringAlpha( A ) }, DirectX::XMFLOAT2{ 0, 0 } },
					/* LB */ Single::Vertex{ XMFLOAT3{ 0, 0, 0 }, XMFLOAT4{ R, G, B, Donya::Color::FilteringAlpha( A ) }, DirectX::XMFLOAT2{ 0, 0 } },
					/* RB */ Single::Vertex{ XMFLOAT3{ 0, 0, 0 }, XMFLOAT4{ R, G, B, Donya::Color::FilteringAlpha( A ) }, DirectX::XMFLOAT2{ 0, 0 } },
				};

				ConvertNDCVertices
				(
					&NDCVertices,
					d3dTexture2DDesc.Width,
					d3dTexture2DDesc.Height,
					scrX, scrY, scrW, scrH,
					texX, texY, texW, texH,
					degree, rotX, rotY
				);

				// Map, Unmap
				{
					D3D11_MAPPED_SUBRESOURCE d3d11MappedSubresource{};
					ZeroMemory( &d3d11MappedSubresource, sizeof( D3D11_MAPPED_SUBRESOURCE ) );

					hr = pImmediateContext->Map( d3dVertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &d3d11MappedSubresource );
					_ASSERT_EXPR( SUCCEEDED( hr ), _TEXT( "Failed : d3dDeviceContext->Map()" ) );

					memcpy_s( d3d11MappedSubresource.pData, sizeof( Single::Vertex ) * NDCVertices.size(), &NDCVertices, d3d11MappedSubresource.RowPitch );

					pImmediateContext->Unmap( d3dVertexBuffer.Get(), 0 );
				}

				Microsoft::WRL::ComPtr<ID3D11RasterizerState>	prevRasterizerState;
				Microsoft::WRL::ComPtr<ID3D11DepthStencilState>	prevDepthStencilState;
				Microsoft::WRL::ComPtr<ID3D11SamplerState>		prevSamplerState;
				{
					pImmediateContext->RSGetState( prevRasterizerState.ReleaseAndGetAddressOf() );
					pImmediateContext->PSGetSamplers( 0, 1, prevSamplerState.ReleaseAndGetAddressOf() );
					pImmediateContext->OMGetDepthStencilState( prevDepthStencilState.ReleaseAndGetAddressOf(), 0 );
				}

				// Setting
				{
					/*
					IA...InputAssembler
					VS...VertexShader
					HS...HullShader
					DS...DomainShader
					GS...GeometryShader
					SO...StreamOuotput
					RS...RasterizerState
					PS...PixelShader
					OM...OutputMerger

					CS...ComputeShader
					*/

					UINT stride = sizeof( Single::Vertex );
					UINT offset = 0;
					pImmediateContext->IASetInputLayout( d3dInputLayout.Get() );
					pImmediateContext->IASetVertexBuffers( 0, 1, d3dVertexBuffer.GetAddressOf(), &stride, &offset );
					pImmediateContext->IASetPrimitiveTopology( D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP );

					pImmediateContext->VSSetShader( d3dVertexShader.Get(), nullptr, 0 );

					pImmediateContext->RSSetState( d3dRasterizerState.Get() );

					pImmediateContext->PSSetShader( d3dPixelShader.Get(), nullptr, 0 );
					pImmediateContext->PSSetShaderResources( 0, 1, d3dShaderResourceView.GetAddressOf() );
					pImmediateContext->PSSetSamplers( 0, 1, d3dSamplerState.GetAddressOf() );

					pImmediateContext->OMSetDepthStencilState( d3dDepthStencilState.Get(), 0xffffffff );
				}

				pImmediateContext->Draw( NDCVertices.size(), 0 );

				// PostProcessing
				{
					ID3D11ShaderResourceView *NullSRV = nullptr;

					pImmediateContext->IASetInputLayout( 0 );

					pImmediateContext->VSSetShader( 0, 0, 0 );

					pImmediateContext->RSSetState( prevRasterizerState.Get() );

					pImmediateContext->PSSetShader( 0, 0, 0 );
					pImmediateContext->PSSetShaderResources( 0, 1, &NullSRV );
					pImmediateContext->PSSetSamplers( 0, 1, prevSamplerState.GetAddressOf() );

					pImmediateContext->OMSetDepthStencilState( prevDepthStencilState.Get(), 1 );
				}
			}
			void Single::Render( float scrX, float scrY, float scrW, float scrH, float degree, float R, float G, float B, float A )
			{
				float texW = 0, texH = 0;
				GetTextureSize( &texW, &texH );

				RenderExt
				(
					scrX, scrY,
					scrW, scrH,
					0.0f, 0.0f,
					texW, texH,
					degree,
					scrW * 0.5f, scrH * 0.5f,
					R, G, B, A
				);
			}
			void Single::Render( float scrX, float scrY, float degree )
			{
				float texW = 0, texH = 0;
				GetTextureSize( &texW, &texH );

				Render
				(
					scrX, scrY,
					texW, texH,
					degree,
					1.0f, 1.0f, 1.0f, 1.0f
				);
			}

		#pragma endregion

		}
	}
}
