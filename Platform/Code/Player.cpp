#include "Player.h"

#include "Donya/Loader.h"
#include "Donya/Sound.h"
#include "Donya/Template.h"	// Use AppendVector()
#if DEBUG_MODE
#include "Donya/Useful.h"	// Use ShowMessageBox
#endif // DEBUG_MODE

#include "Bullets/Buster.h"	// Use Buster::GetLivingCount()
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
		"Slide",
		"Jump",
		"KnockBack",
		"GrabLadder",
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


int  Player::Remaining::count = 2;
int  Player::Remaining::Get() { return count; }
void Player::Remaining::Set( int v )
{
	const auto &maxCount = Parameter().Get().maxRemainCount;
	count = std::min( maxCount, v );
}
void Player::Remaining::Decrement() { count--; }

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
	constexpr size_t levelCount = scast<size_t>( Player::ShotLevel::LevelCount );
	if ( chargeSeconds.size() != levelCount )
	{
		chargeSeconds.resize( levelCount, 1.0f );
	}
	if ( ImGui::TreeNode( u8"ショット設定" ) )
	{
		fireParam.ShowImGuiNode( u8"発射情報" );
		ImGui::DragInt( u8"画面内に出せる弾数",	&maxBusterCount );

		if ( ImGui::TreeNode( u8"チャージ秒数設定" ) )
		{
			ImGui::DragFloat( u8"通常", &chargeSeconds[scast<int>( Player::ShotLevel::Normal )], 0.1f );
			chargeSeconds[scast<int>( Player::ShotLevel::Normal )] = 0.0f;

			ImGui::DragFloat( u8"強化", &chargeSeconds[scast<int>( Player::ShotLevel::Tough  )], 0.1f );
			ImGui::DragFloat( u8"最大", &chargeSeconds[scast<int>( Player::ShotLevel::Strong )], 0.1f );

			ImGui::TreePop();
		}

		maxBusterCount = std::max( 1, maxBusterCount );

		ImGui::TreePop();
	}

	if ( ImGui::TreeNode( u8"はしご設定" ) )
	{
		ImGui::Helper::ShowAABBNode( u8"はしごの継続判定", &ladderGrabArea );

		ImGui::DragFloat( u8"はしごでの移動速度",		&ladderMoveSpeed,		0.01f );
		ImGui::DragFloat( u8"はしごショット隙の秒数",	&ladderShotLagSecond,	0.01f );

		ladderMoveSpeed		= std::max( 0.01f,	ladderMoveSpeed		);
		ladderShotLagSecond	= std::max( 0.0f,	ladderShotLagSecond	);

		ImGui::TreePop();
	}

	if ( ImGui::TreeNode( u8"メータ設定" ) )
	{
		ImGui::DragFloat2( u8"ＨＰ描画位置（左上座標）",	&hpDrawPos.x	);
		ImGui::ColorEdit3( u8"ＨＰ描画色",				&hpDrawColor.x	);
		ImGui::DragFloat ( u8"ＨＰ描画スケール",			&hpDrawScale	);

		ImGui::TreePop();
	}

	ImGui::Helper::ShowAABBNode( u8"地形との当たり判定", &hitBox  );
	ImGui::Helper::ShowAABBNode( u8"攻撃との喰らい判定", &hurtBox );
	ImGui::Helper::ShowAABBNode( u8"スライド中・地形との当たり判定", &slideHitBox  );
	ImGui::Helper::ShowAABBNode( u8"スライド中・攻撃との喰らい判定", &slideHurtBox );

	if ( ImGui::TreeNode( u8"汎用設定" ) )
	{
		ImGui::DragInt  ( u8"最大体力",				&maxHP						);
		ImGui::DragInt  ( u8"最大残機数",			&maxRemainCount				);
		ImGui::DragInt  ( u8"初期残機数",			&initialRemainCount			);
		ImGui::DragFloat( u8"移動速度",				&moveSpeed,			0.01f	);
		ImGui::DragFloat( u8"スライディング速度",		&slideMoveSpeed,	0.01f	);
		ImGui::DragFloat( u8"スライディング秒数",		&slideMoveSeconds,	0.01f	);
		ImGui::DragFloat( u8"ジャンプ力",			&jumpStrength,		0.01f	);
		ImGui::DragFloat( u8"重力",					&gravity,			0.01f	);
		ImGui::SliderFloat( u8"重力抵抗力",			&gravityResistance,	0.0f, 1.0f );
		ImGui::DragFloat( u8"重力抵抗可能秒数",		&resistableSeconds,	0.01f	);
		ImGui::DragFloat( u8"最高落下速度",			&maxFallSpeed,		0.01f	);
		ImGui::DragFloat( u8"のけぞる秒数",			&knockBackSeconds,	0.01f	);
		ImGui::DragFloat( u8"のけぞり速度",			&knockBackSpeed,	0.01f	);
		ImGui::DragFloat( u8"無敵秒数",				&invincibleSeconds,	0.01f	);
		ImGui::DragFloat( u8"無敵中点滅間隔（秒）",	&flushingInterval,	0.01f	);

		auto MakePositive	= []( float *v )
		{
			*v = std::max( 0.001f, *v );
		};
		maxHP				= std::max( 1, maxHP				);
		maxRemainCount		= std::max( 1, maxRemainCount		);
		initialRemainCount	= std::max( 1, initialRemainCount	);
		MakePositive( &moveSpeed			);
		MakePositive( &slideMoveSpeed		);
		MakePositive( &slideMoveSeconds		);
		MakePositive( &jumpStrength			);
		MakePositive( &gravity				);
		MakePositive( &resistableSeconds	);
		MakePositive( &maxFallSpeed			);
		MakePositive( &knockBackSeconds		);
		MakePositive( &knockBackSpeed		);
		MakePositive( &invincibleSeconds	);
		MakePositive( &flushingInterval		);
		
		ImGui::TreePop();
	}
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
	currKind = CalcNowKind( inst, elapsedTime );
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
	case MotionKind::Slide:		return true;
	case MotionKind::Jump:		return false;
	case MotionKind::KnockBack:	return true;
	case MotionKind::GrabLadder:return true;
	default: break;
	}

	_ASSERT_EXPR( 0, L"Error: Unexpected kind!" );
	return false;
}
Player::MotionKind Player::MotionManager::CalcNowKind( Player &inst, float elapsedTime ) const
{
	// Continue same motion if the game time is pausing
	if ( IsZero( elapsedTime ) ) { return currKind; }
	// else

	if ( inst.pMover && inst.pMover->NowGrabbingLadder( inst ) )
	{ return MotionKind::GrabLadder; }
	if ( inst.pMover && inst.pMover->NowKnockBacking( inst ) )
	{ return MotionKind::KnockBack; }
	if ( inst.pMover && inst.pMover->NowSliding( inst ) )
	{ return MotionKind::Slide; }
	// else

	const bool nowMoving = IsZero( inst.velocity.x ) ? false : true;
	const bool onGround  = inst.onGround;
	
	if ( !onGround ) { return MotionKind::Jump;	}
	if ( nowMoving ) { return MotionKind::Run;	}
	// else
	return MotionKind::Idle;
}

