#include "Skull.h"

#include <numeric>			// Use std::accumulate

#include "../Donya/Random.h"

#include "../Bullets/SkullBullet.h"
#include "../Parameter.h"

namespace Boss
{
	namespace Parameter
	{
		static ParamOperator<SkullParam> skullParam{ "Skull" };
		const  SkullParam &GetSkull()
		{
			return skullParam.Get();
		}

		namespace Impl
		{
			void LoadSkull()
			{
				skullParam.LoadParameter();
			}
		#if USE_IMGUI
			void UpdateSkull( const std::string &nodeCaption )
			{
				skullParam.ShowImGuiNode( nodeCaption );
			}
		#endif // USE_IMGUI
		}
	}

	void Skull::MoverBase::Init( Skull &inst ) {}
	void Skull::MoverBase::Uninit( Skull &inst ) {}
	void Skull::MoverBase::Update( Skull &inst, float elapsedTime, const Input &input ) {}
	void Skull::MoverBase::PhysicUpdate( Skull &inst, float elapsedTime, const std::vector<Donya::Collision::Box3F> &solids )
	{
		inst.Base::PhysicUpdate( elapsedTime, solids );
	}
	
	void Skull::AppearPerformance::Init( Skull &inst )
	{
		inst.AppearInit();
	}
	void Skull::AppearPerformance::Update( Skull &inst, float elapsedTime, const Input &input )
	{
		inst.AppearUpdate( elapsedTime, input );
	}
	void Skull::AppearPerformance::PhysicUpdate( Skull &inst, float elapsedTime, const std::vector<Donya::Collision::Box3F> &solids )
	{
		wasLanding = inst.AppearPhysicUpdate( elapsedTime, solids );
	}
	bool Skull::AppearPerformance::ShouldChangeMover( const Skull &inst ) const
	{
		return wasLanding; // For now, transition as immediately if the falling is finished
	}
	std::function<void()> Skull::AppearPerformance::GetChangeStateMethod( Skull &inst ) const
	{
		return [&]() { inst.AssignMover<DetectTargetAction>(); };
	}
#if USE_IMGUI
	std::string Skull::AppearPerformance::GetMoverName() const
	{
		return u8"登場";
	}
#endif // USE_IMGUI
	
	void Skull::DetectTargetAction::Init( Skull &inst )
	{
		inst.velocity = 0.0f;
	}
	void Skull::DetectTargetAction::Update( Skull &inst, float elapsedTime, const Input &input )
	{
		if ( nextState != Destination::None ) { return; }
		// else

		auto AssignShot = [&]()
		{
			nextState = Destination::Shot;
			inst.RegisterPreviousBehavior( Behavior::Shot );
		};
		auto AssignJump = [&]()
		{
			nextState		= Destination::Jump;
			inst.aimingPos	= input.wsTargetPos;
			inst.RegisterPreviousBehavior( Behavior::Jump );
		};
		auto PrevActionIs = [&]( Behavior comparison )
		{
			return ( inst.previousBehaviors.front() == comparison );
		};

		// Shot detection
		if ( !IsZero( input.controllerInputDirection.x ) )
		{
			if ( IsContinuingSameAction( inst ) && PrevActionIs( Behavior::Shot ) )
			{
				AssignJump();
			}
			else
			{
				AssignShot();
			}

			return;
		}
		// else

		// Jump detection(trigger timing only)
		if ( input.pressShot && !inst.previousInput.pressShot )
		{
			if ( IsContinuingSameAction( inst ) && PrevActionIs( Behavior::Jump ) )
			{
				AssignShot();
			}
			else
			{
				AssignJump();
			}

			return;
		}
		// else
	}
	bool Skull::DetectTargetAction::ShouldChangeMover( const Skull &inst ) const
	{
		return ( nextState != Destination::None ) ? true : false;
	}
	std::function<void()> Skull::DetectTargetAction::GetChangeStateMethod( Skull &inst ) const
	{
		switch ( nextState )
		{
		case Destination::Shot: return [&]() { inst.AssignMover<Shot>(); };
		case Destination::Jump: return [&]() { inst.AssignMover<Jump>(); };
		default: break;
		}

		_ASSERT_EXPR( 0, L"Error: ChangeStateMethod() requested when invalid timing!" );
		return [&]() { inst.AssignMover<DetectTargetAction>(); }; // Fail safe
	}
#if USE_IMGUI
	std::string Skull::DetectTargetAction::GetMoverName() const
	{
		return u8"行動検知";
	}
#endif // USE_IMGUI
	bool Skull::DetectTargetAction::IsContinuingSameAction( const Skull &inst ) const
	{
		return ( inst.previousBehaviors.front() == inst.previousBehaviors.back() );
	}

