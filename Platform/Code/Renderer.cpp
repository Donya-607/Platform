#include "Renderer.h"

#include "Donya/RenderingStates.h"

namespace
{
	static constexpr D3D11_DEPTH_STENCIL_DESC	DepthStencilDesc()
	{
		D3D11_DEPTH_STENCIL_DESC standard{};
		standard.DepthEnable			= TRUE;
		standard.DepthWriteMask			= D3D11_DEPTH_WRITE_MASK_ALL;
		standard.DepthFunc				= D3D11_COMPARISON_LESS;
		standard.StencilEnable			= FALSE;
		return standard;
	}
	static constexpr D3D11_RASTERIZER_DESC		RasterizerDesc()
	{
		D3D11_RASTERIZER_DESC standard{};
		standard.FillMode				= D3D11_FILL_SOLID;
		standard.CullMode				= D3D11_CULL_BACK;
		standard.FrontCounterClockwise	= TRUE;
		standard.DepthBias				= 0;
		standard.DepthBiasClamp			= 0;
		standard.SlopeScaledDepthBias	= 0;
		standard.DepthClipEnable		= TRUE;
		standard.ScissorEnable			= FALSE;
		standard.MultisampleEnable		= FALSE;
		standard.AntialiasedLineEnable	= TRUE;
		return standard;
	}
	static constexpr D3D11_SAMPLER_DESC			SamplerDesc()
	{
		D3D11_SAMPLER_DESC standard{};
		/*
		standard.MipLODBias		= 0;
		standard.MaxAnisotropy	= 16;
		*/
		standard.Filter					= D3D11_FILTER_ANISOTROPIC;
		standard.AddressU				= D3D11_TEXTURE_ADDRESS_WRAP;
		standard.AddressV				= D3D11_TEXTURE_ADDRESS_WRAP;
		standard.AddressW				= D3D11_TEXTURE_ADDRESS_WRAP;
		standard.ComparisonFunc			= D3D11_COMPARISON_ALWAYS;
		standard.MinLOD					= 0;
		standard.MaxLOD					= D3D11_FLOAT32_MAX;
		return standard;
	}

	// These settings are hard-coded by a programmer.

	static constexpr Donya::Model::RegisterDesc TransSetting()
	{
		return Donya::Model::RegisterDesc::Make( 4, /* setVS = */ false, /* setPS = */ true );
	}

	static constexpr Donya::Model::RegisterDesc SceneSetting()
	{
		return Donya::Model::RegisterDesc::Make( 0, /* setVS = */ true, /* setPS = */ true );
	}
	static constexpr Donya::Model::RegisterDesc ModelSetting()
	{
		return Donya::Model::RegisterDesc::Make( 1, /* setVS = */ true, /* setPS = */ true );
	}

	static constexpr Donya::Model::RegisterDesc MeshSetting()
	{
		return Donya::Model::RegisterDesc::Make( 2, /* setVS = */ true, /* setPS = */ false );
	}
	static constexpr Donya::Model::RegisterDesc SubsetSetting()
	{
		return Donya::Model::RegisterDesc::Make( 3, /* setVS = */ false, /* setPS = */ true );
	}

	static constexpr Donya::Model::RegisterDesc DiffuseMapSetting()
	{
		return Donya::Model::RegisterDesc::Make( 0, /* setVS = */ false, /* setPS = */ true );
	}
	static constexpr Donya::Model::RegisterDesc SamplerSetting()
	{
		return Donya::Model::RegisterDesc::Make( 0, /* setVS = */ false, /* setPS = */ true );
	}
}

bool RenderingHelper::CBuffer::Create()
{
	bool succeeded = true;
	if ( !trans.Create() ) { succeeded = false; }
	if ( !scene.Create() ) { succeeded = false; }
	if ( !model.Create() ) { succeeded = false; }
	return succeeded;
}

bool RenderingHelper::PrimitiveSet::Create()
{
	bool succeeded = true;
	if ( !modelCube.Create()		) { succeeded = false; }
	if ( !rendererCube.Create()		) { succeeded = false; }
	if ( !modelSphere.Create()		) { succeeded = false; }
	if ( !rendererSphere.Create()	) { succeeded = false; }
	return succeeded;
}

