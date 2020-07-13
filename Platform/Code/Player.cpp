#include "Player.h"

#include "Donya/Loader.h"
#include "Donya/Sound.h"
#if DEBUG_MODE
#include "Donya/Useful.h"	// Use ShowMessageBox
#endif // DEBUG_MODE

#include "Bullets/Buster.h"	// use Buster::GetLivingCount()
#include "Common.h"
#include "FilePath.h"
#include "Map.h"			// Use Map::ToWorldPos()
#include "Music.h"
#include "Parameter.h"
#include "PlayerParam.h"
#include "StageFormat.h"

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

	static std::shared_ptr<ModelHelper::SkinningSet> pModel{};

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

		pModel = std::make_shared<ModelHelper::SkinningSet>();
		const bool result = ModelHelper::Load( filePath, pModel.get() );
		if ( !result )
		{
			pModel.reset(); // Make not loaded state

			const std::string msg = "Failed: Loading failed: " + filePath;
			Donya::OutputDebugStr( msg.c_str() );
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
	std::shared_ptr<ModelHelper::SkinningSet> GetModelOrNullptr()
	{
		return pModel;
	}
}

Donya::Vector3	PlayerInitializer::GetWorldInitialPos() const { return wsInitialPos; }
bool			PlayerInitializer::ShouldLookingRight() const { return lookingRight; }
void PlayerInitializer::LoadParameter( int stageNo )
{
#if DEBUG_MODE
	LoadJson( stageNo );
	// If a user was changed only a json file, the user wanna apply the changes to binary file also.
	// So save here.
	SaveBin( stageNo );
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
void PlayerInitializer::RemakeByCSV( const CSVLoader &loadedData )
{
	auto ShouldConsider = []( int id )
	{
		return ( id == StageFormat::StartPointRight || id == StageFormat::StartPointLeft );
	};
	auto ApplyIfNeeded  = [&]( int id, size_t r, size_t c )->bool
	{
		if ( !ShouldConsider( id ) ) { return false; }
		// else

		wsInitialPos = Map::ToWorldPos( r, c );
		lookingRight = ( id == StageFormat::StartPointRight );
		return true;
	};

	bool wasApplied = false;

#if DEBUG_MODE
	struct Cell
	{
		size_t row = 0;
		size_t column = 0;
	};
	std::vector<Cell> duplications;
	Cell appliedCell{};
#endif // DEBUG_MODE

	const auto &data = loadedData.Get();
	const size_t rowCount = data.size();
	for ( size_t r = 0; r < rowCount; ++r )
	{
		const size_t columnCount = data[r].size();
		for ( size_t c = 0; c < columnCount; ++c )
		{
		#if DEBUG_MODE
			if ( wasApplied )
			{
				if ( ShouldConsider( data[r][c] ) )
				{
					Cell tmp;
					tmp.row = r;
					tmp.column = c;
					duplications.emplace_back( std::move( tmp ) );
				}
				continue;
			}
			// else
		#endif // DEBUG_MODE

			if ( ApplyIfNeeded( data[r][c], r, c ) )
			{
				wasApplied = true;
			#if DEBUG_MODE
				appliedCell.row		= r;
				appliedCell.column	= c;
			#else
				break;
			#endif // DEBUG_MODE
			}
		}

	#if !DEBUG_MODE
		if ( wasApplied ) { break; }
	#endif // !DEBUG_MODE
	}

#if DEBUG_MODE
	if ( !duplications.empty() )
	{
		std::string msg = u8"初期位置が複数個検出されました。\n";
		msg += u8"（初めに見つかったもののみ適用されます）\n";

		auto Append = [&msg]( const Cell &cell )
		{
			auto AppendAlignment = [&msg]( size_t v )
			{
				if ( v < 100 ) { msg += u8"_"; }
				if ( v < 10  ) { msg += u8"_"; }
			};

			msg += u8"[行：";
			AppendAlignment( cell.row );
			msg += std::to_string( cell.row    );
			msg += u8"]";
			
			msg += u8"[列：";
			AppendAlignment( cell.column );
			msg += std::to_string( cell.column );
			msg += u8"]";
		};

		msg += u8"適用位置";
		Append( appliedCell );
		msg += u8"\n";

		const size_t count = duplications.size();
		for ( size_t i = 0; i < count; ++i )
		{
			msg += u8"重複" + Donya::MakeArraySuffix( i );
			Append( duplications[i] );
			msg += u8"\n";
		}

		Donya::ShowMessageBox
		(
			msg,
			"Many initial position was detected.",
			MB_ICONEXCLAMATION | MB_OK
		);
	}
#endif // DEBUG_MODE
}
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
	ImGui::DragInt  ( u8"最大体力",				&maxHP						);
	ImGui::DragFloat( u8"移動速度",				&moveSpeed,			0.01f	);
	ImGui::DragFloat( u8"ジャンプ力",			&jumpStrength,		0.01f	);
	ImGui::DragFloat( u8"重力",					&gravity,			0.01f	);
	ImGui::SliderFloat( u8"重力抵抗力",			&gravityResistance,	0.0f, 1.0f );
	ImGui::DragFloat( u8"重力抵抗可能秒数",		&resistableSeconds,	0.01f	);
	ImGui::DragFloat( u8"最高落下速度",			&maxFallSpeed,		0.01f	);
	ImGui::DragFloat( u8"のけぞる秒数",			&knockBackSeconds,	0.01f	);
	ImGui::DragFloat( u8"のけぞり速度",			&knockBackSpeed,	0.01f	);
	ImGui::DragFloat( u8"無敵秒数",				&invincibleSeconds,	0.01f	);
	ImGui::DragFloat( u8"無敵中点滅間隔（秒）",	&flushingInterval,	0.01f	);
	fireParam.ShowImGuiNode( u8"ショット設定" );
	ImGui::DragInt( u8"画面内に出せる弾数",	&maxBusterCount );
	ImGui::Helper::ShowAABBNode( u8"地形との当たり判定", &hitBox  );
	ImGui::Helper::ShowAABBNode( u8"攻撃との喰らい判定", &hurtBox );

	auto MakePositive = []( float *v )
	{
		*v = std::max( 0.001f, *v );
	};
	maxHP			= std::max( 1, maxHP			);
	MakePositive( &moveSpeed			);
	MakePositive( &jumpStrength			);
	MakePositive( &gravity				);
	MakePositive( &resistableSeconds	);
	MakePositive( &maxFallSpeed			);
	MakePositive( &knockBackSeconds		);
	MakePositive( &knockBackSpeed		);
	MakePositive( &invincibleSeconds	);
	MakePositive( &flushingInterval		);
	maxBusterCount	= std::max( 1, maxBusterCount	);
}
#endif // USE_IMGUI