	void Skull::Shot::Init( Skull &inst )
	{
		inst.velocity	= 0.0f;

		timer			= 0.0f;
		interval		= Parameter::GetSkull().shotFireIntervalSecond; // Make to fire immediately when begin-lag is finished.
		fireCount		= 0;
		wasFinished		= false;
	}
	void Skull::Shot::Update( Skull &inst, float elapsedTime, const Input &input )
	{
		timer += elapsedTime;

		const auto &data = Parameter::GetSkull();

		if ( data.shotFireCount <= fireCount )
		{
			if ( data.shotEndLagSecond <= timer )
			{
				wasFinished = true;
			}
			return;
		}
		// else

		if ( timer < data.shotBeginLagSecond ) { return; }
		// else

		interval += elapsedTime;
		if ( data.shotFireIntervalSecond <= interval )
		{
			Fire( inst, elapsedTime, input );
			fireCount++;
			interval -= data.shotFireIntervalSecond;
		}

		if ( data.shotFireCount <= fireCount )
		{
			timer = 0; // Re count from the beginning
		}
	}
	bool Skull::Shot::ShouldChangeMover( const Skull &inst ) const
	{
		return wasFinished;
	}
	std::function<void()> Skull::Shot::GetChangeStateMethod( Skull &inst ) const
	{
		return [&]() { inst.AssignMover<DetectTargetAction>(); };
	}
#if USE_IMGUI
	std::string Skull::Shot::GetMoverName() const
	{
		return u8"ショット";
	}
#endif // USE_IMGUI
	void Skull::Shot::Fire( Skull &inst, float elapsedTime, const Input &input ) const
	{
		const auto &data = Parameter::GetSkull();

		Bullet::FireDesc desc = data.shotDesc;
		desc.kind			=  data.shotDesc.kind;
		desc.initialSpeed	=  data.shotDesc.initialSpeed;
		desc.position		=  inst.orientation.RotateVector( data.shotDesc.position );
		desc.position		+= inst.body.WorldPosition(); // Local space to World space
		desc.owner			=  inst.hurtBox.id;

		desc.direction				= ( input.wsTargetPos - desc.position ).Unit();
		const float &increment		= data.shotDegreeIncrement;
		const float actualDegree	= desc.direction.XY().Degree();
		const float roundDegree		= std::roundf( actualDegree );
		const float remain			= fmodf( roundDegree, increment );

		float resultDegree = roundDegree - remain;

		/*
		The angles interval set from:
		|---|---|---|---|
		to:
		--|---|---|---|--
		*/
		if ( increment * 0.5f < fabsf( remain ) )
		{
			resultDegree += increment;
		}

		desc.direction.x = cosf( ToRadian( resultDegree ) );
		desc.direction.y = sinf( ToRadian( resultDegree ) );

		Bullet::Admin::Get().RequestFire( desc );
	}