void Player::ShotManager::Init()
{
	prevChargeSecond = 0.0f;
	currChargeSecond = 0.0f;
}
void Player::ShotManager::Update( float elapsedTime, Input input )
{
	prevChargeSecond = currChargeSecond;

	// If calculate the charge level after update the "currChargeSecond",
	// the level will be zero(ShotLevel::Normal) absolutely when fire timing, because that timing is the input was released.
	// So I must calculate it before the update. It will not be late for one frame by this.
	CalcChargeLevel();

	if ( input.useShot )
	{
		currChargeSecond += elapsedTime;
	}
	else
	{
		currChargeSecond = 0.0f;
	}
}
bool Player::ShotManager::IsShotRequested() const
{
	const bool prevIsZero = IsZero( prevChargeSecond );
	const bool currIsZero = IsZero( currChargeSecond );

	// I wanna fire a charge shot by release,
	// but I don't wanna fire the normal shot by release.
	const bool nowHighLevel = ( chargeLevel != ShotLevel::Normal && chargeLevel != ShotLevel::LevelCount );

	return ( prevIsZero && !currIsZero )					// Now triggered a button
		|| ( !prevIsZero && currIsZero && nowHighLevel );	// Now released a button, and now charging at least one.
}
void Player::ShotManager::CalcChargeLevel()
{
	const auto &data = Parameter().Get();

	constexpr std::array<ShotLevel, 2> compareHighLevels
	{
		ShotLevel::Strong,
		ShotLevel::Tough,
	};

	if ( data.chargeSeconds.size() < compareHighLevels.size() )
	{
		chargeLevel = ShotLevel::Normal;
	}
	else
	{
		bool assigned = false;
		for ( const auto &it : compareHighLevels )
		{
			if ( data.chargeSeconds[scast<int>( it )] <= currChargeSecond )
			{
				chargeLevel = it;
				assigned = true;
				break;
			}
		}
		if ( !assigned ) { chargeLevel = ShotLevel::Normal; }
	}
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

void Player::MoverBase::Init( Player &inst )
{
	AssignBodyParameter( inst );
}
void Player::MoverBase::MotionUpdate( Player &inst, float elapsedTime )
{
	inst.motionManager.Update( inst, elapsedTime );
}
void Player::MoverBase::MoveOnlyHorizontal( Player &inst, float elapsedTime, const Map &terrain, float roomLeftBorder, float roomRightBorder )
{
	const auto movement		= inst.velocity * elapsedTime;
	const auto aroundSolids	= inst.FetchAroundSolids( inst.GetHitBox(), movement, terrain );
	inst.Actor::MoveX( movement.x, aroundSolids );
	inst.Actor::MoveZ( movement.z, aroundSolids );

	// Clamp into room
	{
		const float outsideLengthL = roomLeftBorder - inst.body.Min().x;
		if ( 0.0f < outsideLengthL )
		{
			inst.body.pos.x += outsideLengthL;
		}
		const float outsideLengthR = inst.body.Max().x - roomRightBorder;
		if ( 0.0f < outsideLengthR )
		{
			inst.body.pos.x -= outsideLengthR;
		}
	}

	// We must apply world position to hurt box also
	inst.hurtBox.pos = inst.body.pos;

	// inst.KillMeIfCollideToKillAreas( elapsedTime, terrain );
}
void Player::MoverBase::MoveOnlyVertical( Player &inst, float elapsedTime, const Map &terrain )
{
	const auto movement		= inst.velocity * elapsedTime;
	const auto aroundSolids	= inst.FetchAroundSolids( inst.GetHitBox(), movement, terrain );
	const int  collideIndex	= inst.Actor::MoveY( movement.y, aroundSolids );
	if ( collideIndex != -1 ) // If collided to any
	{
		if ( inst.velocity.y <= 0.0f )
		{
			inst.Landing();
		}

		inst.velocity.y = 0.0f;
	}
	else if ( !IsZero( movement.y ) ) // Should not change if the movement is none
	{
		inst.onGround = false;
	}

	// We must apply world position to hurt box also
	inst.hurtBox.pos = inst.body.pos;

	// inst.KillMeIfCollideToKillAreas( elapsedTime, terrain );
}
void Player::MoverBase::AssignBodyParameter( Player &inst )
{
	inst.body		= inst.GetNormalBody( /* ofHurtBox = */ false );
	inst.hurtBox	= inst.GetNormalBody( /* ofHurtBox = */ true  );
}

void Player::Normal::Update( Player &inst, float elapsedTime, Input input, const Map &terrain )
{
	inst.MoveHorizontal( elapsedTime, input );

	// Deformity of MoveVertical()
	{
		if ( input.useJump && inst.Jumpable() && !IsZero( elapsedTime ) ) // Make to can not act if game time is pausing
		{
			if ( Donya::SignBit( input.moveVelocity.y ) < 0 )
			{
				gotoSlide = true;

				// Certainly doing Fall() if do not jump
				inst.Fall( elapsedTime, input );
				// But We must set the pressing flag because We wanna prevent to jump.
				inst.wasReleasedJumpInput = false;
			}
			else
			{
				inst.Jump();
			}
		}
		else
		{
			inst.Fall( elapsedTime, input );
		}
	}

	inst.ShotIfRequested( elapsedTime, input );

	// Try to grabbing ladder if the game time is not pausing
	if ( !gotoSlide && !IsZero( elapsedTime ) )
	{
		auto GotoLadderIfSpecifyTileIsLadder = [&]( const Donya::Vector3 &wsVerifyPosition )
		{
			const auto targetTile = terrain.GetPlaceTileOrNullptr( wsVerifyPosition );
			if ( targetTile && targetTile->GetID() == StageFormat::Ladder )
			{
				inst.pTargetLadder = targetTile;
				gotoLadder = true;
			}
		};

		const int verticalInputSign = Donya::SignBit( input.moveVelocity.y );
		if ( verticalInputSign == 1 )
		{
			const auto centerPos = inst.GetPosition();
			GotoLadderIfSpecifyTileIsLadder( centerPos );
			if ( !gotoLadder )
			{
				const Donya::Vector3 headOffset{ 0.0f, inst.body.size.y, 0.0f };
				GotoLadderIfSpecifyTileIsLadder( centerPos + headOffset );
			}
		}
		else
		if ( verticalInputSign == -1 && inst.onGround )
		{
			constexpr Donya::Vector3 verticalOffset{ 0.0f, Tile::unitWholeSize, 0.0f };
			const auto underPosition = inst.GetPosition() - verticalOffset;
			GotoLadderIfSpecifyTileIsLadder( underPosition );
		}
	}

	MotionUpdate( inst, elapsedTime );
}
void Player::Normal::Move( Player &inst, float elapsedTime, const Map &terrain, float roomLeftBorder, float roomRightBorder )
{
	MoveOnlyHorizontal( inst, elapsedTime, terrain, roomLeftBorder, roomRightBorder );
	MoveOnlyVertical  ( inst, elapsedTime, terrain );
}
bool Player::Normal::ShouldChangeMover( const Player &inst ) const
{
	return ( gotoSlide || gotoLadder );
}
std::function<void()> Player::Normal::GetChangeStateMethod( Player &inst ) const
{
	if ( gotoSlide )
	{
		return [&inst]() { inst.AssignMover<Slide>(); };
	}
	if ( gotoLadder )
	{
		return [&inst]() { inst.AssignMover<GrabLadder>(); };
	}
	// else
	return []() {}; // No op
}

void Player::Slide::Init( Player &inst )
{
	MoverBase::Init( inst );

	nextStatus	= Destination::None;
	timer		= 0.0f;

	slideSign	= Donya::SignBitF( inst.orientation.LocalFront().x );
	if ( IsZero( slideSign ) ) { slideSign = 1.0f; } // Fail safe

	inst.velocity.x = Parameter().Get().slideMoveSpeed * slideSign;
//	inst.velocity.y = 0.0f; // We must not erase the Y velocity for keeping a landing by gravity continuously.
	inst.velocity.z = 0.0f;
	inst.UpdateOrientation( /* lookingRight = */ ( slideSign < 0.0f ) ? false : true );
}
void Player::Slide::Uninit( Player &inst )
{
	MoverBase::Uninit( inst );

	inst.velocity.x = 0.0f;
}
void Player::Slide::Update( Player &inst, float elapsedTime, Input input, const Map &terrain )
{
	timer += elapsedTime;

	const int  horizontalInputSign	= Donya::SignBit( input.moveVelocity.x );
	const bool moveToBackward		=  ( horizontalInputSign != 0 ) && ( horizontalInputSign != Donya::SignBit( slideSign ) );
	const bool slideIsEnd			=  ( Parameter().Get().slideMoveSeconds <= timer )
									|| ( moveToBackward )
									|| ( !inst.onGround )
									|| ( input.useJump && inst.Jumpable() )
									;
	if ( slideIsEnd && !IsZero( elapsedTime ) ) // Make to can not act if game time is pausing
	{
		const Donya::Collision::Box3F normalBody = inst.GetNormalBody( /* ofHurtBox = */ false );
		
		if ( inst.WillCollideToAroundTiles( normalBody, inst.velocity * elapsedTime, terrain ) )
		{
			// I can not finish the sliding now, because I will be buried.
			if ( moveToBackward )
			{
				inst.velocity.x	*= -1.0f;
				slideSign		*= -1.0f;

				const auto halfTurn = Donya::Quaternion::Make( Donya::Vector3::Up(), ToRadian( 180.0f ) );
				inst.orientation.RotateBy( halfTurn );
				inst.orientation.Normalize();
			}
		}
		else
		{
			// I can finish the sliding here.
			nextStatus = Destination::Normal;
		}
	}

	// If the jump was triggered in here, the "slideIsEnd" is also true.
	inst.MoveVertical( elapsedTime, input );

	MotionUpdate( inst, elapsedTime );
}
void Player::Slide::Move( Player &inst, float elapsedTime, const Map &terrain, float roomLeftBorder, float roomRightBorder )
{
#if 0 // Prevent to fall into a tight hole
	// Horizontal move
	{
		const auto movement		= inst.velocity * elapsedTime;
		const auto aroundTiles	= terrain.GetPlaceTiles( inst.GetHitBox(), movement );
		const auto aroundSolids	= Map::ToAABB( aroundTiles );
		const int  collideIndex = inst.Actor::MoveX( movement.x, aroundSolids );
		inst.Actor::MoveZ( movement.z, aroundSolids );

		// Prevent falling into a hole that size is almost the same as my body.
		// e.g. If my body size is one tile, the sliding move is not fall into a space that one tile size.
		// But if I was pushed back by some solid when above process, I should prioritize that resolution.
		if ( collideIndex != -1 ) // If do not collided to any
		{
			// I will fall if the under tile is empty, and the left and right tiles is tile.
			
			const auto myBody = inst.GetHitBox(); // It is moved by above process
			const Donya::Vector3 center = myBody.WorldPosition();

			Donya::Vector3 search = center;
			search.y -= myBody.size.y;

			const auto underTile = terrain.GetPlaceTileOrNullptr( search );

			if ( !underTile )
			{
				const float centerX = search.x;

				search.x = centerX - myBody.size.x;
				const auto leftIndex  = Map::ToTilePos( search );

				search.x = centerX + myBody.size.x;
				const auto rightIndex = Map::ToTilePos( search );

				if ( leftIndex == rightIndex )
				{
					// Prevent falling by more move
					float	gapLength		= 0.0f;
					int		horizontalSign	= Donya::SignBit( movement.x );

					// Calculate the "gapLength"
					{
						const auto baseIndex = leftIndex;

						constexpr float halfTileSize = Tile::unitWholeSize * 0.5f;
						auto CalcRightDiffLength = [&]()
						{
							const float rightWall = Map::ToWorldPos( 0, baseIndex.x + 1 ).x - halfTileSize;
							const float rightBody = centerX + myBody.size.x;
							return fabsf( rightWall - rightBody );
						};
						auto CalcLeftDiffLength  = [&]()
						{
							const float leftWall = Map::ToWorldPos( 0, baseIndex.x - 1 ).x + halfTileSize;
							const float leftBody = centerX - myBody.size.x;
							return fabsf( leftWall - leftBody );
						};

						if ( horizontalSign == 1 )
						{
							gapLength = CalcLeftDiffLength();
						}
						else
						if ( horizontalSign == -1 )
						{
							gapLength = CalcRightDiffLength();
						}
						else
						{
							const float right = CalcRightDiffLength();
							const float left  = CalcLeftDiffLength();
							if ( left < right )
							{
								gapLength = left;
								horizontalSign = -1;
							}
							else
							{
								gapLength = right;
								horizontalSign = 1;
							}
						}
					}

					Donya::Vector3 extraMovement
					{
						gapLength * scast<float>( horizontalSign ),
						0.0f,
						0.0f
					};
					const auto exAroundTiles  = terrain.GetPlaceTiles( myBody, extraMovement );
					const auto exAroundSolids = Map::ToAABB( exAroundTiles );
					inst.Actor::MoveX( extraMovement.x, exAroundSolids );
				}
			}
		}

		// We must apply world position to hurt box also
		inst.hurtBox.pos = inst.body.pos;
	}
#else
	MoveOnlyHorizontal( inst, elapsedTime, terrain, roomLeftBorder, roomRightBorder );
#endif // Prevent to fall into a tight hole

	MoveOnlyVertical  ( inst, elapsedTime, terrain );
}
bool Player::Slide::ShouldChangeMover( const Player &inst ) const
{
	return ( nextStatus != Destination::None );
}
std::function<void()> Player::Slide::GetChangeStateMethod( Player &inst ) const
{
	switch ( nextStatus )
	{
	case Destination::Normal: return [&inst]() { inst.AssignMover<Normal>(); };
	default: break;
	}

	return [&inst]() { inst.AssignMover<Normal>(); };
}
void Player::Slide::AssignBodyParameter( Player &inst )
{
	inst.body		= inst.GetSlidingBody( /* ofHurtBox = */ false );
	inst.hurtBox	= inst.GetSlidingBody( /* ofHurtBox = */ true  );
}

void Player::GrabLadder::Init( Player &inst )
{
	MoverBase::Init( inst );

	releaseWay		= ReleaseWay::None;
	shotLagSecond	= 0.0f;
	lookingSign		= Donya::SignBitF( inst.orientation.LocalFront().x );
	if ( IsZero( lookingSign ) ) { lookingSign = 1.0f; } // Fail safe

	inst.velocity	= 0.0f;
	LookToFront( inst );
	inst.onGround	= false;

	// Adjust the position into a ladder
	{
		std::shared_ptr<const Tile> pLadder = inst.pTargetLadder.lock();

		// X axis
		{
			float centerPosX = 0.0f;
			if ( pLadder )
			{
				centerPosX = pLadder->GetPosition().x;
			}
			else // Fail safe
			{
				// Convert to the center of the current tile, then assign it

				const auto tileIndex	= Map::ToTilePos ( inst.GetPosition() );
				const auto tileCenter	= Map::ToWorldPos( tileIndex.y, tileIndex.x );
				centerPosX = tileCenter.x;
			}

			inst.body.pos.x =  centerPosX;
			inst.body.pos.x -= inst.body.offset.x; // Make to the WorldPosition().x become to tileCenter.x
		}

		// Y axis
		{
			float limitTopY		= 0.0f;
			float limitDownY	= 0.0f;
			if ( pLadder )
			{
				const auto ladderBody = pLadder->GetHitBox();
				limitTopY	= ladderBody.Max().y;
				limitDownY	= ladderBody.Min().y;
			}
			else // Fail safe
			{
				const auto myBody = inst.GetHitBox();
				limitTopY	= myBody.Max().y;
				limitDownY	= myBody.Min().y;
			}

			constexpr float margin = 0.001f; // Make as: the Max/Min pos != limitTopY/limitDownY
			const float adjustToDown	= std::max( 0.0f, ( grabArea.Max().y - limitTopY  ) + margin );
			const float adjustToUp		= std::max( 0.0f, ( limitDownY - grabArea.Min().y ) + margin );
			inst.body.pos.y -= adjustToDown;
			inst.body.pos.y += adjustToUp;
		}


		// We must apply world position to other boxes also
		inst.hurtBox.pos	= inst.body.pos;
		grabArea.pos		= inst.body.pos;
	}

	inst.pTargetLadder.reset();
}
void Player::GrabLadder::Uninit( Player &inst )
{
	MoverBase::Uninit( inst );
	
	inst.velocity = 0.0f;
	inst.pTargetLadder.reset();
	inst.UpdateOrientation( /* lookingRight = */ ( lookingSign < 0.0f ) ? false : true );

	if ( releaseWay == ReleaseWay::Climb )
	{
		// Adjust the position onto a ladder

		Donya::Vector3 grabPos = grabArea.WorldPosition();
		grabPos.y += grabArea.size.y;

		const auto  tileIndex	= Map::ToTilePos ( grabPos );
		const auto  tileCenter	= Map::ToWorldPos( tileIndex.y, tileIndex.x );
		const float tileFoot	= tileCenter.y - ( Tile::unitWholeSize * 0.5f );
		const float myFoot		= inst.GetHitBox().Min().y;

		const float adjustmentOnLadder = tileFoot - myFoot;
		inst.body.pos.y += adjustmentOnLadder;
		constexpr float margin = 0.001f; // Make as: my foot != the ladder top
		inst.body.pos.y += margin * Donya::SignBitF( adjustmentOnLadder );

		// We must apply world position to other boxes also
		inst.hurtBox.pos	= inst.body.pos;
		grabArea.pos		= inst.body.pos;

		inst.Landing();
	}
}
void Player::GrabLadder::Update( Player &inst, float elapsedTime, Input input, const Map &terrain )
{
	inst.velocity = 0.0f;

	if ( NowUnderShotLag() )
	{
		shotLagSecond -= elapsedTime;
		std::max( 0.0f, shotLagSecond );
	}

	const auto &data = Parameter().Get();

	if ( !NowUnderShotLag() )
	{
		inst.velocity.y = data.ladderMoveSpeed * input.moveVelocity.y;
	}

	ShotProcess( inst, elapsedTime, input );

	releaseWay = JudgeWhetherToRelease( inst, elapsedTime, input, terrain );

	MotionUpdate( inst, elapsedTime * Donya::SignBitF( inst.velocity.y ) );
}
void Player::GrabLadder::Move( Player &inst, float elapsedTime, const Map &terrain, float roomLeftBorder, float roomRightBorder )
{
	MoveOnlyHorizontal( inst, elapsedTime, terrain, roomLeftBorder, roomRightBorder );
	MoveOnlyVertical  ( inst, elapsedTime, terrain );

	// We must apply world position to other boxes also
	inst.hurtBox.pos	= inst.body.pos;
	grabArea.pos		= inst.body.pos;
}
bool Player::GrabLadder::ShouldChangeMover( const Player &inst ) const
{
	return ( releaseWay != ReleaseWay::None ) ? true : false;
}
std::function<void()> Player::GrabLadder::GetChangeStateMethod( Player &inst ) const
{
	return [&inst]() { inst.AssignMover<Normal>(); };
}
void Player::GrabLadder::AssignBodyParameter( Player &inst )
{
	// Use normal body
	MoverBase::AssignBodyParameter( inst );
	grabArea = inst.GetLadderGrabArea();
}
void Player::GrabLadder::LookToFront( Player &inst )
{
	// Initial orientation looks to the front
	inst.orientation = Donya::Quaternion::Identity();
}
bool Player::GrabLadder::NowUnderShotLag() const
{
	return ( 0.0f < shotLagSecond ) ? true : false;
}
void Player::GrabLadder::ShotProcess( Player &inst, float elapsedTime, Input input )
{
	const bool wantShot = ( inst.shotManager.IsShotRequested() && inst.NowShotable() );
	if ( !wantShot ) { return; }
	// else

	const auto &data = Parameter().Get();
	shotLagSecond = data.ladderShotLagSecond;

	const int sideInputSign = Donya::SignBit( input.moveVelocity.x );
	if ( sideInputSign != 0 )
	{
		lookingSign = scast<float>( sideInputSign );
	}

	// Update the orientation temporarily for decide the shot direction
	inst.UpdateOrientation( /* lookingRight = */ ( lookingSign < 0.0f ) ? false : true );
	inst.ShotIfRequested( elapsedTime, input );
	LookToFront( inst );
}
Player::GrabLadder::ReleaseWay Player::GrabLadder::JudgeWhetherToRelease( Player &inst, float elapsedTime, Input input, const Map &terrain ) const
{
	if ( releaseWay != ReleaseWay::None ) { return releaseWay; }
	// else

	if ( inst.onGround )
	{
		return ReleaseWay::Dismount;
	}
	// else

	if ( !input.useJump ) { inst.wasReleasedJumpInput = true; }
	if ( input.useJump && inst.wasReleasedJumpInput && !IsZero( elapsedTime ) )
	{
		inst.wasReleasedJumpInput = false; // Prevent the grab-release loop by press keeping the jump and the up input
		return ReleaseWay::Release;
	}
	// else

	bool onNotLadder = false;
	const auto grabbingTiles = terrain.GetPlaceTiles( grabArea );
	for ( const auto &pIt : grabbingTiles )
	{
		if ( !pIt || ( pIt && pIt->GetID() != StageFormat::Ladder ) )
		{
			onNotLadder = true;
			break;
		}
	}
	if ( onNotLadder )
	{
		const auto myBody = inst.GetHitBox();
		const auto myHead = myBody.WorldPosition() + Donya::Vector3{ 0.0f, myBody.size.y, 0.0f };
		const auto myFoot = myBody.WorldPosition() - Donya::Vector3{ 0.0f, myBody.size.y, 0.0f };

		const auto topTile		= terrain.GetPlaceTileOrNullptr( myHead );
		const auto downTile		= terrain.GetPlaceTileOrNullptr( myFoot );
		const bool topIsLadder	= ( topTile  && topTile ->GetID() == StageFormat::Ladder );
		const bool downIsLadder	= ( downTile && downTile->GetID() == StageFormat::Ladder );

		if ( topIsLadder  ) { return ReleaseWay::Dismount;	}
		if ( downIsLadder ) { return ReleaseWay::Climb;		}
		// else
		return ReleaseWay::Release;
	}
	
	return releaseWay;
}

void Player::KnockBack::Init( Player &inst )
{
	MoverBase::Init( inst );

	const bool knockedFromRight = ( inst.pReceivedDamage && inst.pReceivedDamage->knockedFromRight );
	inst.UpdateOrientation( knockedFromRight );

	if ( !inst.prevSlidingStatus )
	{
		const float impulseSign = ( knockedFromRight ) ? -1.0f : 1.0f;
		inst.velocity.x = Parameter().Get().knockBackSpeed * impulseSign;
	}
	
	timer = 0.0f;
}
void Player::KnockBack::Uninit( Player &inst )
{
	MoverBase::Uninit( inst );

	inst.velocity.x = 0.0f;
}
void Player::KnockBack::Update( Player &inst, float elapsedTime, Input input, const Map &terrain )
{
	timer += elapsedTime;

	Input emptyInput{}; // Discard the input for a resistance of gravity.
	inst.Fall( elapsedTime, emptyInput );

	MotionUpdate( inst, elapsedTime );
}
void Player::KnockBack::Move( Player &inst, float elapsedTime, const Map &terrain, float roomLeftBorder, float roomRightBorder )
{
	MoveOnlyHorizontal( inst, elapsedTime, terrain, roomLeftBorder, roomRightBorder );
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
	inst.currentHP		= 0;
	inst.onGround		= false;

	Donya::Sound::Play( Music::Player_Miss );
}
void Player::Miss::Update( Player &inst, float elapsedTime, Input input, const Map &terrain )
{
	// Overwrite forcely
	inst.body.exist		= false;
	inst.hurtBox.exist	= false;
}
void Player::Miss::Move( Player &inst, float elapsedTime, const Map &terrain, float roomLeftBorder, float roomRightBorder )
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
	shotManager.Init();
	currentHP			= data.maxHP;
	prevSlidingStatus	= false;
	onGround			= false;

	const float gaugeAmount = scast<float>( data.maxHP );
	hpDrawer.Init( gaugeAmount, gaugeAmount );
	hpDrawer.SetDrawOption( data.hpDrawPos, data.hpDrawColor, data.hpDrawScale );
	
	UpdateOrientation( initializer.ShouldLookingRight() );

	AssignMover<Normal>();
}
void Player::Uninit()
{
	if ( pMover ) { pMover->Uninit( *this ); }
	pTargetLadder.reset();
}
void Player::Update( float elapsedTime, Input input, const Map &terrain )
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

		if ( prevSlidingStatus )
		{
			ApplyExceptPosition( &body,		data.slideHitBox  );
			ApplyExceptPosition( &hurtBox,	data.slideHurtBox );
		}
		else
		{
			ApplyExceptPosition( &body,		data.hitBox  );
			ApplyExceptPosition( &hurtBox,	data.hurtBox );
		}

		hpDrawer.SetDrawOption( data.hpDrawPos, data.hpDrawColor, data.hpDrawScale );
	}
