#pragma once

#include <memory>
#include <string>
#include <vector>

#include "Donya/Shader.h"
#include "Donya/CBuffer.h"
#include "Donya/Model.h"
#include "Donya/ModelCommon.h"
#include "Donya/ModelPose.h"
#include "Donya/ModelPrimitive.h"
#include "Donya/ModelRenderer.h"

class RenderingHelper
{
public:
	struct TransConstant
	{
		float zNear				= 1.0f;
		float zFar				= 3.0f;
		float lowerAlpha		= 0.1f;
		float heightThreshold	= 0.0f;	// I don't wanna transparentize a pixel that under the threshold.
	};
private:
	struct CBuffer
	{
		Donya::CBuffer<TransConstant> trans;
		Donya::CBuffer<Donya::Model::Constants::PerScene::Common> scene;
		Donya::CBuffer<Donya::Model::Constants::PerModel::Common> model;
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
	public:
		bool Create();
	};
	struct State
	{
		static constexpr int DEFAULT_ID = -1;
		int DS	= DEFAULT_ID;
		int RS	= DEFAULT_ID;
		int PS	= DEFAULT_ID;
	public:
		bool Create();
	};
private:
	State							state;
	std::unique_ptr<CBuffer>		pCBuffer;
	std::unique_ptr<ShaderSet>		pShader;
	std::unique_ptr<Renderer>		pRenderer;
	std::unique_ptr<PrimitiveSet>	pPrimitive;
	bool wasCreated = false;
public:
	bool Init();
public:
	void UpdateConstant( const TransConstant &constant );
	void UpdateConstant( const Donya::Model::Constants::PerScene::Common &constant );
	void UpdateConstant( const Donya::Model::Constants::PerModel::Common &constant );
	void UpdateConstant( const Donya::Model::Cube::Constant		&constant );	// For primitive.
	void UpdateConstant( const Donya::Model::Sphere::Constant	&constant );	// For primitive.
	void ActivateConstantTrans();
	void ActivateConstantScene();
	void ActivateConstantModel();
	void ActivateConstantCube();	// For primitive.
	void ActivateConstantSphere();	// For primitive.
	void DeactivateConstantTrans();
	void DeactivateConstantScene();
	void DeactivateConstantModel();
	void DeactivateConstantCube();	// For primitive.
	void DeactivateConstantSphere();// For primitive.
public:
	void ActivateShaderNormalStatic();
	void ActivateShaderNormalSkinning();
	void ActivateShaderCube();		// For primitive.
	void ActivateShaderSphere();	// For primitive.
	void DeactivateShaderNormalStatic();
	void DeactivateShaderNormalSkinning();
	void DeactivateShaderCube();	// For primitive.
	void DeactivateShaderSphere();	// For primitive.
public:
	void ActivateDepthStencilModel();
	void ActivateDepthStencilCube();
	void ActivateDepthStencilSphere();
	void ActivateRasterizerModel();
	void ActivateRasterizerCube();
	void ActivateRasterizerSphere();
	void ActivateSamplerModel();
	void DeactivateDepthStencilModel();
	void DeactivateDepthStencilCube();
	void DeactivateDepthStencilSphere();
	void DeactivateRasterizerModel();
	void DeactivateRasterizerCube();
	void DeactivateRasterizerSphere();
	void DeactivateSamplerModel();
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