void Player::MotionManager::Init()
{
	prevKind = currKind = MotionKind::Jump;

	model.Initialize( GetModelOrNullptr() );
	AssignPose( currKind );
}
void Player::MotionManager::Update( Player &inst, float elapsedTime )
{
	prevKind = currKind;
	currKind = CalcNowKind( inst );
	if ( currKind != prevKind )
	{
		model.animator.ResetTimer();
	}
	
	ShouldEnableLoop( currKind )
	? model.animator.EnableLoop()
	: model.animator.DisableLoop();

	model.animator.Update( elapsedTime );
	AssignPose( currKind );
}
void Player::MotionManager::Draw( RenderingHelper *pRenderer, const Donya::Vector4x4 &W ) const
{
	if ( !pRenderer ) { return; }
	if ( !model.pResource )
	{
		_ASSERT_EXPR( 0, L"Error: Player's model is not assigned!" );
		return;
	}
	// else

	Donya::Model::Constants::PerModel::Common modelConstant{};
	modelConstant.drawColor		= Donya::Vector4{ 1.0f, 1.0f, 1.0f, 1.0f };
	modelConstant.worldMatrix	= W;
	pRenderer->UpdateConstant( modelConstant );
	pRenderer->ActivateConstantModel();

	pRenderer->Render( model.pResource->model, model.pose );

	pRenderer->DeactivateConstantModel();
}
int  Player::MotionManager::ToMotionIndex( MotionKind kind ) const
{
	return scast<int>( kind );
}
void Player::MotionManager::AssignPose( MotionKind kind )
{
	const int motionIndex = ToMotionIndex( kind );
	if ( !model.IsAssignableIndex( motionIndex ) )
	{
		_ASSERT_EXPR( 0, L"Error: Specified motion index out of range!" );
		return;
	}
	// else

	model.AssignMotion( motionIndex );
}
bool Player::MotionManager::ShouldEnableLoop( MotionKind kind ) const
{
	switch ( kind )
	{
	case MotionKind::Idle:		return true;
	case MotionKind::Run:		return true;
	case MotionKind::Jump:		return false;
	case MotionKind::KnockBack:	return true;
	default: break;
	}

	_ASSERT_EXPR( 0, L"Error: Unexpected kind!" );
	return false;
}
Player::MotionKind Player::MotionManager::CalcNowKind( Player &inst ) const
{
	if ( inst.pMover && inst.pMover->NowKnockBacking( inst ) )
	{ return MotionKind::KnockBack; }
	// else

	const bool nowMoving = IsZero( inst.velocity.x ) ? false : true;
	const bool onGround  = inst.onGround;
	
	if ( !onGround ) { return MotionKind::Jump;	}
	if ( nowMoving ) { return MotionKind::Run;	}
	// else
	return MotionKind::Idle;
}

