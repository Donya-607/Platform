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
		bool							wasInitialized		= false;
	private:
		class Instance
		{
		private:
			Effekseer::Effect *pHandle = nullptr;
		public:
			Instance( Effekseer::Manager *pManager, const stdEfkString &filePath, float scaleWhenCreate = 1.0f, const stdEfkString &materialPath = {} );
			~Instance();
		public:
			bool IsValid() const;
			Effekseer::Effect *GetEffectOrNullptr();
		};
		std::unordered_map<stdEfkString, std::shared_ptr<Instance>> instances;
	private:
		Admin() = default;
	public:
		bool Init( ID3D11Device *pDevice, ID3D11DeviceContext *pContext );
		void Uninit();

		void Update( float updateSpeedMagnification = 1.0f );

		void Draw();
	public:
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
		bool LoadEffect( Effect::Kind effectAttribute );
		void UnloadEffect( Effect::Kind effectAttribute );
		void UnloadEffectAll();
	public:
		float GetEffectScale( Effect::Kind effectAttribute );
		/// <summary>
		/// Returns nullptr if specified effect was not loaded.
		/// </summary>
		Effekseer::Effect *GetEffectOrNullptr( Effect::Kind effectAttribute );
	public:
	#if USE_IMGUI
		void ShowImGuiNode( const std::string &nodeCaption );
	#endif // USE_IMGUI
	};
}
