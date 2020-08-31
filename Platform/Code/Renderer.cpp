#include "Renderer.h"

#include "Donya/RenderingStates.h"
#include "Donya/Template.h"			// Use AppendVector()

namespace Config
{
	// These configurations are hard-coded by programmer

	enum ConstantName
	{
		Scene = 0,
		Model,
		Mesh,
		Subset,

		Shadow,
		PointLight,

		ConstantCount
	};
	enum TextureName
	{
		DiffuseMap = 0,
		ShadowMap,

		TextureCount
	};
	enum SamplerSlot : unsigned int
	{
		SamplerModel = 0,
		SamplerShadow,
	};

	using RegisterDesc = Donya::Model::RegisterDesc;
	static constexpr RegisterDesc constants[ConstantName::ConstantCount]
	{
		/* Scene	*/	RegisterDesc::Make( 0, /* setVS = */ true,	/* setPS = */ true	),
		/* Model	*/	RegisterDesc::Make( 1, /* setVS = */ true,	/* setPS = */ true	),
		/* Mesh		*/	RegisterDesc::Make( 2, /* setVS = */ true,	/* setPS = */ false	),
		/* Subset	*/	RegisterDesc::Make( 3, /* setVS = */ false,	/* setPS = */ true	),

		/* Shadow	*/	RegisterDesc::Make( 4, /* setVS = */ true,	/* setPS = */ true	),
		/* PointLight*/	RegisterDesc::Make( 5, /* setVS = */ false,	/* setPS = */ true	),
	};
	static constexpr RegisterDesc textures[TextureName::TextureCount]
	{
		/* DiffuseMap	*/	RegisterDesc::Make( 0, /* setVS = */ false,	/* setPS = */ true	),
		/* ShadowMap	*/	RegisterDesc::Make( 1, /* setVS = */ false,	/* setPS = */ true	),
	};
}

bool RenderingHelper::CBuffer::Create()
{
	bool succeeded = true;
	if ( !scene		.Create() ) { succeeded = false; }
	if ( !pointLight.Create() ) { succeeded = false; }
	if ( !model		.Create() ) { succeeded = false; }
	if ( !shadow	.Create() ) { succeeded = false; }
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
	constexpr const char *normalVSFilePathStatic	= "./Data/Shaders/ModelStaticVS.cso";
	constexpr const char *normalVSFilePathSkinning	= "./Data/Shaders/ModelSkinningVS.cso";
	constexpr const char *normalPSFilePath			= "./Data/Shaders/ModelPS.cso";
	constexpr const char *shadowVSFilePathStatic	= "./Data/Shaders/CastShadowModelStaticVS.cso";
	constexpr const char *shadowVSFilePathSkinning	= "./Data/Shaders/CastShadowModelSkinningVS.cso";
	constexpr const char *shadowPSFilePath			= "./Data/Shaders/CastShadowModelPS.cso";
	constexpr auto IEDescsPos	= Donya::Model::Vertex::Pos::GenerateInputElements( 0 );
	constexpr auto IEDescsTex	= Donya::Model::Vertex::Tex::GenerateInputElements( 1 );
	constexpr auto IEDescsBone	= Donya::Model::Vertex::Bone::GenerateInputElements( 2 );

	std::vector<D3D11_INPUT_ELEMENT_DESC> IEDescsStatic{};
	Donya::AppendVector( &IEDescsStatic, IEDescsPos );
	Donya::AppendVector( &IEDescsStatic, IEDescsTex );
	std::vector<D3D11_INPUT_ELEMENT_DESC> IEDescsSkinning{ IEDescsStatic };
	Donya::AppendVector( &IEDescsSkinning, IEDescsBone );

	bool succeeded = true;
	if ( !normalStatic	.Create( IEDescsStatic,		normalVSFilePathStatic,		normalPSFilePath ) ) { succeeded = false; }
	if ( !normalSkinning.Create( IEDescsSkinning,	normalVSFilePathSkinning,	normalPSFilePath ) ) { succeeded = false; }
	if ( !shadowStatic	.Create( IEDescsStatic,		shadowVSFilePathStatic,		shadowPSFilePath ) ) { succeeded = false; }
	if ( !shadowSkinning.Create( IEDescsSkinning,	shadowVSFilePathSkinning,	shadowPSFilePath ) ) { succeeded = false; }
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

	if ( !pCBuffer->Create() )		{ succeeded = false; }
	if ( !pRenderer->Create() )		{ succeeded = false; }
	if ( !pShader->Create() )		{ succeeded = false; }
	if ( !pPrimitive->Create() )	{ succeeded = false; }

	if ( succeeded ) { wasCreated = true; }
	return succeeded;
}

