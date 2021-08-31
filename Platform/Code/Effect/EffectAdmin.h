#pragma once

#include <d3d11.h>
#include <memory>
#include <string>
#include <unordered_map>

#include "Effekseer.h"
#include "EffekseerRendererDX11.h"

#include "../Donya/Template.h"
#include "../Donya/UseImGui.h"	// Use USE_IMGUI macro
#include "../Donya/Vector.h"

#include "Effect.h"
#include "EffectKind.h"
#include "EffectUtil.h"

namespace Effect
{
	class Admin final : public Donya::Singleton<Admin>
	{
		friend Donya::Singleton<Admin>;
	private:
		static constexpr int			maxInstanceCount	= 4096;
		static constexpr int32_t		maxSpriteCount		= 8192;
	private:
		Effekseer::Manager				*pManager			= nullptr;
		EffekseerRendererDX11::Renderer	*pRenderer			= nullptr;
		float							updateSpeed			= 1.0f;
		bool							wasInitialized		= false;
	private:
		class Instance
		{
		private:
			Effekseer::Effect *pFx = nullptr;
		public:
			Instance( Effekseer::Manager *pManager, const stdEfkString &filePath, float scaleWhenCreate = 1.0f, const stdEfkString &materialPath = {} );
			~Instance();
		public:
			bool IsValid() const;
			Effekseer::Effect *GetEffectOrNullptr();
		};
		std::unordered_map<stdEfkString, std::shared_ptr<Instance>> instances; // The source effects
		std::vector<Effect::Handle> handles; // The instances of some effect
	private:
		Admin() = default;
	public:
		bool Init( ID3D11Device *pDevice, ID3D11DeviceContext *pContext );
		void Uninit();

		void Update( float deltaTime );

		void Draw();
	public:
		void SetUpdateSpeed( float updateFactor );
		void SetLightColorAmbient( const Donya::Vector4 &color );
		void SetLightColorAmbient( const Donya::Vector3 &color, float alpha = 1.0f );
		void SetLightColorDiffuse( const Donya::Vector4 &color );
		void SetLightColorDiffuse( const Donya::Vector3 &color, float alpha = 1.0f );
		void SetLightDirection( const Donya::Vector3 &unitDirection );
		void SetViewMatrix( const Donya::Vector4x4 &cameraMatrix );
		void SetProjectionMatrix( const Donya::Vector4x4 &projectionMatrix );
	public:
		Effekseer::Manager *GetManagerOrNullptr() const;
	public:
		void LoadParameter();
		/// <summary>
		/// Returns true if the load was succeeded, or specified effect was already has loaded.
		/// </summary>
		bool LoadEffect( Effect::Kind effectKind );
		void UnloadEffect( Effect::Kind effectKind );
		void UnloadEffectAll();
	public:
		/// <summary>
		/// Generate specify effect's instance into internal.
		/// </summary>
		void GenerateInstance( Kind effectKind, const Donya::Vector3 &position, int32_t startFrame = 0 );
		/// <summary>
		/// Copy the instance into internal.
		/// it is convenient when usage like detaching the local effect.
		/// </summary>
		void AddCopy( const Effect::Handle &instance );
		/// <summary>
		/// Clear all instances from internal.
		/// </summary>
		void ClearInstances();
	public:
		float GetEffectScale( Effect::Kind effectKind );
		/// <summary>
		/// Returns nullptr if specified effect was not loaded.
		/// </summary>
		Effekseer::Effect *GetEffectOrNullptr( Effect::Kind effectKind );
	public:
	#if USE_IMGUI
		void ShowImGuiNode( const std::string &nodeCaption );
	#endif // USE_IMGUI
	};
}
