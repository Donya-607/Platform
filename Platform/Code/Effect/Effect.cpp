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
		Handle rv{};
		rv.Disable();

		auto &admin = GetAdmin();

		Fx::Manager	*pManager	= admin.GetManagerOrNullptr();
		if ( !pManager )
		{
			_ASSERT_EXPR( 0, L"Error: Effect manager is invalid!" );
			return rv;
		}
		// else

		Fx::Effect	*pEffect	= admin.GetEffectOrNullptr( kind );
		if ( !pEffect )
		{
			_ASSERT_EXPR( 0, L"Error: Effect is invalid! May you use wrong file name?" );
			return rv;
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

		rv.Assign( handle );
		return std::move( rv );
	}
	void Handle::Assign( const Fx::Handle &fxHandle )
	{
		handle = fxHandle;
	}
	void Handle::Disable()
	{
		handle = invalidID;
	}

	namespace
	{
		// Hack:
		// I separated these functions to change the return statement.
		// But another code in these function is the same. Is there a smart idea?

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
		template<typename ReturnType, typename DoingMethod>
		ReturnType OperateIfManagerIsAvailable( DoingMethod method )
		{
			Fx::Manager *pManager = GetAdmin().GetManagerOrNullptr();
			if ( !pManager )
			{
				_ASSERT_EXPR( 0, !"Error: Effect manager is invalid!" );
				return ReturnType{};
			}
			// else

			return method( pManager );
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
	void Handle::SetRotation( const Donya::Vector3 &axis, float radian )
	{
		OperateIfManagerIsAvailable
		(
			[&]( Fx::Manager *pManager )
			{
				pManager->SetRotation( handle, ToFxVector( axis ), radian );
			}
		);
	}
	void Handle::SetRotation( const Donya::Quaternion &unitOrientation )
	{
		Donya::Vector3 axis{};
		float radian = 0.0f;

		unitOrientation.ToAxisAngle( &axis, &radian );

		SetRotation( axis, radian );
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
	bool Handle::IsExists() const
	{
		return OperateIfManagerIsAvailable<bool>
		(
			[&]( Fx::Manager *pManager )
			{
				return pManager->Exists( handle );
			}
		);
	}
}