void Player::Flusher::Start( float flushingSeconds )
{
	workingSeconds	= flushingSeconds;
	timer			= 0.0f;
}
void Player::Flusher::Update( float elapsedTime )
{
	timer += elapsedTime;
}
bool Player::Flusher::Drawable() const
{
	if ( !NowWorking() ) { return true; }
	// else

	/*
	--- cycle
	Undrawable
	--- cycle * 0.5f
	Drawable
	--- 0.0f
	*/

	const auto &cycle = Parameter().Get().flushingInterval;
	const float remain = std::fmodf( timer, cycle );
	return ( remain < cycle * 0.5f );
}
bool Player::Flusher::NowWorking() const
{
	return ( timer < workingSeconds ) ? true : false;
}

#pragma region Mover

void Player::MoverBase::MotionUpdate( Player &inst, float elapsedTime )
{
	inst.motionManager.Update( inst, elapsedTime );
}
void Player::MoverBase::MoveOnlyHorizontal( Player &inst, float elapsedTime, const Map &terrain )
{
	const auto aroundTiles  = terrain.GetPlaceTiles( inst.GetHitBox() );
	const auto aroundSolids = Map::ToAABB( aroundTiles );
	inst.Actor::MoveX( inst.velocity.x * elapsedTime, aroundSolids );
	inst.Actor::MoveZ( inst.velocity.z * elapsedTime, aroundSolids );

	// We must apply world position to hurt box also.
	inst.hurtBox.pos = inst.body.pos;
}
void Player::MoverBase::MoveOnlyVertical( Player &inst, float elapsedTime, const Map &terrain )
{
	const auto aroundTiles  = terrain.GetPlaceTiles( inst.GetHitBox() );
	const auto aroundSolids = Map::ToAABB( aroundTiles );
	const int  collideIndex = inst.Actor::MoveY( inst.velocity.y * elapsedTime, aroundSolids );
	if ( collideIndex != -1 ) // If collided to any
	{
		if ( inst.velocity.y <= 0.0f )
		{
			inst.Landing();
		}

		inst.velocity.y = 0.0f;
	}
	else
	{
		inst.onGround = false;
	}

	// We must apply world position to hurt box also.
	inst.hurtBox.pos = inst.body.pos;
}

