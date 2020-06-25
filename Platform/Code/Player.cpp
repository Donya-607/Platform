#include "Player.h"

#include "Donya/Loader.h"
#include "Donya/Sound.h"

#include "Common.h"
#include "FilePath.h"
#include "Music.h"
#include "Parameter.h"
#include "PlayerParam.h"

namespace
{
	constexpr size_t		MOTION_COUNT	= scast<size_t>( Player::MotionKind::MotionCount );
	constexpr const char	*MODEL_NAME		= "Player/Player";
	constexpr const char	*KIND_NAMES[MOTION_COUNT]
	{
		"Idle",
		"Run",
		"Jump",
	};

	struct SourceInstance
	{
		Donya::Model::SkinningModel	model;
		Donya::Model::MotionHolder	motionHolder;
	};
	static std::unique_ptr<SourceInstance> pModel{};

	bool LoadModel()
	{
		// Already has loaded.
		if ( pModel ) { return true; }
		// else

		const auto filePath = MakeModelPath( MODEL_NAME );

		if ( !Donya::IsExistFile( filePath ) )
		{
			Donya::OutputDebugStr( "Error : The Player's model file does not exist." );
			return false;
		}
		// else

		Donya::Loader loader{};
		if ( !loader.Load( filePath ) ) { return false; }
		// else

		const Donya::Model::Source source = loader.GetModelSource();

		pModel = std::make_unique<SourceInstance>();
		pModel->model = Donya::Model::SkinningModel::Create( source, loader.GetFileDirectory() );
		pModel->motionHolder.AppendSource( source );

		if ( !pModel->model.WasInitializeSucceeded() )
		{
			pModel.reset();
			return false;
		}
		// else

		return true;
	}

	bool IsOutOfRange( Player::MotionKind kind )
	{
		const size_t uintKind = scast<size_t>( kind );
		return ( MOTION_COUNT <= uintKind ) ? true : false;
	}
	const Donya::Model::SkinningModel &GetModel()
	{
		_ASSERT_EXPR( pModel, L"Error : The Player's model does not initialized!" );
		return pModel->model;
	}
	const Donya::Model::MotionHolder  &GetMotions()
	{
		_ASSERT_EXPR( pModel, L"Error : The Player's motions does not initialized!" );
		return pModel->motionHolder;
	}
}

Donya::Vector3	PlayerInitializer::GetWorldInitialPos() const { return wsInitialPos; }
bool			PlayerInitializer::ShouldLookingRight() const { return lookingRight; }
void PlayerInitializer::LoadParameter( int stageNo )
{
#if DEBUG_MODE
	LoadJson( stageNo );
#else
	LoadBin( stageNo );
#endif // DEBUG_MODE
}
void PlayerInitializer::LoadBin( int stageNo )
{
	constexpr bool fromBinary = true;
	Donya::Serializer::Load( *this, MakeStageParamPathBinary( ID, stageNo ).c_str(), ID, fromBinary );
}
void PlayerInitializer::LoadJson( int stageNo )
{
	constexpr bool fromBinary = false;
	Donya::Serializer::Load( *this, MakeStageParamPathJson( ID, stageNo ).c_str(), ID, fromBinary );
}
#if USE_IMGUI
void PlayerInitializer::SaveBin( int stageNo )
{
	constexpr bool fromBinary = true;

	const std::string filePath = MakeStageParamPathBinary( ID, stageNo );
	MakeDirectoryIfNotExists( filePath );
	MakeFileIfNotExists( filePath, fromBinary );

	Donya::Serializer::Save( *this, filePath.c_str(), ID, fromBinary );
}
void PlayerInitializer::SaveJson( int stageNo )
{
	constexpr bool fromBinary = false;

	const std::string filePath = MakeStageParamPathJson( ID, stageNo );
	MakeDirectoryIfNotExists( filePath );
	MakeFileIfNotExists( filePath, fromBinary );

	Donya::Serializer::Save( *this, filePath.c_str(), ID, fromBinary );
}
void PlayerInitializer::ShowImGuiNode( const std::string &nodeCaption, int stageNo, bool allowShowIONode )
{
	if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
	// else

	ImGui::DragFloat3( u8"初期のワールド座標",	&wsInitialPos.x, 0.01f );
	ImGui::Checkbox  ( u8"初期は右向きか",		&lookingRight );

	if ( allowShowIONode )
	{
		const auto result = ParameterHelper::ShowIONode();
		using Op = ParameterHelper::IOOperation;
		if ( result == Op::Save )
		{
			SaveBin ( stageNo );
			SaveJson( stageNo );
		}
		else if ( result == Op::LoadBinary )
		{
			LoadBin( stageNo );
		}
		else if ( result == Op::LoadJson )
		{
			LoadJson( stageNo );
		}
	}

	ImGui::TreePop();
}
#endif // USE_IMGUI