bool RenderingHelper::Renderer::Create()
{
	pStatic		= std::make_unique<Donya::Model::StaticRenderer>();
	pSkinning	= std::make_unique<Donya::Model::SkinningRenderer>();
	return true;
}

bool RenderingHelper::Shader::Create( const std::vector<D3D11_INPUT_ELEMENT_DESC> &IEDescs, const std::string &fileNameVS, const std::string &fileNamePS )
{
	bool succeeded = true;
	if ( !VS.CreateByCSO( fileNameVS, IEDescs ) ) { succeeded = false; }
	if ( !PS.CreateByCSO( fileNamePS          ) ) { succeeded = false; }
	return succeeded;
}
void RenderingHelper::Shader::Activate()
{
	VS.Activate();
	PS.Activate();
}
void RenderingHelper::Shader::Deactivate()
{
	VS.Deactivate();
	PS.Deactivate();
}

bool RenderingHelper::ShaderSet::Create()
{
	constexpr const char *VSFilePathStatic		= "./Data/Shaders/ModelStaticVS.cso";
	constexpr const char *VSFilePathSkinning	= "./Data/Shaders/ModelSkinningVS.cso";
	constexpr const char *PSFilePath			= "./Data/Shaders/ModelPS.cso";
	constexpr auto IEDescsPos	= Donya::Model::Vertex::Pos::GenerateInputElements( 0 );
	constexpr auto IEDescsTex	= Donya::Model::Vertex::Tex::GenerateInputElements( 1 );
	constexpr auto IEDescsBone	= Donya::Model::Vertex::Bone::GenerateInputElements( 2 );

	auto Append = []( auto &dest, const auto &source )
	{
		dest.insert( dest.end(), source.begin(), source.end() );
	};

	std::vector<D3D11_INPUT_ELEMENT_DESC> IEDescsStatic{};
	Append( IEDescsStatic, IEDescsPos );
	Append( IEDescsStatic, IEDescsTex );
	std::vector<D3D11_INPUT_ELEMENT_DESC> IEDescsSkinning{ IEDescsStatic };
	Append( IEDescsSkinning, IEDescsBone );

	bool succeeded = true;
	if ( !normalStatic.Create( IEDescsStatic, VSFilePathStatic, PSFilePath ) ) { succeeded = false; }
	if ( !normalSkinning.Create( IEDescsSkinning, VSFilePathSkinning, PSFilePath ) ) { succeeded = false; }
	return succeeded;
}

bool RenderingHelper::State::Create()
{
	using FindFunction = std::function<bool( int )>;
	auto  FindAvailableID = []( const FindFunction &IsAlreadyExists )
	{
		for ( int i = 0; i < INT_MAX; ++i )
		{
			if ( IsAlreadyExists( i ) ) { continue; }
			// else

			return i;
		}

		_ASSERT_EXPR( 0, L"Error : An available identifier was not found!" );
		return NULL;
	};

	bool succeeded = true;

	if ( DS == DEFAULT_ID )
	{
		DS =  FindAvailableID( Donya::DepthStencil::IsAlreadyExists );
		if ( !Donya::DepthStencil::CreateState( DS, DepthStencilDesc() ) ) { succeeded = false; }
	}
	if ( RS == DEFAULT_ID )
	{
		RS = FindAvailableID( Donya::Rasterizer::IsAlreadyExists );
		if ( !Donya::Rasterizer::CreateState( RS, RasterizerDesc() ) ) { succeeded = false; }
	}
	if ( PS == DEFAULT_ID )
	{
		PS = FindAvailableID( Donya::Sampler::IsAlreadyExists );
		if ( !Donya::Sampler::CreateState( PS, SamplerDesc() ) ) { succeeded = false; }
	}

	return succeeded;
}