void Player::Normal::Update( Player &inst, float elapsedTime, Input input )
{
	inst.MoveHorizontal( elapsedTime, input );
	inst.MoveVertical( elapsedTime, input );

	inst.Shot( elapsedTime, input );

	MotionUpdate( inst, elapsedTime );
}
void Player::Normal::Move( Player &inst, float elapsedTime, const Map &terrain )
{
	MoveOnlyHorizontal( inst, elapsedTime, terrain );
	MoveOnlyVertical  ( inst, elapsedTime, terrain );
}
bool Player::Normal::ShouldChangeMover( const Player &inst ) const
{
	return false;
}
std::function<void()> Player::Normal::GetChangeStateMethod( Player &inst ) const
{
	return []() {}; // No op
}

void Player::KnockBack::Init( Player &inst )
{
	MoverBase::Init( inst );

	// TODO: I should consider the sliding status to knock-back sign

	const bool  knockedFromRight	= ( inst.pReceivedDamage && inst.pReceivedDamage->knockedFromRight );
	const float impulseSign			= ( knockedFromRight ) ? -1.0f : 1.0f;
	inst.velocity.x = Parameter().Get().knockBackSpeed * impulseSign;
	inst.UpdateOrientation( knockedFromRight );
	
	timer = 0.0f;

	Donya::Sound::Play( Music::Player_Damage );
}
void Player::KnockBack::Uninit( Player &inst )
{
	MoverBase::Uninit( inst );

	inst.velocity.x = 0.0f;
}
void Player::KnockBack::Update( Player &inst, float elapsedTime, Input input )
{
	timer += elapsedTime;

	Input emptyInput{}; // Discard the input for a resistance of gravity.
	inst.Fall( elapsedTime, emptyInput );

	MotionUpdate( inst, elapsedTime );
}
void Player::KnockBack::Move( Player &inst, float elapsedTime, const Map &terrain )
{
	MoveOnlyHorizontal( inst, elapsedTime, terrain );
	MoveOnlyVertical  ( inst, elapsedTime, terrain );
}
bool Player::KnockBack::ShouldChangeMover( const Player &inst ) const
{
	const auto &knockBackSecoonds = Parameter().Get().knockBackSeconds;
	return ( knockBackSecoonds <= timer );
}
std::function<void()> Player::KnockBack::GetChangeStateMethod( Player &inst ) const
{
	return [&inst]() { inst.AssignMover<Normal>(); };
}

void Player::Miss::Init( Player &inst )
{
	MoverBase::Init( inst );

	inst.body.exist		= false;
	inst.hurtBox.exist	= false;
	inst.velocity		= 0.0f;
	inst.onGround		= false;

	Donya::Sound::Play( Music::Player_Miss );
}
void Player::Miss::Update( Player &inst, float elapsedTime, Input input )
{
	// Overwrite forcely
	inst.body.exist		= false;
	inst.hurtBox.exist	= false;
}
void Player::Miss::Move( Player &inst, float elapsedTime, const Map &terrain )
{
	// No op
}
bool Player::Miss::Drawable( const Player &inst ) const
{
	return false;
}
bool Player::Miss::ShouldChangeMover( const Player &inst ) const
{
	return false;
}
std::function<void()> Player::Miss::GetChangeStateMethod( Player &inst ) const
{
	return []() {}; // No op
}

// region Mover
#pragma endregion