	void Skull::Jump::Init( Skull &inst )
	{
		const auto &data = Parameter::GetSkull();

		const Donya::Vector2 horizontalDiff	= ( inst.aimingPos - inst.body.WorldPosition() ).XZ();
		const float actualLength			= horizontalDiff.Length();
		const auto &locations				= data.jumpDestLengths;
		
		float length = actualLength;
		bool  jumpAsVertically = false;
		if ( !locations.empty() )
		{
			const size_t locationCount = locations.size();
			for ( size_t i = 0; i < locationCount; ++i )
			{
				if ( actualLength <= locations[i] )
				{
					if ( i == 0 )
					{
						jumpAsVertically = true;
						length = 0.0f;
					}
					else
					{
						jumpAsVertically = false;
						length = locations[i - 1];
					}

					break;
				}
				else if ( locationCount <= i + 1 )
				{
					length = locations.back();
				}
			}
		}

		length = std::min( length, actualLength );

		const float theta	= ToRadian( ( jumpAsVertically ) ? 90.0f : Donya::Clamp( data.jumpDegree, 0.1f, 90.0f ) );
		const float sin		= sinf( theta );
		const float cos		= cosf( theta );

		float speed = 0.0f;
		if ( jumpAsVertically )
		{
			// v = sqrt( 2*g*h ) / sinT
			// See: https://keisan.casio.jp/exec/system/1204505822

			// But it will jump as vertically here, the sin(theta) will be 1.0f.

			const float numerator = sqrtf( 2.0f * inst.GetGravity() * data.jumpVerticalHeight );
			speed = numerator;
		}
		else
		{
			// v = sqrt( g*l / 2*sinT*cosT )
			// See: https://keisan.casio.jp/exec/system/1204505832

			const float numerator	= inst.GetGravity() * length;
			const float denominator	= 2.0f * sin * cos;

			// The sin and cos are made by 0 ~ 90 degrees, so these has guaranteed to be a positive value.
			// So it will not pass a negative value to sqrtf().
			speed = sqrtf( numerator / ( denominator + EPSILON ) );
		}

		inst.velocity.x = cos * speed * Donya::SignBitF( horizontalDiff.x );
		inst.velocity.y = sin * speed;
		inst.velocity.z = 0.0f;
		wasLanding = false;
	}
	void Skull::Jump::Update( Skull &inst, float elapsedTime, const Input &input )
	{
		inst.velocity.y -= inst.GetGravity() * elapsedTime;
	}
	void Skull::Jump::PhysicUpdate( Skull &inst, float elapsedTime, const std::vector<Donya::Collision::Box3F> &solids )
	{
		if ( inst.NowDead() ) { return; }
		// else

		inst.MoveOnlyX( elapsedTime, solids );
		inst.MoveOnlyZ( elapsedTime, solids );

		const int collideIndex = inst.MoveOnlyY( elapsedTime, solids );
		if ( collideIndex != -1 ) // If collide to any
		{
			if ( inst.velocity.y <= 0.0f )
			{
				wasLanding = true;
			}
		}
	}
	bool Skull::Jump::ShouldChangeMover( const Skull &inst ) const
	{
		return wasLanding;
	}
	std::function<void()> Skull::Jump::GetChangeStateMethod( Skull &inst ) const
	{
		return [&]() { inst.AssignMover<Shield>(); };
	}
#if USE_IMGUI
	std::string Skull::Jump::GetMoverName() const
	{
		return u8"ジャンプ";
	}
#endif // USE_IMGUI

	void Skull::Shield::Init( Skull &inst )
	{
		inst.velocity	= 0.0f;
		ReleaseShieldIfHas( inst );

		nowProtected	= false;
		timer			= 0.0f;

		const auto &protectSeconds = Parameter::GetSkull().shieldProtectSeconds;
		if ( protectSeconds.empty() ) { protectSecond = 1.0f; }
		else
		{
			auto AddBias = []( int sum, const SkullParam::RandomElement &element )
			{
				return sum + element.bias;
			};
			const int randomLimit = std::accumulate( protectSeconds.cbegin(), protectSeconds.cend(), 0, AddBias );
			if ( randomLimit <= 0 ) { protectSecond = 1.0f; }
			else
			{
				const int random = Donya::Random::GenerateInt( randomLimit );
				int border = 0;
				for ( const auto &it : protectSeconds )
				{
					border += it.bias;
					if ( random < border )
					{
						protectSecond = it.second;
						break;
					}
				}
			}
		}
	}
	void Skull::Shield::Update( Skull &inst, float elapsedTime, const Input &input )
	{
		const auto &data = Parameter::GetSkull();

		timer += elapsedTime;

		if ( data.shieldBeginLagSecond <= timer && timer < protectSecond )
		{
			nowProtected = true;
			GenerateShieldIfNull( inst );
		}
		else
		{
			nowProtected = false;
			ReleaseShieldIfHas( inst );
		}

		// Calculate the destination of Run state
		{
			const Donya::Vector3 myselfPos = inst.body.WorldPosition();
			const int toTargetSign = Donya::SignBit( input.wsTargetPos.x - myselfPos.x );
		
			Donya::Vector3 destination = input.wsTargetPos;
			destination.x -= data.runDestTakeDist * toTargetSign;

			// Aim to the front of "targetPos".
			// But if that aiming pos places backward of myself, it do not move.
			const int toDestinationSign = Donya::SignBit( destination.x - myselfPos.x );
			if ( toDestinationSign != toTargetSign ) // It means you should move to backward
			{
				inst.aimingPos = myselfPos;
			}
			else
			{
				inst.aimingPos = destination;
			}
		}

		if ( inst.pShield )
		{
			inst.pShield->Update( elapsedTime, inst.roomArea );
			inst.pShield->SetWorldPosition( inst.body.WorldPosition() );
		}
	}
	bool Skull::Shield::ShouldChangeMover( const Skull &inst ) const
	{
		return ( protectSecond + Parameter::GetSkull().shieldEndLagSecond <= timer ) ? true : false;
	}
	std::function<void()> Skull::Shield::GetChangeStateMethod( Skull &inst ) const
	{
		return [&]() { inst.AssignMover<Run>(); };
	}
#if USE_IMGUI
	std::string Skull::Shield::GetMoverName() const
	{
		return u8"シールド";
	}
#endif // USE_IMGUI
	void Skull::Shield::GenerateShieldIfNull( Skull &inst )
	{
		if ( inst.pShield ) { return; }
		// else

		Bullet::FireDesc desc;
		desc.kind			= Bullet::Kind::SkullShield;
		desc.initialSpeed	= 0.0f;
		desc.direction		= Donya::Vector3::Zero();
		desc.position		= inst.body.WorldPosition();
		
		inst.pShield = std::make_unique<Bullet::SkullShield>();
		inst.pShield->Init( desc );
	}
	void Skull::Shield::ReleaseShieldIfHas( Skull &inst )
	{
		if ( !inst.pShield ) { return; }
		// else

		inst.pShield->Uninit();
		inst.pShield.reset();
	}