void RenderingHelper::UpdateConstant( const Donya::Model::Constants::PerScene::Common &constant )
{
	pCBuffer->scene.data = constant;
}
void RenderingHelper::UpdateConstant( const Donya::Model::Constants::PerScene::PointLightRoom &constant )
{
	pCBuffer->pointLight.data = constant;
}
void RenderingHelper::UpdateConstant( const Donya::Model::Constants::PerModel::Common &constant )
{
	pCBuffer->model.data = constant;
}
void RenderingHelper::UpdateConstant( const ShadowConstant &constant )
{
	pCBuffer->shadow.data = constant;
}
void RenderingHelper::UpdateConstant( const Donya::Model::Cube::Constant	&constant )
{
	pPrimitive->rendererCube.UpdateConstant( constant );
}
void RenderingHelper::UpdateConstant( const Donya::Model::Sphere::Constant	&constant )
{
	pPrimitive->rendererSphere.UpdateConstant( constant );
}
void RenderingHelper::ActivateConstantScene()
{
	constexpr auto desc = Config::constants[Config::Scene];
	pCBuffer->scene.Activate( desc.setSlot, desc.setVS, desc.setPS );
}
void RenderingHelper::ActivateConstantPointLight()
{
	constexpr auto desc = Config::constants[Config::PointLight];
	pCBuffer->pointLight.Activate( desc.setSlot, desc.setVS, desc.setPS );
}
void RenderingHelper::ActivateConstantModel()
{
	constexpr auto desc = Config::constants[Config::Model];
	pCBuffer->model.Activate( desc.setSlot, desc.setVS, desc.setPS );
}
void RenderingHelper::ActivateConstantShadow()
{
	constexpr auto desc = Config::constants[Config::Shadow];
	pCBuffer->shadow.Activate( desc.setSlot, desc.setVS, desc.setPS );
}
void RenderingHelper::ActivateConstantCube()
{
	pPrimitive->rendererCube.ActivateConstant();
}
void RenderingHelper::ActivateConstantSphere()
{
	pPrimitive->rendererSphere.ActivateConstant();
}
void RenderingHelper::DeactivateConstantScene()
{
	pCBuffer->scene.Deactivate();
}
void RenderingHelper::DeactivateConstantPointLight()
{
	pCBuffer->pointLight.Deactivate();
}
void RenderingHelper::DeactivateConstantModel()
{
	pCBuffer->model.Deactivate();
}
void RenderingHelper::DeactivateConstantShadow()
{
	pCBuffer->shadow.Deactivate();
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
void RenderingHelper::ActivateShaderShadowStatic()
{
	pShader->shadowStatic.Activate();
}
void RenderingHelper::ActivateShaderShadowSkinning()
{
	pShader->shadowSkinning.Activate();
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
void RenderingHelper::DeactivateShaderShadowStatic()
{
	pShader->shadowStatic.Deactivate();
}
void RenderingHelper::DeactivateShaderShadowSkinning()
{
	pShader->shadowSkinning.Deactivate();
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

void RenderingHelper::ActivateDepthStencilCube()
{
	pPrimitive->rendererCube.ActivateDepthStencil();
}
void RenderingHelper::ActivateDepthStencilSphere()
{
	pPrimitive->rendererSphere.ActivateDepthStencil();
}
void RenderingHelper::ActivateRasterizerCube()
{
	pPrimitive->rendererCube.ActivateRasterizer();
}
void RenderingHelper::ActivateRasterizerSphere()
{
	pPrimitive->rendererSphere.ActivateRasterizer();
}
void RenderingHelper::ActivateSamplerModel( int identifier )
{
	Donya::Sampler::SetPS( identifier, Config::SamplerModel );
}
void RenderingHelper::ActivateSamplerShadow( int identifier )
{
	Donya::Sampler::SetPS( identifier, Config::SamplerShadow );
}
void RenderingHelper::DeactivateDepthStencilCube()
{
	pPrimitive->rendererCube.DeactivateDepthStencil();
}
void RenderingHelper::DeactivateDepthStencilSphere()
{
	pPrimitive->rendererSphere.DeactivateDepthStencil();
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
	Donya::Sampler::ResetPS( Config::SamplerModel );
}
void RenderingHelper::DeactivateSamplerShadow()
{
	Donya::Sampler::ResetPS( Config::SamplerShadow );
}

void RenderingHelper::ActivateShadowMap( const Donya::Surface &shadowMap )
{
	constexpr auto desc = Config::textures[Config::ShadowMap];
	shadowMap.SetDepthStencilShaderResourcePS( desc.setSlot );
}
void RenderingHelper::DeactivateShadowMap( const Donya::Surface &shadowMap )
{
	constexpr auto desc = Config::textures[Config::ShadowMap];
	shadowMap.ResetShaderResourcePS( desc.setSlot );
}

void RenderingHelper::Render( const Donya::Model::StaticModel	&model, const Donya::Model::Pose &pose )
{
	pRenderer->pStatic->Render
	(
		model,
		pose,
		Config::constants[Config::Mesh],
		Config::constants[Config::Subset],
		Config::textures[Config::DiffuseMap]
	);
}
void RenderingHelper::Render( const Donya::Model::SkinningModel	&model, const Donya::Model::Pose &pose )
{
	pRenderer->pSkinning->Render
	(
		model,
		pose,
		Config::constants[Config::Mesh],
		Config::constants[Config::Subset],
		Config::textures[Config::DiffuseMap]
	);
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