void Player::Init( const PlayerInitializer &initializer )
{
	const auto &data	= Parameter().Get();
	body				= data.hitBox;
	hurtBox				= data.hurtBox;
	hurtBox.id			= Donya::Collision::GetUniqueID();
	hurtBox.ownerID		= Donya::Collision::invalidID;
	hurtBox.ignoreList.clear();
	body.pos			= initializer.GetWorldInitialPos();
	hurtBox.pos			= body.pos;
	velocity			= 0.0f;
	motionManager.Init();
	currentHP			= data.maxHP;
	onGround			= false;
	
	UpdateOrientation( initializer.ShouldLookingRight() );

	AssignMover<Normal>();
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

	hurtBox.UpdateIgnoreList( elapsedTime );

	if ( !pMover )
	{
		_ASSERT_EXPR( 0, L"Error: Player's mover is not assigned!" );
		return;
	}
	// else

	if ( pReceivedDamage )
	{
		currentHP -= pReceivedDamage->damage.amount;
		if ( currentHP <= 0 )
		{
			AssignMover<Miss>();
		}
		else
		{
			invincibleTimer.Start( Parameter().Get().invincibleSeconds );
			AssignMover<KnockBack>();
		}

		pReceivedDamage.reset();
	}

	invincibleTimer.Update( elapsedTime );
	hurtBox.exist = !invincibleTimer.NowWorking();

	pMover->Update( *this, elapsedTime, input );
	if ( pMover->ShouldChangeMover( *this ) )
	{
		auto ChangeMethod = pMover->GetChangeStateMethod( *this );
		ChangeMethod();
	}
}
void Player::PhysicUpdate( float elapsedTime, const Map &terrain )
{
	if ( !pMover )
	{
		_ASSERT_EXPR( 0, L"Error: Player's mover is not assigned!" );
		return;
	}
	// else

	pMover->Move( *this, elapsedTime, terrain );
}
void Player::Draw( RenderingHelper *pRenderer ) const
{
	if ( !pRenderer ) { return; }
	if ( !pMover )
	{
		_ASSERT_EXPR( 0, L"Error: Player's mover is not assigned!" );
		return;
	}
	if ( !pMover->Drawable( *this ) || !invincibleTimer.Drawable() ) { return; }
	// else

	const Donya::Vector3   &drawPos = body.pos; // I wanna adjust the hit-box to fit for drawing model, so I don't apply the offset for the position of drawing model.
	const Donya::Vector4x4 W = MakeWorldMatrix( 1.0f, /* enableRotation = */ true, drawPos );

	motionManager.Draw( pRenderer, W );
}
void Player::DrawHitBox( RenderingHelper *pRenderer, const Donya::Vector4x4 &matVP, const Donya::Vector4 &unused ) const
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
bool Player::NowMiss() const
{
	return ( !pMover ) ? true : ( pMover->NowMiss( *this ) ) ? true : false;
}
Donya::Collision::Box3F	Player::GetHurtBox() const
{
	return hurtBox;
}
Donya::Quaternion		Player::GetOrientation() const
{
	return orientation;
}
void Player::GiveDamage( const Definition::Damage &damage, const Donya::Collision::Box3F &collidingHitBox ) const
{
	const auto myCenter		= body.WorldPosition();
	const auto otherMax		= collidingHitBox.Max();
	const auto otherMin		= collidingHitBox.Min();
	const float distLeft	= otherMin.x - myCenter.x;
	const float distRight	= otherMax.x - myCenter.x;
	GiveDamageImpl( damage, distLeft, distRight );
}
void Player::GiveDamage( const Definition::Damage &damage, const Donya::Collision::Sphere3F &collidingHitBox ) const
{
	const auto myCenter		= body.WorldPosition();
	const auto otherCenter	= collidingHitBox.WorldPosition();
	const auto otherLeft	= otherCenter.x - collidingHitBox.radius;
	const auto otherRight	= otherCenter.x + collidingHitBox.radius;
	const float distLeft	= otherLeft  - myCenter.x;
	const float distRight	= otherRight - myCenter.x;
	GiveDamageImpl( damage, distLeft, distRight );
}
void Player::GiveDamageImpl( const Definition::Damage &damage, float distLeft, float distRight ) const
{
	// Receive only smallest damage if same timing
	if ( pReceivedDamage && pReceivedDamage->damage.amount <= damage.amount ) { return; }
	// else

	bool knockedFromRight{};
	if ( 0.0f <= distLeft  ) { knockedFromRight = true;  }
	else
	if ( distRight <= 0.0f ) { knockedFromRight = false; }
	else
	{
		/*
		|	<-distLeft->	|	<-distRight->	|
		Min				myCenter				Max
		*/
		knockedFromRight = ( fabsf( distLeft ) <= fabsf( distRight ) );
	}

	if ( !pReceivedDamage )
	{
		pReceivedDamage = std::make_unique<DamageDesc>();
	}

	pReceivedDamage->damage = damage;
	pReceivedDamage->knockedFromRight = knockedFromRight;
}
void Player::MoveHorizontal( float elapsedTime, Input input )
{
	const auto &data = Parameter().Get();

	const float  movement = data.moveSpeed * input.moveVelocity.x;
	velocity.x = movement;

	if ( !IsZero( velocity.x ) )
	{
		const bool lookRight = ( 0.0f < velocity.x ) ? true : false;
		UpdateOrientation( lookRight );
	}
}
void Player::MoveVertical  ( float elapsedTime, Input input )
{
	if ( input.useJump && Jumpable() )
	{
		Jump();
	}
	else
	{
		Fall( elapsedTime, input );
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
		if ( Bullet::Buster::GetLivingCount() < data.maxBusterCount )
		{
			const Donya::Vector3 front = orientation.LocalFront();
			const float dot = Donya::Dot( front, Donya::Vector3::Right() );
			const float lookingSign = Donya::SignBitF( dot );

			Bullet::FireDesc desc = data.fireParam;
			desc.direction	=  Donya::Vector3::Right() * lookingSign;
			desc.position.x	*= lookingSign;
			desc.position	+= GetPosition();
			desc.owner		=  hurtBox.id;
		
			Bullet::Admin::Get().RequestFire( desc );
			Donya::Sound::Play( Music::Player_Shot );
		}
	}
}
void Player::UpdateOrientation( bool lookingRight )
{
	const float rotateSign = ( lookingRight ) ? 1.0f : -1.0f;
	orientation = Donya::Quaternion::Make
	(
		Donya::Vector3::Up(), ToRadian( 90.0f ) * rotateSign
	);
}
void Player::Jump()
{
	const auto &data = Parameter().Get();

	onGround				= false;
	velocity.y				= data.jumpStrength;
	keepJumpSecond			= 0.0f;
	wasReleasedJumpInput	= false;
	Donya::Sound::Play( Music::Player_Jump );
}
bool Player::Jumpable() const
{
	return ( onGround && wasReleasedJumpInput ) ? true : false;
}
void Player::Fall( float elapsedTime, Input input )
{
	const auto &data = Parameter().Get();

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
void Player::Landing()
{
	if ( !onGround )
	{
		onGround = true;

		if ( pMover && !pMover->NowKnockBacking( *this ) )
		{
			Donya::Sound::Play( Music::Player_Landing );
		}
	}

	velocity.y = 0.0f;
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

	ImGui::DragInt   ( u8"現在の体力",		&currentHP );
	ImGui::Text      ( u8"現在のステート：%s", pMover->GetMoverName().c_str() );

	ImGui::DragFloat3( u8"ワールド座標",	&body.pos.x,	0.01f );
	ImGui::DragFloat3( u8"速度",			&velocity.x,	0.01f );

	Donya::Vector3 front = orientation.LocalFront();
	ImGui::SliderFloat3( u8"前方向",		&front.x, -1.0f, 1.0f );
	orientation = Donya::Quaternion::LookAt( Donya::Vector3::Front(), front.Unit(), Donya::Quaternion::Freeze::Up );

	ImGui::DragFloat( u8"ショットを入力し続けている秒数", &keepShotSecond, 0.01f );
	ImGui::DragFloat( u8"ジャンプを入力し続けている秒数", &keepJumpSecond, 0.01f );
	ImGui::Checkbox( u8"ジャンプ入力を離したか",	&wasReleasedJumpInput );
	ImGui::Checkbox( u8"地上にいるか",			&onGround );
	bool tmp{};
	tmp = pMover->NowKnockBacking( *this );
	ImGui::Checkbox( u8"のけぞり中か",			&tmp );
	tmp = invincibleTimer.NowWorking();
	ImGui::Checkbox( u8"無敵中か",				&tmp );

	ImGui::TreePop();
}
#endif // USE_IMGUI