#endif // USE_IMGUI

	hurtBox.UpdateIgnoreList( elapsedTime );

	shotManager.Update( elapsedTime, input );

	if ( !pMover )
	{
		_ASSERT_EXPR( 0, L"Error: Player's mover is not assigned!" );
		return;
	}
	// else

	invincibleTimer.Update( elapsedTime );
	ApplyReceivedDamageIfHas( elapsedTime, terrain );

	hurtBox.exist = !invincibleTimer.NowWorking();

	hpDrawer.SetCurrent( scast<float>( currentHP ) );
	hpDrawer.Update( elapsedTime );

	pMover->Update( *this, elapsedTime, input, terrain );
	if ( pMover->ShouldChangeMover( *this ) )
	{
		auto ChangeMethod = pMover->GetChangeStateMethod( *this );
		ChangeMethod();
	}

	prevSlidingStatus = pMover->NowSliding( *this );
}
void Player::PhysicUpdate( float elapsedTime, const Map &terrain, float roomLeftBorder, float roomRightBorder )
{
	if ( !pMover )
	{
		_ASSERT_EXPR( 0, L"Error: Player's mover is not assigned!" );
		return;
	}
	// else

	pMover->Move( *this, elapsedTime, terrain, roomLeftBorder, roomRightBorder );
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
	constexpr Donya::Vector4 laddColor{ 1.0f, 0.3f, 0.0f, 0.6f };
	const auto  &hurt		= hurtBox;
	const auto  ladd		= GetLadderGrabArea();
	const float bodyNear	= body.WorldPosition().z - body.size.z;
	const float hurtNear	= hurt.WorldPosition().z - hurt.size.z;
	const float laddNear	= ladd.WorldPosition().z - ladd.size.z;
	// Drawing the far box first
	if ( bodyNear < hurtNear )
	{
		if ( laddNear < bodyNear )
		{
			DrawProcess( hurt, hurtColor );
			DrawProcess( body, bodyColor );
			DrawProcess( ladd, laddColor );
		}
		else
		{
			DrawProcess( ladd, laddColor );
			DrawProcess( hurt, hurtColor );
			DrawProcess( body, bodyColor );
		}
	}
	else
	{
		if ( laddNear < hurtNear )
		{
			DrawProcess( body, bodyColor );
			DrawProcess( hurt, hurtColor );
			DrawProcess( ladd, laddColor );
		}
		else
		{
			DrawProcess( ladd, laddColor );
			DrawProcess( body, bodyColor );
			DrawProcess( hurt, hurtColor );
		}
	}
#endif // DEBUG_MODE
}
void Player::DrawMeter( float drawDepth ) const
{
	hpDrawer.Draw( drawDepth );
}
bool Player::NowMiss() const
{
	return ( !pMover ) ? true : ( pMover->NowMiss( *this ) ) ? true : false;
}
bool Player::NowGrabbingLadder() const
{
	return ( pMover && pMover->NowGrabbingLadder( *this ) ) ? true : false;
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
bool Player::WillDie() const
{
	if ( !pReceivedDamage ) { return false; }

	return ( currentHP - pReceivedDamage->damage.amount <= 0 );
}
void Player::KillMe()
{
	AssignMover<Miss>();
}
void Player::KillMeIfCollideToKillAreas( float elapsedTime, const Map &terrain )
{
	if ( invincibleTimer.NowWorking() || NowMiss() ) { return; }
	if ( pReceivedDamage ) { return; } // If now receiving damage, I prioritize that.
	// else

	const auto nowBody   = GetHitBox();
	const auto killAreas = FetchAroundKillAreas( nowBody, velocity * elapsedTime, terrain );
	for ( const auto &it : killAreas )
	{
		if ( Donya::Collision::IsHit( nowBody, it ) )
		{
			KillMe();
			return;
		}
	}
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
void Player::ApplyReceivedDamageIfHas( float elapsedTime, const Map &terrain )
{
	if ( !pReceivedDamage ) { return; }
	// else

	// Reject if now invincible
	if ( invincibleTimer.NowWorking() )
	{
		pReceivedDamage.reset();
		return;
	}
	// else

	currentHP -= pReceivedDamage->damage.amount;
	if ( currentHP <= 0 )
	{
		KillMe();
	}
	else
	{
		invincibleTimer.Start( Parameter().Get().invincibleSeconds );

		if ( prevSlidingStatus )
		{
			// If now throughing under a tile, I do not knock-backing.
			// Else do knock-backing but do not push back the position(it will process in KnockBack::Init()).
			if ( !WillCollideToAroundTiles( GetNormalBody( /* ofHurtBox = */ false ), velocity * elapsedTime, terrain ) )
			{
				AssignMover<KnockBack>();
			}
		}
		else
		{
			AssignMover<KnockBack>();
		}

		Donya::Sound::Play( Music::Player_Damage );
	}

	pReceivedDamage.reset();
}
void Player::AssignBodyInfo( Donya::Collision::Box3F *p, bool useHurtBox ) const
{
	p->pos			= ( useHurtBox ) ? hurtBox.pos			: body.pos;
	p->id			= ( useHurtBox ) ? hurtBox.id			: body.id;
	p->ownerID		= ( useHurtBox ) ? hurtBox.ownerID		: body.ownerID;
	p->ignoreList	= ( useHurtBox ) ? hurtBox.ignoreList	: body.ignoreList;
	p->exist		= ( useHurtBox ) ? hurtBox.exist		: body.exist;
}
Donya::Collision::Box3F Player::GetNormalBody ( bool ofHurtBox ) const
{
	const auto &data = Parameter().Get();

	Donya::Collision::Box3F tmp = ( ofHurtBox ) ? data.hurtBox : data.hitBox;
	AssignBodyInfo( &tmp, ofHurtBox );
	return tmp;
}
Donya::Collision::Box3F Player::GetSlidingBody( bool ofHurtBox ) const
{
	const auto &data = Parameter().Get();

	Donya::Collision::Box3F tmp = ( ofHurtBox ) ? data.slideHurtBox : data.slideHitBox;
	AssignBodyInfo( &tmp, ofHurtBox );
	return tmp;
}
Donya::Collision::Box3F Player::GetLadderGrabArea() const
{
	const auto &data = Parameter().Get();

	Donya::Collision::Box3F tmp = data.ladderGrabArea;
	AssignBodyInfo( &tmp, /* useHurtBoxInfo = */ false );
	return tmp;
}
std::vector<Donya::Collision::Box3F> Player::FetchAroundSolids( const Donya::Collision::Box3F &body, const Donya::Vector3 &movement, const Map &terrain ) const
{
	const auto aroundTiles = terrain.GetPlaceTiles( body, movement );
	auto aroundSolids = Map::ToAABBSolids( aroundTiles, terrain, body );
	if ( invincibleTimer.NowWorking() )
	{
		const auto aroundKillAreas = Map::ToAABBKillAreas( aroundTiles, terrain, body );
		Donya::AppendVector( &aroundSolids, aroundKillAreas );
	}
	return aroundSolids;
}
std::vector<Donya::Collision::Box3F> Player::FetchAroundKillAreas( const Donya::Collision::Box3F &body, const Donya::Vector3 &movement, const Map &terrain ) const
{
	const auto aroundTiles = terrain.GetPlaceTiles( body, movement );
	return Map::ToAABBKillAreas( aroundTiles, terrain, body );
}
bool Player::WillCollideToAroundTiles( const Donya::Collision::Box3F &body, const Donya::Vector3 &movement, const Map &terrain ) const
{
	auto aroundSolids = FetchAroundSolids( body, movement, terrain );
	if ( !invincibleTimer.NowWorking() )
	{
		const auto aroundKillAreas = FetchAroundKillAreas( body, movement, terrain );
		Donya::AppendVector( &aroundSolids, aroundKillAreas );
	}

	for ( const auto &it : aroundSolids )
	{
		if ( Donya::Collision::IsHit( body, it ) )
		{
			return true;
		}
	}

	return false;
}
void Player::MoveHorizontal( float elapsedTime, Input input )
{
	const auto &data = Parameter().Get();

	const float  movement = data.moveSpeed * input.moveVelocity.x;
	velocity.x = movement;

	if ( !IsZero( velocity.x * elapsedTime ) ) // The "elapsedTime" prevents to rotate when pauses a game time
	{
		const bool lookRight = ( 0.0f < velocity.x ) ? true : false;
		UpdateOrientation( lookRight );
	}
}
void Player::MoveVertical  ( float elapsedTime, Input input )
{
	if ( input.useJump && Jumpable() && !IsZero( elapsedTime ) ) // Make to can not act if game time is pausing
	{
		Jump();
	}
	else
	{
		Fall( elapsedTime, input );
	}
}
bool Player::NowShotable() const
{
	const bool generatable = ( Bullet::Buster::GetLivingCount() < Parameter().Get().maxBusterCount );
	return ( generatable ) ? true : false;
}
void Player::ShotIfRequested( float elapsedTime, Input input )
{
	if ( !shotManager.IsShotRequested() ) { return; }
	if ( !NowShotable() ) { return; }
	// else

	const auto &data = Parameter().Get();

	const Donya::Vector3 front = orientation.LocalFront();
	const float dot = Donya::Dot( front, Donya::Vector3::Right() );
	const float lookingSign = Donya::SignBitF( dot );

	Bullet::FireDesc desc = data.fireParam;
	desc.direction	=  Donya::Vector3::Right() * lookingSign;
	desc.position.x	*= lookingSign;
	desc.position	+= GetPosition();
	desc.owner		=  hurtBox.id;

	const ShotLevel level = shotManager.ChargeLevel();
	if ( level != ShotLevel::Normal && level != ShotLevel::LevelCount )
	{
		using Dmg = Definition::Damage;
		desc.pAdditionalDamage			= std::make_shared<Dmg>();
		desc.pAdditionalDamage->amount	= scast<int>( level );
		desc.pAdditionalDamage->type	= Dmg::Type::Pierce;
	}
		
	Bullet::Admin::Get().RequestFire( desc );
	Donya::Sound::Play( Music::Player_Shot );
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

		if ( pMover && !pMover->NowKnockBacking( *this ) && !pMover->NowGrabbingLadder( *this ) )
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

	ImGui::DragInt		( u8"現在の体力",					&currentHP );
	ImGui::Text			( u8"現在のステート：%s",				pMover->GetMoverName().c_str() );

	ImGui::Text			( u8"%04.2f: ショット長押し秒数",		shotManager.ChargeSecond() );
	ImGui::Text			( u8"%d: チャージレベル",				scast<int>( shotManager.ChargeLevel() ) );
	ImGui::DragFloat	( u8"ジャンプを入力し続けている秒数",	&keepJumpSecond, 0.01f );
	ImGui::Checkbox		( u8"ジャンプ入力を離したか",			&wasReleasedJumpInput );
	ImGui::Checkbox		( u8"地上にいるか",					&onGround );
	bool tmp{};
	tmp = pMover->NowKnockBacking( *this );
	ImGui::Checkbox		( u8"のけぞり中か",					&tmp );
	tmp = invincibleTimer.NowWorking();
	ImGui::Checkbox		( u8"無敵中か",						&tmp );

	ImGui::DragFloat3	( u8"ワールド座標",					&body.pos.x,	0.01f );
	ImGui::DragFloat3	( u8"速度",							&velocity.x,	0.01f );

	Donya::Vector3 front = orientation.LocalFront();
	ImGui::SliderFloat3	( u8"前方向",						&front.x, -1.0f, 1.0f );
	orientation = Donya::Quaternion::LookAt( Donya::Vector3::Front(), front.Unit(), Donya::Quaternion::Freeze::Up );

	ImGui::TreePop();
}
#endif // USE_IMGUI
