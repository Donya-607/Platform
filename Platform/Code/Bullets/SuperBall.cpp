#include "SuperBall.h"

#include "../Parameter.h"

namespace Bullet
{
	namespace Parameter
	{
		static ParamOperator<SuperBallParam> skullBusterParam{ "SuperBall", "Bullet/" };

		const SuperBallParam &GetSuperBall()
		{
			return skullBusterParam.Get();
		}

		namespace Impl
		{
			void LoadSuperBall()
			{
				skullBusterParam.LoadParameter();
			}
		#if USE_IMGUI
			void UpdateSuperBall( const std::string &nodeCaption )
			{
				skullBusterParam.ShowImGuiNode( nodeCaption );
			}
		#endif // USE_IMGUI
		}
	}


	void SuperBall::Init( const FireDesc &parameter )
	{
		Base::Init( parameter );

		accelCount = 0;
	}
	void SuperBall::Uninit() {} // No op
	void SuperBall::Update( float elapsedTime, const Donya::Collision::Box3F &wsScreenHitBox )
	{
		Base::Update( elapsedTime, wsScreenHitBox );

		const auto &data = Parameter::GetSuperBall();
		UpdateMotionIfCan( elapsedTime * data.basic.animePlaySpeed, 0 );
	}
	void SuperBall::PhysicUpdate( float elapsedTime, const Map &terrain )
	{
		const auto &data = Parameter::GetSuperBall();
		Donya::Vector3 acceleratedVelocity = velocity;
		for ( int i = 0; i < accelCount; ++i )
		{
			acceleratedVelocity *= data.acceleratePercent;
		}

		// Use an Actor's collision method
		Actor mover;
		mover.body = GetHitBox();

		const auto movement		= acceleratedVelocity * elapsedTime;
		const auto aroundTiles	= terrain.GetPlaceTiles( mover.body, movement );
		const auto aroundSolids	= Map::ToAABBSolids( aroundTiles, terrain, mover.body );

		bool wasCollided  = false;
		int  collideIndex = -1;
		auto IsCollided = []( int collideIndex )
		{
			return ( collideIndex != -1 ) ? true : false;
		};

		constexpr int dimensionCount = 3; // X,Y,Z
		for ( int i = 0; i < dimensionCount; ++i )
		{
			collideIndex = Actor::MoveAxis( &mover, i, movement[i], aroundSolids );
			if ( IsCollided( collideIndex ) )
			{
				velocity[i] *= -1.0f;
				wasCollided =  true;
			}
		}

		if ( wasCollided )
		{
			accelCount++;
			accelCount = std::min( data.accelerateCount, accelCount );
		}

		body.pos		= mover.body.pos;
		hitSphere.pos	= body.pos;
	}
	bool SuperBall::Destructible() const
	{
		return true;
	}
	Kind SuperBall::GetKind() const
	{
		return Kind::SuperBall;
	}
	Definition::Damage SuperBall::GetDamageParameter() const
	{
		return Parameter::GetSuperBall().basic.damage;
	}
	void SuperBall::AssignBodyParameter( const Donya::Vector3 &wsPos )
	{
		const auto &data = Parameter::GetSuperBall().basic;
		body.pos	= wsPos;
		body.offset	= data.hitBoxOffset;
		body.size	= data.hitBoxSize;
	}
#if USE_IMGUI
	void SuperBallParam::ShowImGuiNode()
	{
		basic.ShowImGuiNode( u8"汎用設定" );
		ImGui::DragInt  ( u8"加速回数上限",	&accelerateCount );
		ImGui::DragFloat( u8"加速倍率",		&acceleratePercent, 0.01f );
		ImGui::Text( u8"（加速は反射時に行います）" );
		accelerateCount		= std::max(    0, accelerateCount   );
		acceleratePercent	= std::max( 0.0f, acceleratePercent );
	}
#endif // USE_IMGUI

}