	void Skull::Run::Init( Skull &inst )
	{
		inst.velocity.x = Parameter::GetSkull().runSpeed * GetCurrentDirectionSign( inst );
		inst.velocity.y = 0.0f;

		timer		= 0.0f;
		wasArrived	= false;
		initialPos	= inst.body.WorldPosition();
	}
	void Skull::Run::Update( Skull &instance, float elapsedTime, const Input &input )
	{
		if ( wasArrived )
		{
			timer += elapsedTime;
		}
	}
	void Skull::Run::PhysicUpdate( Skull &inst, float elapsedTime, const std::vector<Donya::Collision::Box3F> &solids )
	{
		const int prevSign = GetCurrentDirectionSign( inst );
		MoverBase::PhysicUpdate( inst, elapsedTime, solids );
		const int nowSign  = GetCurrentDirectionSign( inst );

		if ( nowSign != prevSign || nowSign == 0 )
		{
			wasArrived = true;
			inst.body.pos.x  = inst.aimingPos.x;
			inst.hurtBox.pos = inst.body.pos;
		}
	}
	bool Skull::Run::ShouldChangeMover( const Skull &inst ) const
	{
		return ( Parameter::GetSkull().runEndLagSecond <= timer );
	}
	std::function<void()> Skull::Run::GetChangeStateMethod( Skull &inst ) const
	{
		return [&]() { inst.AssignMover<DetectTargetAction>(); };
	}
#if USE_IMGUI
	std::string Skull::Run::GetMoverName() const
	{
		return u8"走行";
	}
#endif // USE_IMGUI
	int  Skull::Run::GetCurrentDirectionSign( const Skull &inst ) const
	{
		return Donya::SignBit( inst.aimingPos.x - inst.body.WorldPosition().x );
	}


