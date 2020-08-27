#pragma once

#include <array>
#include <vector>

#include "Donya/CBuffer.h"
#include "Donya/Displayer.h"
#include "Donya/Serializer.h"
#include "Donya/Shader.h"
#include "Donya/Surface.h"
#include "Donya/UseImGui.h"
#include "Donya/Vector.h"

class BloomApplier
{
public:
	struct Parameter
	{
		static constexpr int maxBlurSampleCount = 16;
	public:
		float	luminanceThreshold	= 1.0f;						// The applier adopts the luminance of greater than this
		float	blurRange			= 5.0f;						// The bluring range, [0.0f < blurRange]
		int		blurSampleCount		= maxBlurSampleCount >> 1;	// The loop count of bluring, [0 < blurSampleCount]
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive
			(
				CEREAL_NVP( luminanceThreshold	),
				CEREAL_NVP( blurRange			),
				CEREAL_NVP( blurSampleCount		)
			);

			if ( 1 <= version )
			{
				// archive( CEREAL_NVP( x ) );
			}
		}
	public:
	#if USE_IMGUI
		void ShowImGuiNode( const std::string &nodeCaption );
	#endif // USE_IMGUI
	};
private:
	struct HighLuminanceConstant
	{
		float			threshold = 1.0f; // Adopt luminance is greater equal than it
		Donya::Vector3	paddings;
	};
	struct BlurParam
	{
	public:
		Donya::Vector2	uvOffset;
		float			distribution	= 1.0f; // Weight
		float			_paddings{};
	};
	struct BlurConstant
	{
		std::array<BlurParam, Parameter::maxBlurSampleCount> params;
		int				sampleCount = Parameter::maxBlurSampleCount >> 1;
		Donya::Int3		_paddings{};
	};
private: // Serialize member
	Parameter parameter;
private:
	static constexpr size_t		blurBufferCount  = 6;
	static constexpr auto		blurBufferFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
	std::vector<Donya::Surface>	blurBuffers;			// PingPong buffer

	Donya::Int2 baseSurfaceSize{ 1920, 1080 };			// Whole size
	Donya::Int2 applicationScreenSize{ 1920, 1080 };	// Whole size

	Donya::Displayer	display;
	Donya::Surface		highLuminanceSurface;

	Donya::VertexShader VS;
	Donya::PixelShader  PSHighLuminance;
	Donya::PixelShader  PSBlur;
	Donya::PixelShader  PSCombine;

	Donya::CBuffer<HighLuminanceConstant>	cbHighLuminance;
	Donya::CBuffer<BlurConstant>			cbBlur;

	bool wasInitialized = false;
private:
	friend class cereal::access;
	template<class Archive>
	void serialize( Archive &archive, std::uint32_t version )
	{
		archive( CEREAL_NVP( parameter ) );

		if ( 1 <= version )
		{
			// archive( CEREAL_NVP( x ) );
		}
	}
public:
	bool Init( const Donya::Int2 &wholeScreenSize );
	/// <summary>
	/// Set shader resource of render target to pixel shader
	/// </summary>
	void SetShaderResourcesPS( size_t startIndex );
	void ResetShaderResourcesPS( size_t startIndex );
public:
	void AssignParameter( const Parameter &parameter );
	void ClearBuffers( const Donya::Vector4 &clearColor );
	/// <summary>
	/// It changes current render target, viewport and shaders.
	/// </summary>
	void WriteLuminance( const Donya::Surface &sceneSurface );
	/// <summary>
	/// It changes current render target, viewport and shaders.
	/// </summary>
	void WriteBlur();
	/// <summary>
	/// It changes current blend-state, sampler state and shaders.<para></para>
	/// You can set zero to the "drawingSize". It will use a registered screen size in that case.
	/// </summary>
	void DrawBlurBuffersByAddBlend( const Donya::Vector2 &drawingSize = Donya::Vector2::Zero() );
private:
	float CalcGaussianWeight( const Donya::Vector2 &pos, float deviation );
	void  UpdateGaussianBlurParams( float bufferWholeWidth, float bufferWholeHeight, const Donya::Vector2 &unitBlurDirection, float multiply );
public:
#if USE_IMGUI
	void DrawHighLuminanceToImGui( const Donya::Vector2 &wholeDrawSize );
	void DrawBlurBuffersToImGui( const Donya::Vector2 &wholeDrawSize );
#endif // USE_IMGUI
};
CEREAL_CLASS_VERSION( BloomApplier::Parameter,	0 )