ParamOperator<PlayerParam> Player::paramInstance{ "Player" };
bool Player::LoadResource()
{
	paramInstance.LoadParameter();
	return LoadModel();
}
const ParamOperator<PlayerParam> &Player::Parameter()
{
	return paramInstance;
}
#if USE_IMGUI
void Player::UpdateParameter( const std::string &nodeCaption )
{
	paramInstance.ShowImGuiNode( nodeCaption );
}
void PlayerParam::ShowImGuiNode()
{
	ImGui::DragFloat( u8"移動速度",			&moveSpeed,			0.01f );
	ImGui::DragFloat( u8"ジャンプ力",		&jumpStrength,		0.01f );
	ImGui::DragFloat( u8"重力",				&gravity,			0.01f );
	ImGui::SliderFloat( u8"重力抵抗力",		&gravityResistance,	0.0f, 1.0f );
	ImGui::DragFloat( u8"重力抵抗可能秒数",	&resistableSeconds,	0.01f );
	ImGui::DragFloat( u8"最高落下速度",		&maxFallSpeed,		0.01f );
	ImGui::Helper::ShowAABBNode( u8"地形との当たり判定", &hitBox  );
	ImGui::Helper::ShowAABBNode( u8"攻撃との喰らい判定", &hurtBox );
	fireParam.ShowImGuiNode( u8"ショット設定" );

	auto MakePositive = []( float *v )
	{
		*v = std::max( 0.001f, *v );
	};
	MakePositive( &moveSpeed			);
	MakePositive( &jumpStrength			);
	MakePositive( &gravity				);
	MakePositive( &resistableSeconds	);
	MakePositive( &maxFallSpeed			);
}
#endif // USE_IMGUI

void Player::MotionManager::Init()
{
	prevKind = currKind = MotionKind::Jump;
	animator.ResetTimer();

	AssignPose( currKind );
}
void Player::MotionManager::Update( Player &inst, float elapsedTime )
{
	prevKind = currKind;
	currKind = CalcNowKind( inst );
	if ( currKind != prevKind )
	{
		animator.ResetTimer();
	}
	
	ShouldEnableLoop( currKind )
	? animator.EnableLoop()
	: animator.DisableLoop();

	animator.Update( elapsedTime );
	AssignPose( currKind );
}
const Donya::Model::Pose &Player::MotionManager::GetPose() const
{
	return pose;
}
int  Player::MotionManager::ToMotionIndex( MotionKind kind ) const
{
	return scast<int>( kind );
}
bool Player::MotionManager::ShouldEnableLoop( MotionKind kind ) const
{
	switch ( kind )
	{
	case MotionKind::Idle:	return true;
	case MotionKind::Run:	return true;
	case MotionKind::Jump:	return false;
	default: break;
	}

	_ASSERT_EXPR( 0, L"Error: Unexpected kind!" );
	return false;
}
void Player::MotionManager::AssignPose( MotionKind kind )
{
	const int   motionIndex  = ToMotionIndex( kind );
	const auto &motionHolder = GetMotions();
	if ( motionHolder.IsOutOfRange( motionIndex ) )
	{
		_ASSERT_EXPR( 0, L"Error: Specified motion index out of range!" );
		return;
	}
	// else

	const auto &currentMotion = motionHolder.GetMotion( motionIndex );
	animator.SetRepeatRange( currentMotion );
	pose.AssignSkeletal( animator.CalcCurrentPose( currentMotion ) );
}
Player::MotionKind Player::MotionManager::CalcNowKind( Player &inst ) const
{
	const bool nowMoving = IsZero( inst.velocity.x ) ? false : true;
	const bool onGround  = inst.onGround;
	
	if ( !onGround ) { return MotionKind::Jump;	}
	if ( nowMoving ) { return MotionKind::Run;	}
	// else
	return MotionKind::Idle;
}