	void Skull::Init( const InitializeParam &parameter, int roomID, const Donya::Collision::Box3F &wsRoomArea )
	{
		Base::Init( parameter, roomID, wsRoomArea );

		previousBehaviors.fill( Behavior::None );

		AssignMover<AppearPerformance>();
	}
	void Skull::Update( float elapsedTime, const Input &input )
	{
	#if USE_IMGUI
		// Apply for be able to see an adjustment immediately
		{
			const auto &data = Parameter::GetSkull();
			body.offset		= data.hitBoxOffset;
			body.size		= data.hitBoxSize;
			hurtBox.offset	= data.hurtBoxOffset;
			hurtBox.size	= data.hurtBoxSize;
		}
	#endif // USE_IMGUI

		Base::Update( elapsedTime, input );

		if ( NowDead() ) { return; }
		// else

		if ( !pMover )
		{
			_ASSERT_EXPR( 0, L"Error: Mover does not exist!" );
			return;
		}
		// else

		pMover->Update( *this, elapsedTime, input );
		if ( pMover->ShouldChangeMover( *this ) )
		{
			auto ChangeState = pMover->GetChangeStateMethod( *this );
			ChangeState();
		}

		const bool lookingRight = ( 0.0f <= ( input.wsTargetPos - body.WorldPosition() ).x ) ? true : false;
		UpdateOrientation( lookingRight );
		
		// TODO: Set a current motion index
		UpdateMotionIfCan( elapsedTime, NULL );

		previousInput = input;
	}
	void Skull::PhysicUpdate( float elapsedTime, const std::vector<Donya::Collision::Box3F> &solids )
	{
		if ( !pMover )
		{
			_ASSERT_EXPR( 0, L"Error: Mover does not exist!" );
			return;
		}
		// else

		pMover->PhysicUpdate( *this, elapsedTime, solids );
	}
	void Skull::Draw( RenderingHelper *pRenderer ) const
	{
		Base::Draw( pRenderer );

		if ( pShield )
		{
			pShield->Draw( pRenderer );
		}
	}
	void Skull::DrawHitBox( RenderingHelper *pRenderer, const Donya::Vector4x4 &matVP ) const
	{
		Base::DrawHitBox( pRenderer, matVP );

		if ( pShield )
		{
			pShield->DrawHitBox( pRenderer, matVP );
		}
	}
	float Skull::GetGravity() const
	{
		return Parameter::GetSkull().gravity;
	}
	Kind Skull::GetKind() const { return Kind::Skull; }
	Definition::Damage Skull::GetTouchDamage() const
	{
		return Parameter::GetSkull().touchDamage;
	}
	bool Skull::NowProtecting() const
	{
		return ( pShield ) ? true : false;
	}
	int  Skull::GetInitialHP() const
	{
		return Parameter::GetSkull().hp;
	}
	void Skull::AssignMyBody( const Donya::Vector3 &wsPos )
	{
		const auto &data = Parameter::GetSkull();
		body.pos		= wsPos;
		body.offset		= data.hitBoxOffset;
		body.size		= data.hitBoxSize;
		hurtBox.pos		= wsPos;
		hurtBox.offset	= data.hurtBoxOffset;
		hurtBox.size	= data.hurtBoxSize;
	}
	void Skull::RegisterPreviousBehavior( Behavior behavior )
	{
		previousBehaviors.back()  = previousBehaviors.front();
		previousBehaviors.front() = behavior;
	}

#if USE_IMGUI
	bool Skull::ShowImGuiNode( const std::string &nodeCaption )
	{
		if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return false; }
		// else

		Base::ShowImGuiNode( u8"基底部分" );

		if ( ImGui::TreeNode( u8"派生部分" ) )
		{
			ImGui::Text( u8"現在のステート：%s", ( pMover ) ? pMover->GetMoverName().c_str() : u8"ERROR_STATE" );

			ImGui::TreePop();
		}

