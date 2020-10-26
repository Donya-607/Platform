#include "Player.h"

#include <algorithm>				// Use std::find()

#include "Effect/EffectAdmin.h"
#include "Effect/EffectKind.h"

#include "Donya/Loader.h"
#include "Donya/Sound.h"
#include "Donya/Template.h"			// Use AppendVector()
#if DEBUG_MODE
#include "Donya/Keyboard.h"
#include "Donya/Useful.h"			// Use ShowMessageBox
#endif // DEBUG_MODE

#include "Bullets/Buster.h"			// Use Buster::GetLivingCount()
#include "Bullets/SkullBullet.h"	// Use Buster::SkullShield
#include "Common.h"
#include "FilePath.h"
#include "Map.h"					// Use Map::ToWorldPos()
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
		"Jump_Rise",
		"Jump_Fall",
		"KnockBack",
		"GrabLadder",
		"Shot",
		"LadderShotLeft",
		"LadderShotRight",
		"Brace",
		"Appear",
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
void PlayerInitializer::AssignParameter( const Donya::Vector3 &wsPos, bool lookRight )
{
	wsInitialPos = wsPos;
	lookingRight = lookRight;
}
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
	Donya::Serializer tmp;
	tmp.LoadBinary( *this, MakeStageParamPathBinary( ID, stageNo ).c_str(), ID );
}
void PlayerInitializer::LoadJson( int stageNo )
{
	Donya::Serializer tmp;
	tmp.LoadJSON( *this, MakeStageParamPathJson( ID, stageNo ).c_str(), ID );
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

		lookingRight = ( id == StageFormat::StartPointRight );
		wsInitialPos = Map::ToWorldPos( r, c, /* alignToCenterOfTile = */ true );
		wsInitialPos.y -= Tile::unitWholeSize * 0.5f;
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

	Donya::Serializer tmp;
	tmp.SaveBinary( *this, filePath.c_str(), ID );
}
void PlayerInitializer::SaveJson( int stageNo )
{
	constexpr bool fromBinary = false;

	const std::string filePath = MakeStageParamPathJson( ID, stageNo );
	MakeDirectoryIfNotExists( filePath );
	MakeFileIfNotExists( filePath, fromBinary );

	Donya::Serializer tmp;
	tmp.SaveJSON( *this, filePath.c_str(), ID );
}
void PlayerInitializer::ShowImGuiNode( const std::string &nodeCaption, int stageNo, bool allowShowIONode )
{
	if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
	// else

	ImGui::DragFloat3( u8"初期のワールド座標（足元）",	&wsInitialPos.x, 0.01f );
	ImGui::Checkbox  ( u8"初期は右向きか",			&lookingRight );

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
void Player::Remaining::Increment()
{
	const auto &maxCount = Parameter().Get().maxRemainCount;
	count = std::min( maxCount, count + 1 );

	Donya::Sound::Play( Music::Player_1UP );
}

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
	if ( ImGui::TreeNode( u8"汎用設定" ) )
	{
		ImGui::DragFloat( u8"登場時の出現タイミング（秒）",&appearDelaySecond,	0.01f	);
		ImGui::DragFloat( u8"退場時の消滅タイミング（秒）",&leaveDelaySecond,	0.01f	);
		ImGui::DragInt  ( u8"最大体力",					&maxHP						);
		ImGui::DragInt  ( u8"最大残機数",				&maxRemainCount				);
		ImGui::DragInt  ( u8"初期残機数",				&initialRemainCount			);
		ImGui::DragFloat( u8"移動速度",					&moveSpeed,			0.01f	);
		ImGui::DragFloat( u8"スライディング速度",			&slideMoveSpeed,	0.01f	);
		ImGui::DragFloat( u8"スライディング秒数",			&slideMoveSeconds,	0.01f	);
		ImGui::DragFloat( u8"ジャンプ力",				&jumpStrength,		0.01f	);
		ImGui::DragFloat( u8"ジャンプの先行入力受付秒数",	&jumpBufferSecond,	0.01f	);
		ImGui::DragFloat( u8"重力",						&gravity,			0.01f	);
		ImGui::SliderFloat( u8"重力抵抗力",				&gravityResistance,	0.0f, 1.0f );
		ImGui::DragFloat( u8"重力抵抗可能秒数",			&resistableSeconds,	0.01f	);
		ImGui::DragFloat( u8"最高落下速度",				&maxFallSpeed,		0.01f	);
		ImGui::DragFloat( u8"のけぞる秒数",				&knockBackSeconds,	0.01f	);
		ImGui::DragFloat( u8"のけぞり速度",				&knockBackSpeed,	0.01f	);
		ImGui::DragFloat( u8"のけぞり抵抗時の短縮倍率",	&braceStandFactor,	0.01f	);
		ImGui::DragFloat( u8"無敵秒数",					&invincibleSeconds,	0.01f	);
		ImGui::DragFloat( u8"無敵中点滅間隔（秒）",		&flushingInterval,	0.01f	);

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
		MakePositive( &jumpBufferSecond		);
		MakePositive( &gravity				);
		MakePositive( &resistableSeconds	);
		MakePositive( &maxFallSpeed			);
		MakePositive( &knockBackSeconds		);
		MakePositive( &knockBackSpeed		);
		MakePositive( &invincibleSeconds	);
		MakePositive( &flushingInterval		);
		
		ImGui::TreePop();
	}

	constexpr size_t motionCount = scast<size_t>( Player::MotionKind::MotionCount );
	if ( animePlaySpeeds.size() != motionCount )
	{
		animePlaySpeeds.resize( motionCount, 1.0f );
	}
	if ( ImGui::TreeNode( u8"アニメーション関係" ) )
	{
		if ( ImGui::TreeNode( u8"再生速度" ) )
		{
			for ( size_t i = 0; i < motionCount; ++i )
			{
				ImGui::DragFloat( KIND_NAMES[i], &animePlaySpeeds[i], 0.01f );
			}

			ImGui::TreePop();
		}
		
		if ( ImGui::TreeNode( u8"ショットモーション設定" ) )
		{
			normalLeftArm .ShowImGuiNode( u8"通常" );
			ladderLeftArm .ShowImGuiNode( u8"はしご・左向き" );
			ladderRightArm.ShowImGuiNode( u8"はしご・右向き" );

			ImGui::TreePop();
		}

		ImGui::TreePop();
	}

	constexpr size_t levelCount = scast<size_t>( Player::ShotLevel::LevelCount );
	if ( chargeParams.size() != levelCount )
	{
		PerChargeLevel defaultArg{};
		chargeParams.resize( levelCount, defaultArg );
	}
	if ( ImGui::TreeNode( u8"チャージごとの設定" ) )
	{
		auto Show = [&]( const char *nodeCaption, PerChargeLevel *p )
		{
			if ( !ImGui::TreeNode( nodeCaption ) ) { return; }
			// else

			ImGui::DragFloat	( u8"チャージ秒数",		&p->chargeSecond,			0.01f );
			ImGui::DragFloat	( u8"発光周期（秒）",		&p->emissiveCycleSecond,	0.01f );
			ImGui::SliderFloat	( u8"最低発光値",		&p->emissiveMinBias,		0.0f, 1.0f );
			ImGui::ColorEdit3	( u8"発光色（最大時）",	&p->emissiveColor.x );
			ImGui::Helper::ShowEaseParam( u8"発光のイージング設定", &p->emissiveEaseKind, &p->emissiveEaseType );

			// It will used as denominator
			p->emissiveCycleSecond = std::max( 0.001f, p->emissiveCycleSecond );

			ImGui::TreePop();
		};

		Show( u8"通常", &chargeParams[scast<int>( Player::ShotLevel::Normal	)] );
		Show( u8"強化", &chargeParams[scast<int>( Player::ShotLevel::Tough	)] );
		Show( u8"最大", &chargeParams[scast<int>( Player::ShotLevel::Strong	)] );

		// Disable Normal because it will be applied in always
		auto &normal = chargeParams[scast<int>( Player::ShotLevel::Normal )];
		normal.chargeSecond  = 0.0f;
		normal.emissiveColor = Donya::Vector3::Zero();

		ImGui::SliderFloat( u8"発光への遷移時の補間係数", &emissiveTransFactor, 0.01f, 1.0f );

		ImGui::TreePop();
	}
	if ( ImGui::TreeNode( u8"ショット設定" ) )
	{
		fireParam.ShowImGuiNode( u8"発射情報" );
		ImGui::DragInt( u8"画面内に出せる弾数",	&maxBusterCount );
		maxBusterCount = std::max( 1, maxBusterCount );

		if ( ImGui::TreeNode( u8"特殊・シールド" ) )
		{
			ImGui::DragFloat ( u8"投擲時の初速",			&shieldThrowSpeed,	0.01f );
			ImGui::DragFloat3( u8"生成位置オフセット",	&shieldPosOffset.x,	0.01f );

			shieldThrowSpeed = std::max( 0.0f, shieldThrowSpeed );

			ImGui::TreePop();
		}

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

	ImGui::Helper::ShowAABBNode( u8"地形との当たり判定", &hitBox  );
	ImGui::Helper::ShowAABBNode( u8"攻撃との喰らい判定", &hurtBox );
	ImGui::Helper::ShowAABBNode( u8"スライド中・地形との当たり判定", &slideHitBox  );
	ImGui::Helper::ShowAABBNode( u8"スライド中・攻撃との喰らい判定", &slideHurtBox );
}
#endif // USE_IMGUI

void Player::InputManager::Init()
{
	prev = Input::GenerateEmpty();
	curr = Input::GenerateEmpty();
	keepJumpSeconds.fill( 0.0f );
	wasReleasedJumps.fill( false );
}
void Player::InputManager::Update( float elapsedTime, const Input input )
{
	prev = curr;
	curr = input;

	for ( int i = 0; i < Input::variationCount; ++i )
	{
		float &sec = keepJumpSeconds[i];
		sec = ( curr.useJumps[i] ) ? sec + elapsedTime : 0.0f;

		if ( !curr.useJumps[i] )
		{
			wasReleasedJumps[i] = true;
		}
	}
}
int  Player::InputManager::UseJumpIndex( bool getCurrent ) const
{
	const auto &input = ( getCurrent ) ? curr : prev;

	int		minimumIndex	= -1;
	float	minimumSecond	= FLT_MAX;
	for ( int i = 0; i < Input::variationCount; ++i )
	{
		if ( !input.useJumps[i] ) { continue; }
		// else

		const float &sec	= keepJumpSeconds[i];
		if ( sec <= minimumSecond )
		{
			minimumIndex	= i;
			minimumSecond	= sec;
		}
	}

	return minimumIndex;
}
int  Player::InputManager::UseShotIndex() const
{
	for ( int i = 0; i < Input::variationCount; ++i )
	{
		if ( curr.useShots[i] )
		{
			return i;
		}
	}

	return -1;
}
int  Player::InputManager::UseDashIndex() const
{
	for ( int i = 0; i < Input::variationCount; ++i )
	{
		if ( curr.useDashes[i] && !prev.useDashes[i] )
		{
			return i;
		}
	}

	return -1;
}
int  Player::InputManager::ShiftGunIndex() const
{
	for ( int i = 0; i < Input::variationCount; ++i )
	{
		if ( curr.shiftGuns[i] && !prev.shiftGuns[i] )
		{
			return i;
		}
	}

	return -1;
}
bool Player::InputManager::UseJump( bool getCurrent ) const
{
	return ( 0 <= UseJumpIndex( getCurrent ) );
}
bool Player::InputManager::UseShot() const
{
	return ( 0 <= UseShotIndex() );
}
bool Player::InputManager::UseDash() const
{
	return ( 0 <= UseDashIndex() );
}
int  Player::InputManager::ShiftGun() const
{
	const int index = ShiftGunIndex();
	return (  index < 0 ) ? 0 : curr.shiftGuns[index];
}
bool Player::InputManager::Jumpable( int jumpInputIndex ) const
{
	if ( jumpInputIndex < 0 || Input::variationCount <= jumpInputIndex ) { return false; }
	// else

	const auto &data = Parameter().Get();
	if ( data.jumpBufferSecond < keepJumpSeconds[jumpInputIndex] ) { return false; }
	// else

	return wasReleasedJumps[jumpInputIndex];
}
void Player::InputManager::Overwrite( const Input overwrite )
{
	curr = overwrite;
}
void Player::InputManager::OverwritePrevious( const Input overwrite )
{
	prev = overwrite;
}
#if USE_IMGUI
void Player::InputManager::ShowImGuiNode( const std::string &nodeCaption )
{
	if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
	// else

	auto ShowInput = []( const std::string &nodeCaption, Input *p )
	{
		if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
		// else

		ImGui::SliderFloat2( u8"スティック", &p->moveVelocity.x, -1.0f, 1.0f );

		for ( int i = 0; i < Input::variationCount; ++i )
		{
			const std::string caption = u8"useJumps" + Donya::MakeArraySuffix( i );
			ImGui::Checkbox(  caption.c_str(), &p->useJumps[i] );
			ImGui::SameLine();
		}
		ImGui::Text( u8"" );

		for ( int i = 0; i < Input::variationCount; ++i )
		{
			const std::string caption = u8"useShots" + Donya::MakeArraySuffix( i );
			ImGui::Checkbox(  caption.c_str(), &p->useShots[i] );
			ImGui::SameLine();
		}
		ImGui::Text( u8"" );
		
		for ( int i = 0; i < Input::variationCount; ++i )
		{
			const std::string caption = u8"useDashes" + Donya::MakeArraySuffix( i );
			ImGui::Checkbox(  caption.c_str(), &p->useDashes[i] );
			ImGui::SameLine();
		}
		ImGui::Text( u8"" );

		ImGui::TreePop();
	};
	ShowInput( u8"前回のフレーム", &prev );
	ShowInput( u8"現在のフレーム", &curr );

	for ( int i = 0; i < Input::variationCount; ++i )
	{
		const std::string caption = u8"ジャンプ長押し秒数" + Donya::MakeArraySuffix( i ) + u8":%5.3f";
		ImGui::Text( caption.c_str(), keepJumpSeconds[i] );
	}
	
	for ( int i = 0; i < Input::variationCount; ++i )
	{
		const std::string caption = u8"ジャンプ入力を離したか" + Donya::MakeArraySuffix( i ) + u8":%5.3f";
		ImGui::Checkbox( caption.c_str(), &wasReleasedJumps[i] );
	}

	ImGui::TreePop();
}
#endif // USE_IMGUI

void Player::MotionManager::Init()
{
	prevKind = currKind = MotionKind::Jump_Fall;

	model.Initialize( GetModelOrNullptr() );
	AssignPose( currKind );

	shotAnimator.ResetTimer();
	shotAnimator.DisableLoop();
	shouldPoseShot = false;
}
void Player::MotionManager::Update( Player &inst, float elapsedTime, bool stopAnimation )
{
	prevKind = currKind;
	currKind = CalcNowKind( inst, elapsedTime );
	if ( currKind != prevKind )
	{
		ResetMotionFrame();
	}
	
	ShouldEnableLoop( currKind )
	? model.animator.EnableLoop()
	: model.animator.DisableLoop();

	if ( !stopAnimation )
	{
		const auto &data = Parameter().Get();
		const size_t currentMotionIndex	= scast<size_t>( ToMotionIndex( currKind ) );
		const size_t playSpeedCount		= data.animePlaySpeeds.size();
		const float  motionAcceleration	= ( playSpeedCount <= currentMotionIndex ) ? 1.0f : data.animePlaySpeeds[currentMotionIndex];
		model.animator.Update( elapsedTime * motionAcceleration );
	}
	AssignPose( currKind );

	UpdateShotMotion( inst, elapsedTime );
}
void Player::MotionManager::Draw( RenderingHelper *pRenderer, const Donya::Vector4x4 &W, const Donya::Vector3 &color, float alpha ) const
{
	if ( !pRenderer ) { return; }
	if ( !model.pResource )
	{
		_ASSERT_EXPR( 0, L"Error: Player's model is not assigned!" );
		return;
	}
	// else

	Donya::Model::Constants::PerModel::Common modelConstant{};
	modelConstant.drawColor		= Donya::Vector4{ color, alpha };
	modelConstant.worldMatrix	= W;
	pRenderer->UpdateConstant( modelConstant );
	pRenderer->ActivateConstantModel();

	pRenderer->Render( model.pResource->model, model.pose );

	pRenderer->DeactivateConstantModel();
}
void Player::MotionManager::ResetMotionFrame()
{
	model.animator.ResetTimer();
}
bool Player::MotionManager::WasCurrentMotionEnded() const
{
	return model.animator.WasEnded();
}
void Player::MotionManager::QuitShotMotion()
{
	shouldPoseShot = false;
	shotAnimator.ResetTimer();
}
void Player::MotionManager::UpdateShotMotion( Player &inst, float elapsedTime )
{
	if ( shotAnimator.WasEnded() )
	{
		shouldPoseShot = false;
	}

	if ( inst.shotManager.IsShotRequested( inst ) && inst.NowShotable( elapsedTime ) )
	{
		shouldPoseShot = true;
		shotAnimator.ResetTimer();
	}

	if ( !shouldPoseShot ) { return; }
	// else

	const auto &data = Parameter().Get();

	if ( inst.pMover && inst.pMover->NowGrabbingLadder( inst ) )
	{
		if ( inst.lookingSign < 0.0f )
		{
			ApplyPartMotion( inst, elapsedTime, MotionKind::LadderShotLeft, data.ladderLeftArm );
		}
		else
		{
			ApplyPartMotion( inst, elapsedTime, MotionKind::LadderShotRight, data.ladderRightArm );
		}
	}
	else
	{
		ApplyPartMotion( inst, elapsedTime, MotionKind::Shot, data.normalLeftArm );
	}
}
void Player::MotionManager::ApplyPartMotion( Player &inst, float elapsedTime, MotionKind useMotionKind, const ModelHelper::PartApply &partData )
{
	if ( !model.pResource ) { return; }
	// else

	const auto &data	= Parameter().Get();
	const auto &holder	= model.pResource->motionHolder;

	const size_t motionIndex = holder.FindMotionIndex( partData.motionName );
	if ( holder.GetMotionCount() <= motionIndex )
	{
		shouldPoseShot = false;
		return;
	}
	// else

	// Update "shotAnimator" and "shotPose" by "motionIndex"
	{
		const auto &motion	= holder.GetMotion( motionIndex );
		shotAnimator.SetRepeatRange( motion );

		const size_t motionKindIndex	= scast<size_t>( useMotionKind );
		const float  motionAcceleration = ( data.animePlaySpeeds.size() <= motionKindIndex ) ? 1.0f : data.animePlaySpeeds[motionKindIndex];
		shotAnimator.Update( fabsf( elapsedTime ) * motionAcceleration );

		shotPose.AssignSkeletal( shotAnimator.CalcCurrentPose( motion ) );
	}

	std::vector<Donya::Model::Animation::Node> normalPose = model.pose.GetCurrentPose();
	assert( shotPose.HasCompatibleWith( normalPose ) );

	std::vector<size_t> applyBoneIndices{};
	for ( const auto &rootName : partData.applyRootBoneNames )
	{
		ExplorePartBone( &applyBoneIndices, normalPose, rootName );
	}

	auto IsTargetRootName = [&partData]( const std::string &boneName )
	{
		const auto found = std::find
		(
			partData.applyRootBoneNames.begin(),
			partData.applyRootBoneNames.end(),
			boneName
		);
		return ( found != partData.applyRootBoneNames.end() ) ? true : false;
	};

	const auto &destPose = shotPose.GetCurrentPose();
	for ( auto &i : applyBoneIndices )
	{
		auto &node = normalPose[i];
		auto &dest = destPose[i];

		node.bone  = dest.bone;
		node.local = dest.local;

		// If the current node is the root bone of apply targets
		if ( IsTargetRootName( node.bone.name ) )
		{
			const auto &d = dest.global;
			const auto &n = node.global;

			Donya::Vector4x4 blend;
			// Apply destination's orientation(and scale)
			blend._11 = d._11;		blend._12 = d._12;		blend._13 = d._13;
			blend._21 = d._21;		blend._22 = d._22;		blend._23 = d._23;
			blend._31 = d._31;		blend._32 = d._32;		blend._33 = d._33;
			// Blend the translations
			blend._41 = Donya::Lerp( n._41, d._41, partData.rootTranslationBlendPercent.x );
			blend._42 = Donya::Lerp( n._42, d._42, partData.rootTranslationBlendPercent.y );
			blend._43 = Donya::Lerp( n._43, d._43, partData.rootTranslationBlendPercent.z );
			
			node.global = blend;
		}
		else
		{
			const Donya::Vector4x4 parentGlobal =
			( node.bone.parentIndex != -1 )
			? normalPose[node.bone.parentIndex].global
			: Donya::Vector4x4::Identity();

			node.global = dest.local * parentGlobal;
		}
	}

	model.pose.AssignSkeletal( normalPose );
}
void Player::MotionManager::ExplorePartBone( std::vector<size_t> *pIndices, const std::vector<Donya::Model::Animation::Node> &skeletal, const std::string &searchName )
{
	if ( !pIndices || searchName.empty() ) { return; }
	// else

	const size_t nodeCount = skeletal.size();

	// Register to apply target
	for ( size_t i = 0; i < nodeCount; ++i )
	{
		const auto &bone = skeletal[i].bone;
		if ( bone.name == searchName )
		{
			pIndices->emplace_back( i );
		}
	}
	
	// Explore the children of searchName
	for ( size_t i = 0; i < nodeCount; ++i )
	{
		const auto &bone = skeletal[i].bone;
		if ( bone.parentName == searchName )
		{
			ExplorePartBone( pIndices, skeletal, bone.name );
		}
	}

	// Make the pIndices will contain the searchName as first and the children as second, by explore recursively after the registration.
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
	case MotionKind::Idle:				return true;
	case MotionKind::Run:				return true;
	case MotionKind::Slide:				return true;
	case MotionKind::Jump_Rise:			return false;
	case MotionKind::Jump_Fall:			return false;
	case MotionKind::KnockBack:			return false;
	case MotionKind::GrabLadder:		return true;
	case MotionKind::Shot:				return false;
	case MotionKind::LadderShotLeft:	return false;
	case MotionKind::LadderShotRight:	return false;
	case MotionKind::Brace:				return true;
	case MotionKind::Appear:			return false;
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

	if ( inst.pMover && inst.pMover->NowAppearing( inst ) )
	{
		return MotionKind::Appear;
	}
	if ( inst.pMover && inst.pMover->NowGrabbingLadder( inst ) )
	{ return MotionKind::GrabLadder; }
	if ( inst.pMover && inst.pMover->NowKnockBacking( inst ) )
	{ return MotionKind::KnockBack; }
	if ( inst.pMover && inst.pMover->NowSliding( inst ) )
	{ return MotionKind::Slide; }
	// else

	const bool nowMoving = IsZero( inst.velocity.x ) ? false : true;
	const bool onGround  = inst.onGround;
	
	if ( !onGround )
	{
		return	( Donya::SignBit( inst.velocity.y ) == 1 )
				? MotionKind::Jump_Rise
				: MotionKind::Jump_Fall;
	}
	// else
	if ( nowMoving ) { return MotionKind::Run;	}
	// else
	const auto &input = inst.inputManager.Current();
	if ( input.moveVelocity.y < 0.0f ) { return MotionKind::Brace; }
	// else
	return MotionKind::Idle;
}

Player::ShotManager::~ShotManager()
{
	StopLoopSFXIfPlaying( /* forcely = */ true );
}
void Player::ShotManager::Init()
{
	chargeLevel			= ShotLevel::Normal;
	prevChargeSecond	= 0.0f;
	currChargeSecond	= 0.0f;
	nowTrigger			= false;
	fxComplete.Disable();
	fxLoop.Disable();

	StopLoopSFXIfPlaying();
}
void Player::ShotManager::Update( const Player &inst, float elapsedTime, const InputManager &input )
{
	prevChargeSecond = currChargeSecond;
	nowTrigger = false;

	const ShotLevel oldChargeLevel = chargeLevel;

	// If calculate the charge level after update the "currChargeSecond",
	// the level will be zero(ShotLevel::Normal) absolutely when fire timing, because that timing is the input was released.
	// So I must calculate it before the update. It will not be late for one frame by this.
	chargeLevel		= CalcChargeLevel();
	destColor		= CalcEmissiveColor();
	emissiveColor	= Donya::Lerp( emissiveColor, destColor, Parameter().Get().emissiveTransFactor );

	if ( oldChargeLevel != chargeLevel )
	{
		if ( chargeLevel == ShotLevel::Tough )
		{
			Donya::Sound::Play( Music::Charge_Start );
		}
		if ( chargeLevel == ShotLevel::Strong )
		{
			fxComplete = Effect::Handle::Generate( Effect::Kind::Charge_Complete, inst.GetPosition() );
			Donya::Sound::Play( Music::Charge_Complete );
		}
	}

	nowTrigger = NowTriggered( input );

	const bool chargeable = ( inst.pGun && inst.pGun->Chargeable() );
	if ( chargeable && input.UseShot() )
	{
		if ( nowTrigger )
		{
			// Reset it, but do not set zero(below addition will prevent zero).
			// If set zero, shot condition will regard as "triggered" in next loop
			// because the "prevChargeSecond" will be zero by this.
			currChargeSecond = 0.0f;
		}

		currChargeSecond += elapsedTime;
		PlayLoopSFXIfStopping();
	}
	else
	{
		currChargeSecond = 0.0f;
		StopLoopSFXIfPlaying();
	}

	fxComplete.SetPosition( inst.GetPosition() );
	fxLoop.SetPosition( inst.GetPosition() );
}
bool Player::ShotManager::IsShotRequested( const Player &inst ) const
{
	if ( nowTrigger ) { return true; }
	// else

	const bool prevIsZero	= IsZero( prevChargeSecond );
	const bool currIsZero	= IsZero( currChargeSecond );
	const bool triggered	= ( prevIsZero && !currIsZero );
	const bool released		= ( !prevIsZero && currIsZero );

	const bool allowReleaseFire = ( inst.pGun && inst.pGun->AllowFireByRelease( chargeLevel ) );

	return ( triggered )
		|| ( allowReleaseFire && released );
}
bool Player::ShotManager::NowTriggered( const InputManager &input ) const
{
	const auto &prev = input.Previous();
	const auto &curr = input.Current();
	for ( int i = 0; i < Input::variationCount; ++i )
	{
		if ( curr.useShots[i] && !prev.useShots[i] )
		{
			return true;
		}
	}

	return false;
}
Player::ShotLevel Player::ShotManager::CalcChargeLevel()
{
	const auto &chargeParams = Parameter().Get().chargeParams;

	constexpr std::array<ShotLevel, 2> compareHighLevels
	{
		ShotLevel::Strong,
		ShotLevel::Tough,
	};

	if ( chargeParams.size() < compareHighLevels.size() )
	{
		return ShotLevel::Normal;
	}
	// else

	for ( const auto &it : compareHighLevels )
	{
		if ( chargeParams[scast<int>( it )].chargeSecond <= currChargeSecond )
		{
			return it;
		}
	}

	return ShotLevel::Normal;
}
Donya::Vector3 Player::ShotManager::CalcEmissiveColor()
{
	constexpr size_t paramCount = scast<size_t>( Player::ShotLevel::LevelCount );
	constexpr Donya::Vector3 defaultColor = Donya::Vector3::Zero();
	
	const auto &chargeParams = Parameter().Get().chargeParams;

	if ( chargeParams.size() != paramCount ) { return defaultColor; }
	// else

	const PlayerParam::PerChargeLevel *pParam = nullptr;

	// Iterate by high-level
	const auto end = chargeParams.crend();
	for ( auto itr = chargeParams.crbegin(); itr != end; ++itr )
	{
		if ( currChargeSecond < itr->chargeSecond ) { continue; }
		// else

		pParam = &( *itr );
		break;
	}

	if ( !pParam ) { return destColor; }
	// else

	if ( pParam->emissiveColor == defaultColor ) { return defaultColor; }
	// else

	if ( IsZero( pParam->emissiveCycleSecond ) )
	{
		_ASSERT_EXPR( 0, L"Error: Division by zero!" );
		return defaultColor;
	}
	// else

	const float angle		= ToRadian( currChargeSecond * 360.0f );
	const float cycleSpeed	= 1.0f / pParam->emissiveCycleSecond;
	const float sin_01		= ( sinf( angle * cycleSpeed ) + 1.0f ) * 0.5f;
	const float sinRange	= 1.0f - pParam->emissiveMinBias;
	const float easeFactor	= ( sin_01 * sinRange ) + pParam->emissiveMinBias;
	const float colorFactor = Donya::Easing::Ease( pParam->emissiveEaseKind, pParam->emissiveEaseType, easeFactor );

	return pParam->emissiveColor.Product( colorFactor );
}
void Player::ShotManager::PlayLoopSFXIfStopping()
{
	if ( playingChargeSE					) { return; }
	if ( chargeLevel == ShotLevel::Normal	) { return; }
	// else

	// Actual position will set at the end of Update()
	constexpr Donya::Vector3 generatePos = Donya::Vector3::Zero();

	fxLoop = Effect::Handle::Generate( Effect::Kind::Charge_Loop, generatePos );
	playingChargeSE = true;
	Donya::Sound::Play( Music::Charge_Loop );
}
void Player::ShotManager::StopLoopSFXIfPlaying( bool forcely )
{
	if ( !playingChargeSE && !forcely ) { return; }
	// else

	fxLoop.Stop();
	fxLoop.Disable();
	playingChargeSE = false;
	Donya::Sound::Stop( Music::Charge_Loop, /* isEnableForAll = */ true );
}

Player::Flusher::~Flusher()
{
	fxHurt.Stop();
	fxHurt.Disable();
}
void Player::Flusher::Start( float flushingSeconds )
{
	workingSeconds	= flushingSeconds;
	timer			= 0.0f;
	fxHurt			= Effect::Handle::Generate( Effect::Kind::HurtDamage, {} );
}
void Player::Flusher::Update( const Player &inst, float elapsedTime )
{
	timer += elapsedTime;
	fxHurt.SetPosition( inst.GetPosition() );
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
void Player::MoverBase::MotionUpdate( Player &inst, float elapsedTime, bool stopAnimation )
{
	inst.motionManager.Update( inst, elapsedTime, stopAnimation );
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
	else if ( 0.001f < fabsf( movement.y ) ) // Should not change if the movement is none
	{
		inst.onGround = false;
	}

	// We must apply world position to hurt box also
	inst.hurtBox.pos = inst.body.pos;
}
void Player::MoverBase::AssignBodyParameter( Player &inst )
{
	inst.body		= inst.GetNormalBody( /* ofHurtBox = */ false );
	inst.hurtBox	= inst.GetNormalBody( /* ofHurtBox = */ true  );
}

void Player::Normal::Update( Player &inst, float elapsedTime, const Map &terrain )
{
	const auto &input = inst.inputManager.Current();

	inst.MoveHorizontal( elapsedTime );

	// Deformity of MoveVertical()
	{
		// Jump condition and resolve vs slide condition
		if ( inst.WillUseJump() && !IsZero( elapsedTime ) ) // Make to can not act if game time is pausing
		{
			const int jumpInputIndex = inst.inputManager.UseJumpIndex();
			assert( 0 <= jumpInputIndex && jumpInputIndex < Input::variationCount );

			if ( Donya::SignBit( input.moveVelocity.y ) < 0 )
			{
				gotoSlide = true;

				// Certainly doing Fall() if do not jump
				inst.Fall( elapsedTime );
				// But We must set the pressing flag because We wanna prevent to jump.
				inst.inputManager.WasReleasedJumpInput()[jumpInputIndex] = false;
			}
			else
			{
				inst.Jump( jumpInputIndex );
			}
		}
		else
		{
			if ( inst.inputManager.UseDash() && inst.onGround )
			{
				gotoSlide = true;
			}

			inst.Fall( elapsedTime );
		}
	}

	inst.ShotIfRequested( elapsedTime );

	// Try to grabbing ladder if the game time is not pausing
	if ( !gotoSlide && !IsZero( elapsedTime ) )
	{
		auto IsLadder	= [&]( const std::shared_ptr<const Tile> &targetTile )
		{
			return ( targetTile && targetTile->GetID() == StageFormat::Ladder ) ? true : false;
		};
		auto GotoLadder	= [&]( const std::shared_ptr<const Tile> &targetTile )
		{
			inst.pTargetLadder	= targetTile;
			gotoLadder			= true;
		};
		auto GotoLadderIfSpecifyTileIsLadder	= [&]( const Donya::Vector3 &wsVerifyPosition )
		{
			const auto targetTile = terrain.GetPlaceTileOrNullptr( wsVerifyPosition );
			if ( IsLadder( targetTile ) )
			{
				GotoLadder( targetTile );
			}
		};
		auto GotoLadderIfSpecifyTilesAreLadder	= [&]( const Donya::Collision::Box3F &wsVerifyArea )
		{
			const auto targetTiles = terrain.GetPlaceTiles( wsVerifyArea );
			for ( const auto &tile : targetTiles )
			{
				if ( IsLadder( tile ) )
				{
					GotoLadder( tile );
					break;
				}
			}
		};

		const int verticalInputSign = Donya::SignBit( input.moveVelocity.y );
		if ( verticalInputSign == 1 )
		{
			const auto grabArea = inst.GetLadderGrabArea();
			GotoLadderIfSpecifyTilesAreLadder( grabArea );
			if ( !gotoLadder ) // If still does not grabbing
			{
				const Donya::Vector3 headOffset{ 0.0f, inst.body.size.y, 0.0f };
				GotoLadderIfSpecifyTileIsLadder( inst.GetPosition() + headOffset );
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

	braceOneself = ( input.moveVelocity.y < 0.0f && inst.onGround && IsZero( inst.velocity.x ) );
	MotionUpdate( inst, elapsedTime );
}
void Player::Normal::Move( Player &inst, float elapsedTime, const Map &terrain, float roomLeftBorder, float roomRightBorder )
{
	MoveOnlyHorizontal( inst, elapsedTime, terrain, roomLeftBorder, roomRightBorder );
	MoveOnlyVertical  ( inst, elapsedTime, terrain );
}
bool Player::Normal::NowBracing( const Player &inst ) const
{
	return braceOneself;
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
	inst.velocity.y = 0.0f;
	inst.velocity.z = 0.0f;
	inst.UpdateOrientation( /* lookingRight = */ ( slideSign < 0.0f ) ? false : true );

	Effect::Handle handle = Effect::Handle::Generate( Effect::Kind::Player_Slide_Begin, inst.GetPosition() );
	handle.SetRotation( 0.0f, ToRadian( 90.0f ) * slideSign, 0.0f );
	Effect::Admin::Get().AddCopy( handle ); // Leave the effect instance's management to admin
	
	Donya::Sound::Play( Music::Player_Dash );
}
void Player::Slide::Uninit( Player &inst )
{
	MoverBase::Uninit( inst );

	inst.velocity.x = 0.0f;
}
void Player::Slide::Update( Player &inst, float elapsedTime, const Map &terrain )
{
	timer += elapsedTime;

	const auto &input = inst.inputManager.Current();

	const int  horizontalInputSign	= Donya::SignBit( input.moveVelocity.x );
	const bool moveToBackward		=  ( horizontalInputSign != 0 ) && ( horizontalInputSign != Donya::SignBit( slideSign ) );
	const bool slideIsEnd			=  ( Parameter().Get().slideMoveSeconds <= timer )
									|| ( moveToBackward )
									|| ( !inst.onGround )
									|| ( inst.WillUseJump() )
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

	if ( !IsZero( inst.velocity.x ) )
	{
		inst.lookingSign = Donya::SignBitF( inst.velocity.x );
	}

	// If the jump was triggered in here, the "slideIsEnd" is also true.
	inst.MoveVertical( elapsedTime );

	inst.ShotIfRequested( elapsedTime );

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
	// TODO: Prevent to fall into a tight hole
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
	inst.lookingSign= inst.lookingSign * -1.0f; // Indicates implicitly the direction that the player should advance
	if ( IsZero( inst.lookingSign ) ) { inst.lookingSign = 1.0f; } // Fail safe

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
	inst.UpdateOrientation( /* lookingRight = */ ( inst.lookingSign < 0.0f ) ? false : true );

	// Adjust the position onto a ladder
	if ( releaseWay == ReleaseWay::Climb )
	{
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

		inst.onGround = true; // Do not play the landing SE
		inst.Landing();
	}

	// Cancel the grab motion
	MotionUpdate( inst, 0.0001f, /* stopAnimation = */ true );
}
void Player::GrabLadder::Update( Player &inst, float elapsedTime, const Map &terrain )
{
	inst.velocity = 0.0f;

	if ( NowUnderShotLag() )
	{
		shotLagSecond -= elapsedTime;
		std::max( 0.0f, shotLagSecond );
	}

	const auto &data  = Parameter().Get();
	const auto &input = inst.inputManager.Current();

	if ( !NowUnderShotLag() )
	{
		inst.velocity.y = data.ladderMoveSpeed * input.moveVelocity.y;
	}

	ShotProcess( inst, elapsedTime );

	const bool  pause = IsZero( inst.velocity.y );
	const float sign  = Donya::SignBitF( inst.velocity.y );
	MotionUpdate
	(
		inst,
		( pause )
		? elapsedTime // For update the shot motion only
		: elapsedTime * sign,
		pause
	);

	// The "releaseWay" must be update after MotionUpdate().
	// Because if it is not None(i.e. ShouldChangeMover() is true),
	// the MotionUpdate() assigns some not ladder motion, then may update some motion as reverse.
	// Reverse playing of motion is undesirable, so I must prevent that.
	releaseWay = JudgeWhetherToRelease( inst, elapsedTime, terrain );
}
void Player::GrabLadder::Move( Player &inst, float elapsedTime, const Map &terrain, float roomLeftBorder, float roomRightBorder )
{
	MoveOnlyHorizontal( inst, elapsedTime, terrain, roomLeftBorder, roomRightBorder );
	MoveOnlyVertical  ( inst, elapsedTime, terrain );

	// We must apply world position to other boxes also
	inst.hurtBox.pos	= inst.body.pos;
	grabArea.pos		= inst.body.pos;
}
bool Player::GrabLadder::NowGrabbingLadder( const Player &inst ) const
{
	// Prioritize an other motion if now releasing
	if ( ShouldChangeMover( inst ) ) { return false; }
	// else
	return true;
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
void Player::GrabLadder::ShotProcess( Player &inst, float elapsedTime )
{
	const bool wantShot = ( inst.shotManager.IsShotRequested( inst ) && inst.NowShotable( elapsedTime ) );
	if ( !wantShot ) { return; }
	// else

	const auto &data  = Parameter().Get();
	const auto &input = inst.inputManager.Current();
	shotLagSecond = data.ladderShotLagSecond;

	const int sideInputSign = Donya::SignBit( input.moveVelocity.x );
	if ( sideInputSign != 0 )
	{
		inst.lookingSign = scast<float>( sideInputSign );
	}

	inst.ShotIfRequested( elapsedTime );
}
Player::GrabLadder::ReleaseWay Player::GrabLadder::JudgeWhetherToRelease( Player &inst, float elapsedTime, const Map &terrain ) const
{
	if ( releaseWay != ReleaseWay::None ) { return releaseWay; }
	// else

	if ( inst.onGround )
	{
		return ReleaseWay::Dismount;
	}
	// else

	const int jumpInputIndex = inst.inputManager.UseJumpIndex();
	const auto &currentInput = inst.inputManager.Current();
	auto &wasReleasedJumpInputs = inst.inputManager.WasReleasedJumpInput();
	if ( !inst.inputManager.UseJump() )
	{
		wasReleasedJumpInputs.fill( true );
	}
	else // Condition of releasing by jump input
	if ( wasReleasedJumpInputs[jumpInputIndex] && inst.velocity.y <= 0.0f && !IsZero( elapsedTime ) )
	{
		// Prevent the grab-release loop by press keeping the jump and the up input
		wasReleasedJumpInputs[jumpInputIndex] = false;
		// Disallow gravity resistance
		inst.inputManager.KeepSecondJumpInput()[jumpInputIndex] = Parameter().Get().resistableSeconds;
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

	const auto &data = Parameter().Get();

	const bool knockedFromRight = ( inst.pReceivedDamage && inst.pReceivedDamage->knockedFromRight );
	inst.UpdateOrientation( knockedFromRight );

	if ( !inst.prevSlidingStatus )
	{
		const float impulseSign = ( knockedFromRight ) ? -1.0f : 1.0f;
		inst.lookingSign = -impulseSign;

		if ( !inst.prevBracingStatus )
		{
			inst.velocity.x  = data.knockBackSpeed * impulseSign;
		}
	}

	inst.motionManager.QuitShotMotion();
	
	timer		= 0.0f;
	motionSpeed	= ( inst.prevBracingStatus ) ? data.braceStandFactor : 1.0f;
}
void Player::KnockBack::Uninit( Player &inst )
{
	MoverBase::Uninit( inst );

	inst.velocity.x = 0.0f;
}
void Player::KnockBack::Update( Player &inst, float elapsedTime, const Map &terrain )
{
	timer += elapsedTime * motionSpeed;

	inst.Fall( elapsedTime );

	MotionUpdate( inst, elapsedTime * motionSpeed );
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
void Player::Miss::Update( Player &inst, float elapsedTime, const Map &terrain )
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

void Player::Appear::Init( Player &inst )
{
	MoverBase::Init( inst );

	inst.velocity = Donya::Vector3::Zero();

	timer	= 0.0f;
	visible	= false;

	Effect::Admin::Get().GenerateInstance( Effect::Kind::Player_Appear, inst.GetPosition() );
	Donya::Sound::Play( Music::Player_Appear );
}
void Player::Appear::Update( Player &inst, float elapsedTime, const Map &terrain )
{
	timer += elapsedTime;

	if ( !visible )
	{
		const auto &data = Parameter().Get();
		if ( data.appearDelaySecond <= timer )
		{
			visible = true;
			inst.motionManager.ResetMotionFrame();
		}
	}

	MotionUpdate( inst, elapsedTime );
}
void Player::Appear::Move( Player &inst, float elapsedTime, const Map &terrain, float roomLeftBorder, float roomRightBorder )
{
	// No op
}
bool Player::Appear::Drawable( const Player &inst ) const
{
	return visible;
}
bool Player::Appear::ShouldChangeMover( const Player &inst ) const
{
	return inst.motionManager.WasCurrentMotionEnded();
}
std::function<void()> Player::Appear::GetChangeStateMethod( Player &inst ) const
{
	return [&inst]() { inst.AssignMover<Normal>(); };
}

void Player::Leave::Init( Player &inst )
{
	MoverBase::Init( inst );

	inst.velocity		= Donya::Vector3::Zero();
	inst.hurtBox.exist	= false;

	timer	= 0.0f;
	visible	= true;

	Effect::Admin::Get().GenerateInstance( Effect::Kind::Player_Leave, inst.GetPosition() );
	Donya::Sound::Play( Music::Player_Leave );
}
void Player::Leave::Update( Player &inst, float elapsedTime, const Map &terrain )
{
	timer += elapsedTime;

	if ( visible )
	{
		const auto &data = Parameter().Get();
		if ( data.leaveDelaySecond <= timer )
		{
			visible = false;
		}
	}

	MotionUpdate( inst, elapsedTime );
}
void Player::Leave::Move( Player &inst, float elapsedTime, const Map &terrain, float roomLeftBorder, float roomRightBorder )
{
	// No op
}
bool Player::Leave::Drawable( const Player &inst ) const
{
	return visible;
}
bool Player::Leave::ShouldChangeMover( const Player &inst ) const
{
	return false;
}
std::function<void()> Player::Leave::GetChangeStateMethod( Player &inst ) const
{
	return []() {}; // No op
}


// region Mover
#pragma endregion

#pragma region Gun

void Player::GunBase::Init( Player &inst )
{
	kind = GetKind();
	inst.pBullet.reset();
}
void Player::GunBase::Uninit( Player &inst )
{
	// No op
}
void Player::GunBase::Update( Player &inst, float elapsedTime )
{
	// No op
}

bool Player::BusterGun::Chargeable() const
{
	return true;
}
bool Player::BusterGun::AllowFireByRelease( ShotLevel chargeLevel ) const
{
	// I wanna fire a charge shot by release,
	// but I don't wanna fire the normal shot by release.
	const bool nowHighLevel = ( chargeLevel != ShotLevel::Normal && chargeLevel != ShotLevel::LevelCount );
	return nowHighLevel;
}
void Player::BusterGun::Fire( Player &inst, const InputManager &input )
{	
	const auto &data = Parameter().Get();

	const Donya::Quaternion lookingRotation = Donya::Quaternion::Make
	(
		Donya::Vector3::Up(), ToRadian( 90.0f ) * inst.lookingSign
	);

	Bullet::FireDesc desc = data.fireParam;
	desc.direction	=  ( inst.lookingSign < 0.0f ) ? -Donya::Vector3::Right() : Donya::Vector3::Right();
	desc.position	=  lookingRotation.RotateVector( desc.position ); // Rotate the local space offset
	desc.position	+= inst.GetPosition(); // Convert to world space
	desc.owner		=  inst.hurtBox.id;

	const ShotLevel level = inst.shotManager.ChargeLevel();
	if ( level != ShotLevel::Normal && level != ShotLevel::LevelCount )
	{
		using Dmg = Definition::Damage;
		desc.pAdditionalDamage			= std::make_shared<Dmg>();
		desc.pAdditionalDamage->amount	= scast<int>( level );
		desc.pAdditionalDamage->type	= Dmg::Type::Pierce;
	}
		
	Bullet::Admin::Get().RequestFire( desc );
	Donya::Sound::Play( Music::Bullet_ShotBuster );
}

void Player::ShieldGun::Init( Player &inst )
{
	GunBase::Init( inst );

	takeShield = false;
}
void Player::ShieldGun::Uninit( Player &inst )
{
	if ( takeShield && inst.pBullet )
	{
		std::shared_ptr<Bullet::Base> pInstance = Bullet::Admin::Get().FindInstanceOrNullptr( inst.pBullet );
		if ( pInstance )
		{
			pInstance->SetLifeTime( 0.0f );
		}
	}

	ReleaseShieldHandle( inst );
}
void Player::ShieldGun::Update( Player &inst, float elapsedTime )
{
	if ( !takeShield ) { return; }
	// else

	if ( !inst.pBullet ) { return; }
	// else

	std::shared_ptr<Bullet::Base> pInstance = Bullet::Admin::Get().FindInstanceOrNullptr( inst.pBullet );
	if ( !pInstance )
	{
		// The handle has been invalided
		ReleaseShieldHandle( inst );
		return;
	}
	// else

	pInstance->SetWorldPosition( CalcShieldPosition( inst ) );
}
bool Player::ShieldGun::Chargeable() const
{
	return false;
}
bool Player::ShieldGun::AllowFireByRelease( ShotLevel nowChargeLevel ) const
{
	return false;
}
void Player::ShieldGun::Fire( Player &inst, const InputManager &input )
{
	if ( !inst.pBullet )
	{
		ExpandShield( inst, input );
		return;
	}
	// else

	ThrowShield( inst, input );
}
void Player::ShieldGun::ReleaseShieldHandle( Player &inst )
{
	takeShield = false;
	inst.pBullet.reset(); // Also release the handle
}
Donya::Vector3 Player::ShieldGun::CalcThrowDirection( const Player &inst, const InputManager &input ) const
{
	const Donya::Vector2 stick = input.Current().moveVelocity.Unit();
	if ( stick.IsZero() ) { return inst.orientation.LocalFront(); }
	// else

	// Convert to 4 directions(right, up, left, down)

	constexpr std::array<Donya::Vector3, 4> directions
	{
		Donya::Vector3::Right(),
		Donya::Vector3::Up(),
		-Donya::Vector3::Right(),
		-Donya::Vector3::Up()
	};
	constexpr int   directionCount = scast<int>( directions.size() );
	constexpr float degreeInterval = 360.0f / directionCount;
	constexpr float halfInterval   = degreeInterval * 0.5f;

	// 0-deg is right, CCW
	const float degree = fmodf( stick.Degree() + 360.0f, 360.0f ); // -180.0f ~ +180.0f -> 0 ~ 360.0f

	int intervalCount  = scast<int>( degree / degreeInterval );
	intervalCount %= directions.size(); // Wrap around the over 360.0f degree

	// It will be 0.0f or 90.0f or 180.0f or 270.0f
	float correctedDegree = 90.0f * scast<float>( intervalCount );
	// Make the border as diagonally,
	// So it will be -45.0f or 45.0f or 135.0f or 225.0f
	correctedDegree -= halfInterval;
	if ( correctedDegree < 0.0f ) { correctedDegree += 360.0f; }

	for ( int i = 0; i < directionCount; ++i )
	{
		const float border = ( scast<float>( i ) * degreeInterval ) + halfInterval;
		if ( correctedDegree < border )
		{
			return directions[i];
		}
	}

	// Arounded
	return directions[0];
}
Donya::Vector3 Player::ShieldGun::CalcShieldPosition( const Player &inst ) const
{
	Donya::Vector3 tmp;
	tmp =  inst.orientation.RotateVector( Parameter().Get().shieldPosOffset );
	tmp += inst.body.WorldPosition(); // Local space to World space
	return tmp;
}
void Player::ShieldGun::ExpandShield( Player &inst, const InputManager &input )
{
	Bullet::FireDesc desc{};
	desc.kind			= Bullet::Kind::SkullShield;
	desc.initialSpeed	= 0.0f;
	desc.direction		= Donya::Vector3::Zero();
	desc.position		= CalcShieldPosition( inst );
	desc.owner			= inst.hurtBox.id;
	inst.pBullet = std::make_unique<Bullet::SkullShield>();
	inst.pBullet->Init( desc );
	inst.pBullet->DisallowRemovingByOutOfScreen();
	Bullet::Admin::Get().AddCopy( inst.pBullet );

	takeShield = true;

	Donya::Sound::Play( Music::Bullet_ShotShield_Expand );
}
void Player::ShieldGun::ThrowShield( Player &inst, const InputManager &input )
{
	if ( !takeShield ) { return; }
	// else

	std::shared_ptr<Bullet::Base> pInstance = Bullet::Admin::Get().FindInstanceOrNullptr( inst.pBullet );
	if ( !pInstance ) { return; } // The handle has been invalided
	// else

	const Donya::Vector3 direction = CalcThrowDirection( inst, input );
	pInstance->SetVelocity( direction * Parameter().Get().shieldThrowSpeed );
	ReleaseShieldHandle( inst );

	Donya::Sound::Play( Music::Bullet_ShotShield_Throw );
}

// Gun
#pragma endregion

void Player::Init( const PlayerInitializer &initializer, const Map &terrain, bool withAppearPerformance )
{
	const bool shouldLookingToRight = initializer.ShouldLookingRight();
	UpdateOrientation( shouldLookingToRight );
	lookingSign = ( shouldLookingToRight ) ? 1.0f : -1.0f;

	const auto &data	= Parameter().Get();
	body				= data.hitBox;
	hurtBox				= data.hurtBox;
	hurtBox.id			= Donya::Collision::GetUniqueID();
	hurtBox.ownerID		= Donya::Collision::invalidID;
	hurtBox.ignoreList.clear();
	body.pos			= initializer.GetWorldInitialPos(); // The "body.pos" will be used as foot position.
	hurtBox.pos			= body.pos;
	velocity			= 0.0f;
	inputManager.Init();
	motionManager.Init();
	shotManager.Init();
	currentHP			= data.maxHP;
	prevSlidingStatus	= false;
	onGround			= false;

	// Validate should I take a ground state
	{
		constexpr Donya::Vector3 errorOffset
		{
			0.0f,
			0.001f,
			0.0f
		};
		const Donya::Vector3 foot
		{
			body.pos.x,
			body.pos.y - body.size.y,
			body.pos.z
		};
		const Donya::Vector3 validations[]
		{
			foot,
			foot - errorOffset // Look below a little
		};

		for ( const auto &it : validations )
		{
			const auto pTile = terrain.GetPlaceTileOrNullptr( foot );
			if ( pTile && StageFormat::IsRidableTileID( pTile->GetID() ) )
			{
				const Donya::Vector3 tileTop = pTile->GetHitBox().Max();
				body.pos.y		= tileTop.y + errorOffset.y; // Place the foot on the tile
				hurtBox.pos.y	= body.pos.y;
				onGround		= true;
				break;
			}
		}
	}

	( withAppearPerformance )
	? AssignMover<Appear>()
	: AssignMover<Normal>();

	AssignGun<BusterGun>();
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

		body.offset		= orientation.RotateVector( body.offset		);
		hurtBox.offset	= orientation.RotateVector( hurtBox.offset	);
	}
#endif // USE_IMGUI

	inputManager.Update( elapsedTime, input );

	hurtBox.UpdateIgnoreList( elapsedTime );

	shotManager.Update( *this, elapsedTime, inputManager );

	if ( !pMover )
	{
		_ASSERT_EXPR( 0, L"Error: Player's mover is not assigned!" );
		return;
	}
	// else

	invincibleTimer.Update( *this, elapsedTime );
	ApplyReceivedDamageIfHas( elapsedTime, terrain );

	hurtBox.exist = ( invincibleTimer.NowWorking() ) ? false : true;

	pMover->Update( *this, elapsedTime, terrain );
	if ( pMover->ShouldChangeMover( *this ) )
	{
		auto ChangeMethod = pMover->GetChangeStateMethod( *this );
		ChangeMethod();
	}

	ShiftGunIfNeeded();
	pGun->Update( *this, elapsedTime );

	prevSlidingStatus = pMover->NowSliding( *this );
	prevBracingStatus = pMover->NowBracing( *this );
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
	if ( !pMover->Drawable( *this ) ) { return; }
	// else

	const Donya::Vector3   &drawPos = body.pos; // I wanna adjust the hit-box to fit for drawing model, so I don't apply the offset for the position of drawing model.
	const Donya::Vector4x4 W = MakeWorldMatrix( 1.0f, /* enableRotation = */ true, drawPos );

	constexpr Donya::Vector3 basicColor{ 1.0f, 1.0f, 1.0f };
	const     Donya::Vector3 emissiveColor = shotManager.EmissiveColor();
	const float alpha = ( invincibleTimer.Drawable() ) ? 1.0f : 0.0f;
	motionManager.Draw( pRenderer, W, basicColor + emissiveColor, alpha );
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
	const auto  body		= GetHitBox();
	const auto  hurt		= GetHurtBox();
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

	if ( pBullet )
	{
		pBullet->DrawHitBox( pRenderer, matVP );
	}
#endif // DEBUG_MODE
}
void Player::RecoverHP( int recovery )
{
	currentHP += recovery;
	currentHP =  std::min( Parameter().Get().maxHP, currentHP );
}
bool Player::NowMiss() const
{
	return ( !pMover ) ? true : ( pMover->NowMiss( *this ) ) ? true : false;
}
bool Player::NowGrabbingLadder() const
{
	return ( pMover && pMover->NowGrabbingLadder( *this ) ) ? true : false;
}
int  Player::GetCurrentHP() const
{
	return currentHP;
}
Donya::Collision::Box3F	Player::GetHurtBox() const
{
	Donya::Collision::Box3F tmp = hurtBox;
	tmp.offset = orientation.RotateVector( tmp.offset );
	return tmp;
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
	Effect::Admin::Get().GenerateInstance( Effect::Kind::Death, GetPosition() );

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
void Player::PerformLeaving()
{
	AssignMover<Leave>();
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
void Player::AssignGunByKind( GunKind kind )
{
	_ASSERT_EXPR( 2 == scast<int>( GunKind::GunCount ), L"Please add the new kind's case to switch statement!" );

	switch ( kind )
	{
	case GunKind::Buster:
		AssignGun<BusterGun>();
		return;
	case GunKind::Shield:
		AssignGun<ShieldGun>();
		return;
	default: _ASSERT_EXPR( 0, L"Unexpected Kind of Gun!" );
		return;
	}
}
void Player::AssignCurrentBodyInfo( Donya::Collision::Box3F *p, bool useHurtBox ) const
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
	AssignCurrentBodyInfo( &tmp, ofHurtBox );
	tmp.offset = orientation.RotateVector( tmp.offset );
	return tmp;
}
Donya::Collision::Box3F Player::GetSlidingBody( bool ofHurtBox ) const
{
	const auto &data = Parameter().Get();

	Donya::Collision::Box3F tmp = ( ofHurtBox ) ? data.slideHurtBox : data.slideHitBox;
	AssignCurrentBodyInfo( &tmp, ofHurtBox );
	tmp.offset = orientation.RotateVector( tmp.offset );
	return tmp;
}
Donya::Collision::Box3F Player::GetLadderGrabArea() const
{
	const auto &data = Parameter().Get();

	Donya::Collision::Box3F tmp = data.ladderGrabArea;
	AssignCurrentBodyInfo( &tmp, /* useHurtBoxInfo = */ false );
	tmp.offset = orientation.RotateVector( tmp.offset );
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
void Player::MoveHorizontal( float elapsedTime )
{
	const auto &data = Parameter().Get();

	const auto &input = inputManager.Current();
	const float  movement = data.moveSpeed * input.moveVelocity.x;
	velocity.x = movement;

	if ( !IsZero( velocity.x * elapsedTime ) ) // The "elapsedTime" prevents to rotate when pauses a game time
	{
		lookingSign = Donya::SignBitF( velocity.x );

		UpdateOrientation( /* lookingRight = */ ( lookingSign < 0.0f ) ? false : true );
	}
}
void Player::MoveVertical  ( float elapsedTime )
{
	if ( WillUseJump() && !IsZero( elapsedTime ) ) // Make to can not act if game time is pausing
	{
		Jump( inputManager.UseJumpIndex() );
	}
	else
	{
		Fall( elapsedTime );
	}
}
bool Player::NowShotable( float elapsedTime ) const
{
	if ( IsZero( elapsedTime )						) { return false; } // If game time is pausing
	if ( pMover && pMover->NowAppearing( *this )	) { return false; }
	// else

	const bool movable		= ( pMover && !pMover->NowKnockBacking( *this ) );
	const bool generatable	= ( Bullet::Buster::GetLivingCount() < Parameter().Get().maxBusterCount );
	return ( generatable && movable ) ? true : false;
}
void Player::ShotIfRequested( float elapsedTime )
{
	if ( !pGun									) { return; }
	if ( !shotManager.IsShotRequested( *this )	) { return; }
	if ( !NowShotable( elapsedTime )			) { return; }
	// else

	pGun->Fire( *this, inputManager );

	//const auto &data = Parameter().Get();

	//const Donya::Quaternion lookingRotation = Donya::Quaternion::Make
	//(
	//	Donya::Vector3::Up(), ToRadian( 90.0f ) * lookingSign
	//);

	//Bullet::FireDesc desc = data.fireParam;
	//desc.direction	=  ( lookingSign < 0.0f ) ? -Donya::Vector3::Right() : Donya::Vector3::Right();
	//desc.position	=  lookingRotation.RotateVector( desc.position ); // Rotate the local space offset
	//desc.position	+= GetPosition(); // Convert to world space
	//desc.owner		=  hurtBox.id;

	//const ShotLevel level = shotManager.ChargeLevel();
	//if ( level != ShotLevel::Normal && level != ShotLevel::LevelCount )
	//{
	//	using Dmg = Definition::Damage;
	//	desc.pAdditionalDamage			= std::make_shared<Dmg>();
	//	desc.pAdditionalDamage->amount	= scast<int>( level );
	//	desc.pAdditionalDamage->type	= Dmg::Type::Pierce;
	//}
	//	
	//Bullet::Admin::Get().RequestFire( desc );
	//Donya::Sound::Play( Music::Player_Shot );
}
void Player::UpdateOrientation( bool lookingRight )
{
	const float rotateSign = ( lookingRight ) ? 1.0f : -1.0f;
	orientation = Donya::Quaternion::Make
	(
		Donya::Vector3::Up(), ToRadian( 90.0f ) * rotateSign
	);
}
void Player::Jump( int inputIndex )
{
	const auto &data = Parameter().Get();

	onGround	= false;
	velocity.y	= data.jumpStrength;
	inputManager.KeepSecondJumpInput()[inputIndex]	= 0.0f;
	inputManager.WasReleasedJumpInput()[inputIndex]	= false;
	Donya::Sound::Play( Music::Player_Jump );
}
bool Player::Jumpable( int inputIndex ) const
{
	return ( onGround && inputManager.Jumpable( inputIndex ) ) ? true : false;
}
bool Player::WillUseJump() const
{
	const int jumpInputIndex = inputManager.UseJumpIndex();
	if ( 0 <= jumpInputIndex && Jumpable( jumpInputIndex ) )
	{
		return true;
	}
	// else
	return false;
}
void Player::Fall( float elapsedTime )
{
	const auto &data = Parameter().Get();

	bool resistGravity = false;
	const bool canResist = !( pMover && pMover->NowKnockBacking( *this ) );
	const int jumpInputIndex = inputManager.UseJumpIndex();
	if ( 0 <= jumpInputIndex && canResist )
	{
		if ( !inputManager.WasReleasedJumpInput()[jumpInputIndex] )
		{
			if ( inputManager.KeepSecondJumpInput()[jumpInputIndex] < data.resistableSeconds )
			{
				resistGravity = true;
			}
		}
	}
	else
	{
		inputManager.WasReleasedJumpInput().fill( true );
	}

#if 0 // Control the Y velocity when the jump input was released. do not works correctly
	const bool nowReleaseMoment = ( jumpInputIndex < 0 && 0 < inputManager.UseJumpIndex( /* getCurrent = */ false ) ); // Current is off(index < 0), Previous is on(0 < index)
	if ( nowReleaseMoment )
	{
		velocity.y = 0.0f;
	}
#endif // 0

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
void Player::ShiftGunIfNeeded()
{
	if ( !pGun ) { return; }
	// else

	const int shiftSign = inputManager.ShiftGun();
	if ( !shiftSign ) { return; }
	// else

	constexpr int kindCount = scast<int>( GunKind::GunCount );

	int index = scast<int>( pGun->GetKind() );
	index += shiftSign;
	if ( index <  0			) { index = kindCount - 1; }
	if ( index >= kindCount	) { index = 0; }

	AssignGunByKind( scast<GunKind>( index ) );
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

	int remaining = Remaining::Get();
	ImGui::SliderInt	( u8"残機数",						&remaining, 0, Parameter().Get().maxRemainCount );
	Remaining::Set( remaining );

	ImGui::DragInt		( u8"現在の体力",					&currentHP );
	ImGui::Text			( u8"現在のステート：%s",				pMover->GetMoverName().c_str() );
	ImGui::Text			( u8"現在のモーション：%s",			KIND_NAMES[scast<int>( motionManager.CurrentKind() )] );
	ImGui::Text			( u8"現在の銃口：%d",					pGun->GetKind() );

	ImGui::Text			( u8"%d: チャージレベル",				scast<int>( shotManager.ChargeLevel() ) );
	ImGui::Text			( u8"%04.2f: ショット長押し秒数",		shotManager.ChargeSecond() );
	ImGui::Checkbox		( u8"地上にいるか",					&onGround );

	bool tmp{};
	tmp = pMover->NowKnockBacking( *this );
	ImGui::Checkbox		( u8"のけぞり中か",					&tmp );
	tmp = invincibleTimer.NowWorking();
	ImGui::Checkbox		( u8"無敵中か",						&tmp );

	inputManager.ShowImGuiNode( u8"入力状態" );

	if ( ImGui::Button( u8"登場演出再生" ) )
	{
		AssignMover<Appear>();
	}
	if ( ImGui::Button( u8"退場させる" ) )
	{
		AssignMover<Leave>();
	}

	ImGui::DragFloat3	( u8"ワールド座標",					&body.pos.x,	0.01f );
	ImGui::DragFloat3	( u8"速度",							&velocity.x,	0.01f );

	Donya::Vector3 front = orientation.LocalFront();
	ImGui::SliderFloat3	( u8"前方向",						&front.x,		-1.0f, 1.0f );
	orientation = Donya::Quaternion::LookAt( Donya::Vector3::Front(), front.Unit(), Donya::Quaternion::Freeze::Up );
	ImGui::SliderFloat	( u8"向いているＸ方向",				&lookingSign,	-1.0f, 1.0f );

	ImGui::TreePop();
}
#endif // USE_IMGUI
