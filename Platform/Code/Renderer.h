#pragma once

#include <memory>
#include <string>
#include <vector>

#include "Donya/Shader.h"
#include "Donya/Surface.h"
#include "Donya/CBuffer.h"
#include "Donya/Model.h"
#include "Donya/ModelCommon.h"
#include "Donya/ModelPose.h"
#include "Donya/ModelPrimitive.h"
#include "Donya/ModelRenderer.h"

class RenderingHelper
{
public:
	struct ShadowConstant
	{
		Donya::Vector4x4	lightProjMatrix;	// The view-projection matrix that was used to making shadow map. The view-projection of light-source.
		Donya::Vector3		shadowColor;		// RGB
		float				shadowBias = 0.03f;	// Used for ease a shadow acne
	};
private:
	struct CBuffer
	{
		Donya::CBuffer<Donya::Model::Constants::PerScene::Common>
			scene;
		Donya::CBuffer<Donya::Model::Constants::PerScene::PointLightRoom>
			pointLight;
		Donya::CBuffer<Donya::Model::Constants::PerModel::Common>
			model;
		Donya::CBuffer<ShadowConstant>
			shadow;
	public:
		bool Create();
	};
	struct PrimitiveSet
	{
		Donya::Model::Cube				modelCube;
		Donya::Model::CubeRenderer		rendererCube;
		Donya::Model::Sphere			modelSphere;
		Donya::Model::SphereRenderer	rendererSphere;
	public:
		bool Create();
	};
	struct Renderer
	{
		std::unique_ptr<Donya::Model::StaticRenderer>	pStatic;
		std::unique_ptr<Donya::Model::SkinningRenderer>	pSkinning;
	public:
		bool Create();
	};
	struct Shader
	{
		Donya::VertexShader	VS;
		Donya::PixelShader	PS;
	public:
		bool Create( const std::vector<D3D11_INPUT_ELEMENT_DESC> &IEDescs, const std::string &fileNameVS, const std::string &fileNamePS );
	public:
		void Activate();
		void Deactivate();
	};
	struct ShaderSet
	{
		Shader	normalStatic;
		Shader	normalSkinning;
		Shader	shadowStatic;
		Shader	shadowSkinning;
	public:
		bool Create();
	};
private:
	std::unique_ptr<CBuffer>		pCBuffer;
	std::unique_ptr<ShaderSet>		pShader;
	std::unique_ptr<Renderer>		pRenderer;
	std::unique_ptr<PrimitiveSet>	pPrimitive;
	bool wasCreated = false;
public:
	bool Init();
public:
	void UpdateConstant( const Donya::Model::Constants::PerScene::Common &constant );
	void UpdateConstant( const Donya::Model::Constants::PerScene::PointLightRoom &constant );
	void UpdateConstant( const Donya::Model::Constants::PerModel::Common &constant );
	void UpdateConstant( const ShadowConstant &constant );
	void UpdateConstant( const Donya::Model::Cube::Constant		&constant );	// For primitive.
	void UpdateConstant( const Donya::Model::Sphere::Constant	&constant );	// For primitive.
	void ActivateConstantScene();
	void ActivateConstantPointLight();
	void ActivateConstantModel();
	void ActivateConstantShadow();
	void ActivateConstantCube();	// For primitive.
	void ActivateConstantSphere();	// For primitive.
	void DeactivateConstantScene();
	void DeactivateConstantPointLight();
	void DeactivateConstantModel();
	void DeactivateConstantShadow();
	void DeactivateConstantCube();	// For primitive.
	void DeactivateConstantSphere();// For primitive.
public:
	void ActivateShaderNormalStatic();
	void ActivateShaderNormalSkinning();
	void ActivateShaderShadowStatic();
	void ActivateShaderShadowSkinning();
	void ActivateShaderCube();		// For primitive.
	void ActivateShaderSphere();	// For primitive.
	void DeactivateShaderNormalStatic();
	void DeactivateShaderNormalSkinning();
	void DeactivateShaderShadowStatic();
	void DeactivateShaderShadowSkinning();
	void DeactivateShaderCube();	// For primitive.
	void DeactivateShaderSphere();	// For primitive.
public:
	void ActivateDepthStencilCube();
	void ActivateDepthStencilSphere();
	void ActivateRasterizerCube();
	void ActivateRasterizerSphere();
	void ActivateSamplerModel( int samplerIdentifier );
	void ActivateSamplerShadow( int samplerIdentifier );
	void DeactivateDepthStencilCube();
	void DeactivateDepthStencilSphere();
	void DeactivateRasterizerCube();
	void DeactivateRasterizerSphere();
	void DeactivateSamplerModel();
	void DeactivateSamplerShadow();
public:
	void ActivateShadowMap( const Donya::Surface &shadowMap );
	void DeactivateShadowMap( const Donya::Surface &shadowMap );
public:
	void Render( const Donya::Model::StaticModel	&model, const Donya::Model::Pose &pose );
	void Render( const Donya::Model::SkinningModel	&model, const Donya::Model::Pose &pose );
public:
	/// <summary>
	/// Call the draw method of a Cube only.
	/// </summary>
	void CallDrawCube();
	/// <summary>
	/// Call the draw method of a Sphere only.
	/// </summary>
	void CallDrawSphere();
	/// <summary>
	/// Set(and will reset after the draw) the vertex buffer, index buffer and primitive topology. Then call the draw method of a Cube.
	/// </summary>
	void DrawCube();
	/// <summary>
	/// Set(and will reset after the draw) the vertex buffer, index buffer and primitive topology. Then call the draw method of a Sphere.
	/// </summary>
	void DrawSphere();
	/// <summary>
	/// The wrapper of the process that drawing a Cube.<para></para>
	/// Doing the set and reset of: Shader(VS, PS), State(DS, RS), CBuffer.
	/// </summary>
	void ProcessDrawingCube( const Donya::Model::Cube::Constant &constant );
	/// <summary>
	/// The wrapper of the process that drawing a Sphere.<para></para>
	/// Doing the set and reset of: Shader(VS, PS), State(DS, RS), CBuffer.
	/// </summary>
	void ProcessDrawingSphere( const Donya::Model::Sphere::Constant &constant );
};