		ImGui::TreePop();
		return true;
	}
	void SkullParam::ShowImGuiNode()
	{
		if ( ImGui::TreeNode( u8"共通パラメータ" ) )
		{
			ImGui::DragInt   ( u8"初期ＨＰ",	&hp					);
			ImGui::DragFloat ( u8"重力",		&gravity,	0.01f	);
			hp		= std::max( 1,		hp		);
			gravity	= std::max( 0.001f, gravity	);

			ImGui::TreePop();
		}

		if ( ImGui::TreeNode( u8"ショット関連" ) )
		{
			ImGui::DragInt  ( u8"発射回数",					&shotFireCount						);
			ImGui::DragFloat( u8"前隙（秒）",				&shotBeginLagSecond,		0.01f	);
			ImGui::DragFloat( u8"発射間隔（秒）",				&shotFireIntervalSecond,	0.01f	);
			ImGui::DragFloat( u8"後隙（秒）",				&shotEndLagSecond,			0.01f	);
			ImGui::DragFloat( u8"発射角度の刻み(degree)",		&shotDegreeIncrement,		1.0f	);
			shotFireCount			= std::max( 1,		shotFireCount			);
			shotBeginLagSecond		= std::max( 0.0f,	shotBeginLagSecond		);
			shotFireIntervalSecond	= std::max( 0.0f,	shotFireIntervalSecond	);
			shotEndLagSecond		= std::max( 0.0f,	shotEndLagSecond		);
			shotDegreeIncrement		= std::max( 0.0f,	shotDegreeIncrement		);

			shotDesc.ShowImGuiNode( u8"発射設定" );

			ImGui::TreePop();
		}

		if ( ImGui::TreeNode( u8"ジャンプ関連" ) )
		{
			ImGui::Text( u8"角度は右向きで真横を０度，真上を９０度とした場合" );
			ImGui::SliderFloat( u8"射出角度(degree)", &jumpDegree, 0.1f, 90.0f );

			ImGui::DragFloat( u8"垂直ジャンプ時の高さ", &jumpVerticalHeight, 0.1f );
			jumpVerticalHeight = std::max( 0.1f, jumpVerticalHeight );

			if ( ImGui::TreeNode( u8"終点の距離たち" ) )
			{
				const float lastLength = ( jumpDestLengths.empty() ) ? 0.0f : jumpDestLengths.back();
				ImGui::Helper::ResizeByButton( &jumpDestLengths, lastLength + 0.01f );

				std::string caption;

				const size_t count = jumpDestLengths.size();
				for ( size_t i = 0; i < count; ++i )
				{
					caption = Donya::MakeArraySuffix( i );
					ImGui::DragFloat( caption.c_str(), &jumpDestLengths[i], 0.1f );

					const float lowestLength = ( i == 0 ) ? 0.0f : jumpDestLengths[i - 1];
					jumpDestLengths[i] = Donya::Clamp( jumpDestLengths[i], lowestLength, jumpDestLengths[i] );
				}

				ImGui::TreePop();
			}

			ImGui::TreePop();
		}

		if ( ImGui::TreeNode( u8"シールド関連" ) )
		{
			ImGui::DragFloat( u8"展開までの秒数", &shieldBeginLagSecond,	0.01f );
			ImGui::DragFloat( u8"展開後隙の秒数", &shieldEndLagSecond,	0.01f );
			shieldBeginLagSecond	= std::max( 0.0f, shieldBeginLagSecond	);
			shieldEndLagSecond		= std::max( 0.0f, shieldEndLagSecond	);

			if ( ImGui::TreeNode( u8"展開時間の設定" ) )
			{
				RandomElement argument;
				argument.bias	= 1;
				argument.second	= 1.0f;
				ImGui::Helper::ResizeByButton( &shieldProtectSeconds, argument );
				if ( shieldProtectSeconds.empty() )
				{
					shieldProtectSeconds.emplace_back( argument );
				}

				auto ShowElement = []( const std::string &caption, RandomElement *p )
				{
					if ( !ImGui::TreeNode( caption.c_str() ) ) { return; }
					// else

					ImGui::DragInt  ( u8"選ばれやすさ",	&p->bias );
					ImGui::DragFloat( u8"待機秒数",		&p->second, 0.01f );
					p->bias		= std::max( 0,		p->bias		);
					p->second	= std::max( 0.0f,	p->second	);

					ImGui::TreePop();
				};

				std::string caption{};
				const size_t count = shieldProtectSeconds.size();
				for ( size_t i = 0; i < count; ++i )
				{
					caption = Donya::MakeArraySuffix( i );
					ShowElement( caption, &shieldProtectSeconds[i] );
				}

				ImGui::TreePop();
			}

			ImGui::TreePop();
		}

		if ( ImGui::TreeNode( u8"走行関連" ) )
		{
			ImGui::DragFloat( u8"速度",					&runSpeed,			0.01f );
			ImGui::DragFloat( u8"終点から離れる距離",		&runDestTakeDist,	0.01f );
			ImGui::DragFloat( u8"着いてからの待機秒数",	&runEndLagSecond,	0.01f );
			runSpeed		= std::max( 0.001f,		runSpeed		);
			runDestTakeDist	= std::max( 0.0f,		runDestTakeDist	);
			runEndLagSecond	= std::max( 0.0f,		runEndLagSecond	);

			ImGui::TreePop();
		}

		if ( ImGui::TreeNode( u8"当たり判定関連" ) )
		{
			ImGui::DragFloat3( u8"当たり判定・オフセット",			&hitBoxOffset.x,	0.01f	);
			ImGui::DragFloat3( u8"当たり判定・サイズ（半分を指定）",	&hitBoxSize.x,		0.01f	);
			ImGui::DragFloat3( u8"喰らい判定・オフセット",			&hurtBoxOffset.x,	0.01f	);
			ImGui::DragFloat3( u8"喰らい判定・サイズ（半分を指定）",	&hurtBoxSize.x,		0.01f	);
			touchDamage.ShowImGuiNode( u8"接触ダメージ設定" );

			hitBoxSize.x	= std::max( 0.0f,	hitBoxSize.x	);
			hitBoxSize.y	= std::max( 0.0f,	hitBoxSize.y	);
			hitBoxSize.z	= std::max( 0.0f,	hitBoxSize.z	);
			hurtBoxSize.x	= std::max( 0.0f,	hurtBoxSize.x	);
			hurtBoxSize.y	= std::max( 0.0f,	hurtBoxSize.y	);
			hurtBoxSize.z	= std::max( 0.0f,	hurtBoxSize.z	);

			ImGui::TreePop();
		}

	}
#endif // USE_IMGUI
}
