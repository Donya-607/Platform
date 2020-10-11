#include "Effect.h"

#include <d3d11.h>
#include <unordered_map>
#include <vector>

#include <cereal/types/vector.hpp>

#include "Effekseer.h"

#include "../Donya/Constant.h"		// Use scast macro.
#include "../Donya/Serializer.h"
#include "../Donya/Useful.h"
#include "../Donya/UseImGui.h"

#include "EffectAdmin.h"
#include "EffectKind.h"
#include "EffectParam.h"
#include "EffectUtil.h"
#include "../FilePath.h"
#include "../Parameter.h"

namespace Fx = Effekseer;

namespace
{
	Effect::Admin &GetAdmin() { return Effect::Admin::Get(); }
}

namespace Effect
{
	Handle Handle::Generate( Kind kind, const Donya::Vector3 &pos, int32_t startFrame )
	{
		auto &admin = GetAdmin();

		Fx::Manager	*pManager	= admin.GetManagerOrNullptr();
		if ( !pManager )
		{
			_ASSERT_EXPR( 0, L"Error: Effect manager is invalid!" );
			return Handle{ -1 };
		}
		// else

		Fx::Effect	*pEffect	= admin.GetEffectOrNullptr( kind );
		if ( !pEffect )
		{
			_ASSERT_EXPR( 0, L"Error: Effect is invalid! May you use wrong file name?" );
			return Handle{ -1 };
		}
		// else

		const Fx::Handle handle = pManager->Play( pEffect, ToFxVector( pos ), startFrame );
		if ( pManager->Exists( handle ) )
		{
			const float kindScale = admin.GetEffectScale( kind );
			// We should update this handle before SetScale().
			// The SetScale() will throws an exception if we didn't call UpdateHandle() before that.
			pManager->UpdateHandle( handle, 0.0f );
			pManager->SetScale( handle, kindScale, kindScale, kindScale );
		}
		return Handle{ handle };
	}

	bool Handle::IsValid() const
	{
		auto pManager = GetAdmin().GetManagerOrNullptr();
		return ( pManager ) ? pManager->Exists( handle ) : false;
	}

	namespace
	{
		template<typename DoingMethod>
		void OperateIfManagerIsAvailable( DoingMethod method )
		{
			Fx::Manager *pManager = GetAdmin().GetManagerOrNullptr();
			if ( !pManager )
			{
				_ASSERT_EXPR( 0, !"Error: Effect manager is invalid!" );
				return;
			}
			// else

			method( pManager );
		}
	}
	void Handle::SetScale( float scale )
	{
		SetScale( scale, scale, scale );
	}
	void Handle::SetScale( float scaleX, float scaleY, float scaleZ )
	{
		OperateIfManagerIsAvailable
		(
			[&]( Fx::Manager *pManager )
			{
				pManager->SetScale( handle, scaleX, scaleY, scaleZ );
			}
		);
	}
	void Handle::SetScale( const Donya::Vector3 &scale )
	{
		SetScale( scale.x, scale.y, scale.z );
	}
	void Handle::SetRotation( float pitch, float yaw, float roll )
	{
		OperateIfManagerIsAvailable
		(
			[&]( Fx::Manager *pManager )
			{
				pManager->SetRotation( handle, pitch, yaw, roll );
			}
		);
	}
	void Handle::SetPosition( const Donya::Vector3 &pos )
	{
		OperateIfManagerIsAvailable
		(
			[&]( Fx::Manager *pManager )
			{
				pManager->SetLocation( handle, ToFxVector( pos ) );
			}
		);
	}
	void Handle::Move( const Donya::Vector3 &velocity )
	{
		OperateIfManagerIsAvailable
		(
			[&]( Fx::Manager *pManager )
			{
				pManager->AddLocation( handle, ToFxVector( velocity ) );
			}
		);
	}
	void Handle::Stop()
	{
		OperateIfManagerIsAvailable
		(
			[&]( Fx::Manager *pManager )
			{
				pManager->StopEffect( handle );
			}
		);
	}
}