bool RenderingHelper::Init()
{
	if ( wasCreated ) { return true; }
	// else

	bool succeeded = true;

	pCBuffer	= std::make_unique<CBuffer>();
	pShader		= std::make_unique<ShaderSet>();
	pRenderer	= std::make_unique<Renderer>();
	pPrimitive	= std::make_unique<PrimitiveSet>();

	if ( !state.Create() )			{ succeeded = false; }
	if ( !pCBuffer->Create() )		{ succeeded = false; }
	if ( !pRenderer->Create() )		{ succeeded = false; }
	if ( !pShader->Create() )		{ succeeded = false; }
	if ( !pPrimitive->Create() )	{ succeeded = false; }

	if ( succeeded ) { wasCreated = true; }
	return succeeded;
}

void RenderingHelper::UpdateConstant( const TransConstant &constant )
{
	pCBuffer->trans.data = constant;
}
void RenderingHelper::UpdateConstant( const Donya::Model::Constants::PerScene::Common &constant )
{
	pCBuffer->scene.data = constant;
}
void RenderingHelper::UpdateConstant( const Donya::Model::Constants::PerModel::Common &constant )
{
	pCBuffer->model.data = constant;
}
void RenderingHelper::UpdateConstant( const Donya::Model::Cube::Constant	&constant )
{
	pPrimitive->rendererCube.UpdateConstant( constant );
}
void RenderingHelper::UpdateConstant( const Donya::Model::Sphere::Constant	&constant )
{
	pPrimitive->rendererSphere.UpdateConstant( constant );
}
void RenderingHelper::ActivateConstantTrans()
{
	constexpr auto desc = TransSetting();
	pCBuffer->trans.Activate( desc.setSlot, desc.setVS, desc.setPS );
}
void RenderingHelper::ActivateConstantScene()
{
	constexpr auto desc = SceneSetting();
	pCBuffer->scene.Activate( desc.setSlot, desc.setVS, desc.setPS );
}
void RenderingHelper::ActivateConstantModel()
{
	constexpr auto desc = ModelSetting();
	pCBuffer->model.Activate( desc.setSlot, desc.setVS, desc.setPS );
}
void RenderingHelper::ActivateConstantCube()
{
	pPrimitive->rendererCube.ActivateConstant();
}
void RenderingHelper::ActivateConstantSphere()
{
	pPrimitive->rendererSphere.ActivateConstant();
}
void RenderingHelper::DeactivateConstantTrans()
{
	pCBuffer->trans.Deactivate();
}
void RenderingHelper::DeactivateConstantScene()
{
	pCBuffer->scene.Deactivate();
}
void RenderingHelper::DeactivateConstantModel()
{
	pCBuffer->model.Deactivate();
}
void RenderingHelper::DeactivateConstantCube()
{
	pPrimitive->rendererCube.DeactivateConstant();
}
void RenderingHelper::DeactivateConstantSphere()
{
	pPrimitive->rendererSphere.DeactivateConstant();
}

void RenderingHelper::ActivateShaderNormalStatic()
{
	pShader->normalStatic.Activate();
}
void RenderingHelper::ActivateShaderNormalSkinning()
{
	pShader->normalSkinning.Activate();
}
void RenderingHelper::ActivateShaderCube()
{
	pPrimitive->rendererCube.ActivateVertexShader();
	pPrimitive->rendererCube.ActivatePixelShader();
}
void RenderingHelper::ActivateShaderSphere()
{
	pPrimitive->rendererSphere.ActivateVertexShader();
	pPrimitive->rendererSphere.ActivatePixelShader();
}
void RenderingHelper::DeactivateShaderNormalStatic()
{
	pShader->normalStatic.Deactivate();
}
void RenderingHelper::DeactivateShaderNormalSkinning()
{
	pShader->normalSkinning.Deactivate();
}
void RenderingHelper::DeactivateShaderCube()
{
	pPrimitive->rendererCube.DeactivateVertexShader();
	pPrimitive->rendererCube.DeactivatePixelShader();
}
void RenderingHelper::DeactivateShaderSphere()
{
	pPrimitive->rendererSphere.DeactivateVertexShader();
	pPrimitive->rendererSphere.DeactivatePixelShader();
}