void Player::Init( const PlayerInitializer &initializer )
{
	const auto &data = Parameter().Get();
	body		= data.hitBox;
	hurtBox		= data.hurtBox;
	body.pos	= initializer.GetWorldInitialPos();
	velocity	= 0.0f;
	motionManager.Init();
	onGround	= false;
	
	const float rotateSign = ( initializer.ShouldLookingRight() ) ? 1.0f : -1.0f;
	orientation = Donya::Quaternion::Make
	(
		Donya::Vector3::Up(), ToRadian( 90.0f ) * rotateSign
	);
}
void Player::Uninit()
{

}
void Player::Update( float elapsedTime, Input input )
{
#if USE_IMGUI
	// Apply for be able to see an adjustment immediately
	{
		const auto &data = Parameter().Get();
		auto ApplyExceptPosition = []( Donya::Collision::Box3F *p, const Donya::Collision::Box3F &source )
		{
			p->offset	= source.offset;
			p->size		= source.size;
			p->exist	= source.exist;
		};

		ApplyExceptPosition( &body,		data.hitBox  );
		ApplyExceptPosition( &hurtBox,	data.hurtBox );
	}
#endif // USE_IMGUI

	MoveHorizontal( elapsedTime, input );
	MoveVertical  ( elapsedTime, input );

	Shot( elapsedTime, input );

	motionManager.Update( *this, elapsedTime );
}
void Player::PhysicUpdate( float elapsedTime, const std::vector<Donya::Collision::Box3F> &solids )
{
	const auto movement = velocity * elapsedTime;
	
	Actor::MoveX( movement.x, solids );
	Actor::MoveZ( movement.z, solids );

	const int collideIndex = Actor::MoveY( movement.y, solids );
	if ( collideIndex != -1 ) // If collided to any
	{
		// Consider as landing
		if ( velocity.y <= 0.0f )
		{
			if ( !onGround )
			{
				onGround = true;
				Donya::Sound::Play( Music::Player_Landing );
			}
		}

		velocity.y = 0.0f;
	}
	else
	{
		onGround = false;
	}

	hurtBox.pos = body.pos; // We must apply world position to hurt box also.
}
void Player::Draw( RenderingHelper *pRenderer )
{
	if ( !pRenderer ) { return; }
	// else

	const Donya::Vector3 &drawPos = body.pos; // I wanna adjust the hit-box to fit for drawing model, so I don't apply the offset for the position of drawing model.
	const Donya::Vector4x4 W = MakeWorldMatrix( 1.0f, /* enableRotation = */ true, drawPos );

	const auto &drawModel = GetModel();
	const auto &drawPose  = motionManager.GetPose();

	Donya::Model::Constants::PerModel::Common modelConstant{};
	modelConstant.drawColor		= Donya::Vector4{ 1.0f, 1.0f, 1.0f, 1.0f };
	modelConstant.worldMatrix	= W;;
	pRenderer->UpdateConstant( modelConstant );
	pRenderer->ActivateConstantModel();

	pRenderer->Render( drawModel, drawPose );

	pRenderer->DeactivateConstantModel();
}
void Player::DrawHitBox( RenderingHelper *pRenderer, const Donya::Vector4x4 &matVP )
{
	if ( !Common::IsShowCollision() || !pRenderer ) { return; }
	// else

#if DEBUG_MODE
	Donya::Model::Cube::Constant constant;
	constant.matViewProj	= matVP;
	constant.lightDirection	= -Donya::Vector3::Up();

	auto DrawProcess = [&]( const Donya::Collision::Box3F &box, const Donya::Vector4 &color )
	{
		const Donya::Vector4x4 W = MakeWorldMatrix
		(
			box.size * 2.0f,
			/* enableRotation = */ false,
			box.WorldPosition()
		);

		constant.matWorld  = W;
		constant.drawColor = color;
		pRenderer->ProcessDrawingCube( constant );
	};
	
	constexpr Donya::Vector4 bodyColor{ 0.0f, 1.0f, 0.3f, 0.6f };
	constexpr Donya::Vector4 hurtColor{ 0.0f, 0.3f, 1.0f, 0.6f };
	const auto  &hurt		= hurtBox;
	const float bodyNear	= body.WorldPosition().z - body.size.z;
	const float hurtNear	= hurt.WorldPosition().z - hurt.size.z;
	// Drawing the far box first
	if ( bodyNear < hurtNear )
	{
		DrawProcess( hurt, hurtColor );
		DrawProcess( body, bodyColor );
	}
	else
	{
		DrawProcess( body, bodyColor );
		DrawProcess( hurt, hurtColor );
	}
#endif // DEBUG_MODE
}
Donya::Collision::Box3F	Player::GetHurtBox() const
{
	return hurtBox;
}
Donya::Quaternion		Player::GetOrientation() const
{
	return orientation;
}
void Player::MoveHorizontal( float elapsedTime, Input input )
{
	const auto &data = Parameter().Get();

	const float  movement = data.moveSpeed * input.moveVelocity.x;
	velocity.x = movement;

	if ( !IsZero( velocity.x ) )
	{
		const float rotateSign = ( 0.0f < velocity.x ) ? 1.0f : -1.0f;
		orientation = Donya::Quaternion::Make
		(
			Donya::Vector3::Up(), ToRadian( 90.0f ) * rotateSign
		);
	}
}
void Player::MoveVertical  ( float elapsedTime, Input input )
{
	const auto &data = Parameter().Get();

	if ( input.useJump && onGround && wasReleasedJumpInput )
	{
		onGround				= false;
		velocity.y				= data.jumpStrength;
		keepJumpSecond			= 0.0f;
		wasReleasedJumpInput	= false;
		Donya::Sound::Play( Music::Player_Jump );
	}
	else
	{
		bool resistGravity = false;
		if ( input.useJump && !wasReleasedJumpInput )
		{
			keepJumpSecond += elapsedTime;
			if ( keepJumpSecond < data.resistableSeconds )
			{
				resistGravity = true;
			}
		}
		else
		{
			wasReleasedJumpInput = true;
		}

		const float applyGravity =	( resistGravity )
									? data.gravity * data.gravityResistance
									: data.gravity;
		velocity.y -= applyGravity * elapsedTime;
		velocity.y = std::max( -data.maxFallSpeed, velocity.y );
	}
}
void Player::Shot( float elapsedTime, Input input )
{
	if ( !input.useShot )
	{
		keepShotSecond = 0.0f;
		return;
	}
	// else

	const bool nowTriggered = IsZero( keepShotSecond );

	keepShotSecond += elapsedTime;

	if ( nowTriggered )
	{
		const auto &data = Parameter().Get();

		const Donya::Vector3 front = orientation.LocalFront();
		const float dot = Donya::Dot( front, Donya::Vector3::Right() );
		const float lookingSign = Donya::SignBitF( dot );

		Bullet::FireDesc desc = data.fireParam;
		desc.direction	=  Donya::Vector3::Right() * lookingSign;
		desc.position.x	*= lookingSign;
		desc.position	+= GetPosition();
		
		Bullet::Admin::Get().RequestFire( desc );
		Donya::Sound::Play( Music::Player_Shot );
	}
}
Donya::Vector4x4 Player::MakeWorldMatrix( const Donya::Vector3 &scale, bool enableRotation, const Donya::Vector3 &translation ) const
{
	Donya::Vector4x4 W{};
	W._11 = scale.x;
	W._22 = scale.y;
	W._33 = scale.z;
	if ( enableRotation ) { W *= orientation.MakeRotationMatrix(); }
	W._41 = translation.x;
	W._42 = translation.y;
	W._43 = translation.z;
	return W;
}
#if USE_IMGUI
void Player::ShowImGuiNode( const std::string &nodeCaption )
{
	if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
	// else

	ImGui::DragFloat3( u8"ワールド座標",	&body.pos.x,	0.01f );
	ImGui::DragFloat3( u8"速度",			&velocity.x,	0.01f );

	Donya::Vector3 front = orientation.LocalFront();
	ImGui::SliderFloat3( u8"前方向",		&front.x, -1.0f, 1.0f );
	orientation = Donya::Quaternion::LookAt( Donya::Vector3::Front(), front.Unit(), Donya::Quaternion::Freeze::Up );

	ImGui::DragFloat( u8"ショットを入力し続けている秒数", &keepShotSecond, 0.01f );
	ImGui::DragFloat( u8"ジャンプを入力し続けている秒数", &keepJumpSecond, 0.01f );
	ImGui::Checkbox( u8"ジャンプ入力を離したか",	&wasReleasedJumpInput );
	ImGui::Checkbox( u8"地上にいるか",			&onGround );

	ImGui::TreePop();
}
#endif // USE_IMGUI
