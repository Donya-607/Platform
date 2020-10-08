#pragma once

#include <memory>

#include <d3d11.h>

#include "Effekseer.h"

#include "../Donya/Template.h"
#include "../Donya/Vector.h"

#include "EffectKind.h"

namespace Effect
{
	class EffectAdmin final : public Donya::Singleton<EffectAdmin>
	{
		friend Donya::Singleton<EffectAdmin>;
	private:
		struct Impl;
	public:
		std::unique_ptr<Impl> pImpl;
	private:
		EffectAdmin();
	public:
		~EffectAdmin();
	public:
		bool Init( ID3D11Device *pDevice, ID3D11DeviceContext *pImmediateContext );
		void Uninit();

		void Update( float updateSpeedMagnification = 1.0f );

		void Draw();
	public:
		void SetViewMatrix( const Donya::Vector4x4 &cameraMatrix );
		void SetProjectionMatrix( const Donya::Vector4x4 &projectionMatrix );
	public:
		Effekseer::Manager *GetManagerOrNullptr() const;
	public:
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
	};
}