void RenderingHelper::ActivateDepthStencilModel()
{
	Donya::DepthStencil::Activate( state.DS );
}
void RenderingHelper::ActivateDepthStencilCube()
{
	pPrimitive->rendererCube.ActivateDepthStencil();
}
void RenderingHelper::ActivateDepthStencilSphere()
{
	pPrimitive->rendererSphere.ActivateDepthStencil();
}
void RenderingHelper::ActivateRasterizerModel()
{
	Donya::Rasterizer::Activate( state.RS );
}
void RenderingHelper::ActivateRasterizerCube()
{
	pPrimitive->rendererCube.ActivateRasterizer();
}
void RenderingHelper::ActivateRasterizerSphere()
{
	pPrimitive->rendererSphere.ActivateRasterizer();
}
void RenderingHelper::ActivateSamplerModel()
{
	const auto desc = SamplerSetting();
	Donya::Sampler::Activate( state.PS, desc.setSlot, desc.setVS, desc.setPS );
}
void RenderingHelper::DeactivateDepthStencilModel()
{
	Donya::DepthStencil::Deactivate();
}
void RenderingHelper::DeactivateDepthStencilCube()
{
	pPrimitive->rendererCube.DeactivateDepthStencil();
}
void RenderingHelper::DeactivateDepthStencilSphere()
{
	pPrimitive->rendererSphere.DeactivateDepthStencil();
}
void RenderingHelper::DeactivateRasterizerModel()
{
	Donya::Rasterizer::Deactivate();
}
void RenderingHelper::DeactivateRasterizerCube()
{
	pPrimitive->rendererCube.DeactivateRasterizer();
}
void RenderingHelper::DeactivateRasterizerSphere()
{
	pPrimitive->rendererSphere.DeactivateRasterizer();
}
void RenderingHelper::DeactivateSamplerModel()
{
	Donya::Sampler::Deactivate();
}

void RenderingHelper::Render( const Donya::Model::StaticModel	&model, const Donya::Model::Pose &pose )
{
	pRenderer->pStatic->Render( model, pose, MeshSetting(), SubsetSetting(), DiffuseMapSetting() );
}
void RenderingHelper::Render( const Donya::Model::SkinningModel	&model, const Donya::Model::Pose &pose )
{
	pRenderer->pSkinning->Render( model, pose, MeshSetting(), SubsetSetting(), DiffuseMapSetting() );
}

void RenderingHelper::CallDrawCube()
{
	pPrimitive->modelCube.CallDraw();
}
void RenderingHelper::CallDrawSphere()
{
	pPrimitive->modelSphere.CallDraw();
}

void RenderingHelper::DrawCube()
{
	pPrimitive->rendererCube.Draw( pPrimitive->modelCube );
}
void RenderingHelper::DrawSphere()
{
	pPrimitive->rendererSphere.Draw( pPrimitive->modelSphere );
}

namespace
{
	template<class PrimitiveRenderer, class Constant, typename DrawMethod>
	void ProcessDrawingImpl( PrimitiveRenderer &renderer, const Constant &constant, const DrawMethod &Draw )
	{
		renderer.ActivateVertexShader();
		renderer.ActivatePixelShader();
		renderer.ActivateDepthStencil();
		renderer.ActivateRasterizer();
		
		renderer.UpdateConstant( constant );
		renderer.ActivateConstant();

		Draw();
		
		renderer.DeactivateConstant();

		renderer.DeactivateRasterizer();
		renderer.DeactivateDepthStencil();
		renderer.DeactivatePixelShader();
		renderer.DeactivateVertexShader();
	}
}

void RenderingHelper::ProcessDrawingCube( const Donya::Model::Cube::Constant &constant )
{
	auto Draw = [&]() { DrawCube(); };
	ProcessDrawingImpl( pPrimitive->rendererCube, constant, Draw );
}
void RenderingHelper::ProcessDrawingSphere( const Donya::Model::Sphere::Constant &constant )
{
	auto Draw = [&]() { DrawSphere(); };
	ProcessDrawingImpl( pPrimitive->rendererSphere, constant, Draw );
}
