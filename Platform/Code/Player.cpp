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
#include "SaveData.h"
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
		"ChargedShot",
		"LadderShotLeft",
		"LadderChargedShotLeft",
		"LadderShotRight",
		"LadderChargedShotRight",
		"Brace",
		"Appear",
		"Winning",
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

	constexpr Donya::Vector3 defaultThemeColor
	{ // These values are from GUI
		0.5882353186607361f,
		0.8901960849761963f,
		0.6862745285034180f,
	};
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
		wsInitialPos.y -= Tile::unitWholeSize * 0.5f; // To bottom
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
		ImGui::DragFloat( u8"登場時の出現タイミング（秒）",&appearDelaySecond,		0.01f	);
		ImGui::DragFloat( u8"退場時の消滅タイミング（秒）",&leaveDelaySecond,		0.01f	);
		ImGui::DragInt  ( u8"最大体力",					&maxHP							);
		ImGui::DragInt  ( u8"最大残機数",				&maxRemainCount					);
		ImGui::DragInt  ( u8"初期残機数",				&initialRemainCount				);
		ImGui::DragFloat( u8"移動速度",					&moveSpeed,				0.01f	);
		ImGui::DragFloat( u8"慣性ジャンプの移動速度",		&inertialMoveSpeed,		0.01f	);
		ImGui::DragFloat( u8"スライディング速度",			&slideMoveSpeed,		0.01f	);
		ImGui::DragFloat( u8"スライディング秒数",			&slideMoveSecond,		0.01f	);
		ImGui::DragFloat( u8"ジャンプ力",				&jumpStrength,			0.01f	);
		ImGui::DragFloat( u8"ジャンプ解除時のＹ速度",		&jumpCancelledVSpeedMax,0.01f	);
		ImGui::DragFloat( u8"重力・上昇中",				&gravityRising,			0.01f	);
		ImGui::DragFloat( u8"重力加速・上昇中",			&gravityRisingAccel,	0.01f	);
		ImGui::DragFloat( u8"重力・下降中",				&gravityFalling,		0.01f	);
		ImGui::DragFloat( u8"重力加速・下降中",			&gravityFallingAccel,	0.01f	);
		ImGui::DragFloat( u8"最大重力",					&gravityMax,			0.01f	);
		ImGui::DragFloat( u8"最高落下速度",				&maxFallSpeed,			0.01f	);
		ImGui::DragFloat( u8"のけぞる秒数",				&knockBackSecond,		0.01f	);
		ImGui::DragFloat( u8"のけぞり速度",				&knockBackSpeed,		0.01f	);
		ImGui::DragFloat( u8"のけぞり抵抗時の短縮倍率",	&braceStandFactor,		0.01f	);
		ImGui::DragFloat( u8"無敵秒数",					&invincibleSecond,		0.01f	);
		ImGui::DragFloat( u8"無敵中点滅間隔（秒）",		&flushingInterval,		0.01f	);

		auto MakePositive	= []( float *v )
		{
			*v = std::max( 0.001f, *v );
		};
		maxHP				= std::max( 1, maxHP				);
		maxRemainCount		= std::max( 1, maxRemainCount		);
		initialRemainCount	= std::max( 1, initialRemainCount	);
		MakePositive( &moveSpeed			);
		MakePositive( &inertialMoveSpeed	);
		MakePositive( &slideMoveSpeed		);
		MakePositive( &slideMoveSecond		);
		MakePositive( &jumpStrength			);
		MakePositive( &gravityRising		);
		MakePositive( &gravityFalling		);
		MakePositive( &gravityMax			);
		MakePositive( &maxFallSpeed			);
		MakePositive( &knockBackSecond		);
		MakePositive( &knockBackSpeed		);
		MakePositive( &invincibleSecond		);
		MakePositive( &flushingInterval		);
		
		ImGui::TreePop();
	}
	if ( ImGui::TreeNode( u8"先行入力の受付秒数" ) )
	{
		ImGui::DragFloat( u8"ジャンプ",			&jumpBufferSecond,	0.01f );
		ImGui::DragFloat( u8"ショット",			&shotBufferSecond,	0.01f );
		ImGui::DragFloat( u8"スライディング",		&slideBufferSecond,	0.01f );

		shotBufferSecond  = std::max( 0.0f, shotBufferSecond  );
		slideBufferSecond = std::max( 0.0f, slideBufferSecond );
		jumpBufferSecond  = std::max( 0.0f, jumpBufferSecond  );

		ImGui::TreePop();
	}

	constexpr size_t motionCount = scast<size_t>( Player::MotionKind::MotionCount );
	if ( animePlaySpeeds.size() != motionCount )
	{
		animePlaySpeeds.resize( motionCount, 1.0f );
	}
	if ( animeTransSeconds.size() != motionCount )
	{
		animeTransSeconds.resize( motionCount, ModelHelper::SkinningOperator::Interpolation::defaultTransitionSecond );
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
		if ( ImGui::TreeNode( u8"遷移にかける秒数" ) )
		{
			for ( size_t i = 0; i < motionCount; ++i )
			{
				ImGui::DragFloat( KIND_NAMES[i], &animeTransSeconds[i], 0.01f );
			}

			ImGui::TreePop();
		}
		
		ImGui::TreePop();
	}

	if ( ImGui::TreeNode( u8"部分モーション設定" ) )
	{
		auto ShowElement = [&]( const char *partName, Player::MotionKind kind )
		{
			const int intKind = scast<int>( kind );
			auto found =  partMotions.find( intKind );
			if ( found == partMotions.end() )
			{
				auto inserted = partMotions.insert
				(
					std::make_pair( intKind, ModelHelper::PartApply{} )
				);
				found  = inserted.first;
			}

			ModelHelper::PartApply &element = found->second;
			element.ShowImGuiNode( partName );
		};

		ShowElement( u8"通常",					Player::MotionKind::Shot					);
		ShowElement( u8"チャージショット",		Player::MotionKind::ChargedShot				);
		ShowElement( u8"はしご・通常・左",		Player::MotionKind::LadderShotLeft			);
		ShowElement( u8"はしご・チャージ・左",	Player::MotionKind::LadderChargedShotLeft	);
		ShowElement( u8"はしご・通常・右",		Player::MotionKind::LadderShotRight			);
		ShowElement( u8"はしご・チャージ・右",	Player::MotionKind::LadderChargedShotRight	);

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

	constexpr size_t themeCount = scast<size_t>( Definition::WeaponKind::WeaponCount );
	if ( themeColors.size() != themeCount )
	{
		themeColors.resize( themeCount, Donya::Vector3{ 1.0f, 1.0f, 1.0f } );
	}
	if ( ImGui::TreeNode( u8"テーマ色設定" ) )
	{
		for ( size_t i = 0; i < themeCount; ++i )
		{
			ImGui::ColorEdit3
			(
				Definition::GetWeaponName( scast<Definition::WeaponKind>( i ) ),
				&themeColors[i].x
			);
		}
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
	constexpr float margin = 2.0f;
	const auto &data = Parameter().Get();

	jumpWasReleased_es.fill( false );
	for ( auto &it : jumps		) { it.Init( data.jumpBufferSecond  * margin ); }
	for ( auto &it : shots		) { it.Init( data.shotBufferSecond  * margin ); }
	for ( auto &it : dashes		) { it.Init( data.slideBufferSecond * margin ); }
	for ( auto &it : shiftGuns	) { it.first = it.second = 0; }

	moveVelocity		= Donya::Vector2::Zero();
	headToDestination	= false;
	wsDestination		= Donya::Vector3::Zero();
}
void Player::InputManager::Update( const Player &inst, float elapsedTime, const Input &input )
{
#if USE_IMGUI
	{
		constexpr float margin = 2.0f;
		const auto &data = Parameter().Get();

		constexpr size_t bufferCount = 3;
		using BufferT = std::array<::Input::BufferedInput, Input::variationCount>;
		std::array<BufferT *, bufferCount> bufferPtrs
		{
			&jumps,
			&shots,
			&dashes,
		};
		std::array<float, bufferCount> bufferSeconds
		{
			data.jumpBufferSecond  * margin,
			data.shotBufferSecond  * margin,
			data.slideBufferSecond * margin,
		};

		for ( size_t i = 0; i < bufferCount; ++i )
		{
			for ( int buf = 0; buf < Input::variationCount; ++buf )
			{
				auto &buffer = bufferPtrs[i]->at( buf );
				if (  buffer.GetLifeSpan() < bufferSeconds[i] )
				{
					buffer.SetLifeSpan( bufferSeconds[i] );
				}
			}
		}
	}
#endif // USE_IMGUI


	RegisterCurrentInputs( Donya::GetElapsedTime(), input );

	if ( headToDestination )
	{
		const Donya::Vector3 diff = wsDestination - inst.GetPosition();
		moveVelocity.x = Donya::SignBitF( diff.x );
		moveVelocity.y = Donya::SignBitF( diff.y );
	}

	const auto &depth = Parameter().Get().jumpBufferSecond;
	for ( int i = 0; i < Input::variationCount; ++i )
	{
		if ( jumps[i].PressingSecond( depth ) < 0.0f )
		{
			jumpWasReleased_es[i] = true;
		}
	}
}
bool Player::InputManager::NowJumpable() const
{
	const int index = IndexOfUsingJump();
	if ( index < 0 ) { return false; }
	// else

	return jumpWasReleased_es[index];
}
bool Player::InputManager::NowUseJump() const
{
	return ( 0 <= IndexOfUsingJump() );
}
bool Player::InputManager::NowReleaseJump() const
{
	return ( 0 <= IndexOfReleasingJump() );
}
bool Player::InputManager::NowUseShot() const
{
	return ( 0 <= IndexOfUsingShot() );
}
bool Player::InputManager::NowTriggerShot() const
{
	return ( 0 <= IndexOfTriggeringShot() );
}
bool Player::InputManager::NowUseDash() const
{
	return ( 0 <= IndexOfUsingDash() );
}
int  Player::InputManager::NowShiftGun() const
{
	const int index = IndexOfShiftingGun();
	return (  index < 0 ) ? 0 : shiftGuns[index].first;
}
void Player::InputManager::DetainNowJumpInput()
{
	const auto &depth = Parameter().Get().jumpBufferSecond;
	for ( int i = 0; i < Input::variationCount; ++i )
	{
		if ( 0.0f <= jumps[i].PressingSecond( depth ) )
		{
			jumpWasReleased_es[i] = false;
		}
	}
}
const Donya::Vector2 &Player::InputManager::CurrentMoveDirection() const
{
	return moveVelocity;
}
bool Player::InputManager::NowHeading() const
{
	return headToDestination;
}
Donya::Vector3 Player::InputManager::HeadingDestinationOrOrigin() const
{
	return ( NowHeading() ) ? wsDestination : Donya::Vector3::Zero();
}
void Player::InputManager::RegisterCurrentInputs( float elapsedTime, const Input &input )
{
	for ( int i = 0; i < Input::variationCount; ++i )
	{
		jumps [i].Update( elapsedTime, input.useJumps [i] );
		shots [i].Update( elapsedTime, input.useShots [i] );
		dashes[i].Update( elapsedTime, input.useDashes[i] );

		shiftGuns[i].second = shiftGuns[i].first;
		shiftGuns[i].first  = input.shiftGuns[i];
	}

	moveVelocity		= input.moveVelocity;
	headToDestination	= input.headToDestination;
	wsDestination		= input.wsDestination;
}
int  Player::InputManager::IndexOfUsingJump() const
{
	const auto &depth = Parameter().Get().jumpBufferSecond;
	for ( int i = 0; i < Input::variationCount; ++i )
	{
		if ( !jumpWasReleased_es[i] ) { continue; }
		// else

		if ( 0.0f <= jumps[i].PressingSecond( depth ) )
		{
			return i;
		}
	}

	return -1;
}
int  Player::InputManager::IndexOfReleasingJump() const
{
	const auto &depth = Parameter().Get().jumpBufferSecond;
	for ( int i = 0; i < Input::variationCount; ++i )
	{
		if ( jumps[i].IsReleased( depth ) )
		{
			return i;
		}
	}

	return -1;
}
int  Player::InputManager::IndexOfUsingShot() const
{
	for ( int i = 0; i < Input::variationCount; ++i )
	{
		if ( 0.0f <= shots[i].PressingSecond( FLT_MAX ) )
		{
			return i;
		}
	}

	return -1;
}
int  Player::InputManager::IndexOfTriggeringShot() const
{
	const auto &depth = Parameter().Get().shotBufferSecond;
	for ( int i = 0; i < Input::variationCount; ++i )
	{
		if ( shots[i].IsTriggered( depth ) )
		{
			return i;
		}
	}

	return -1;
}
int  Player::InputManager::IndexOfUsingDash() const
{
	const auto &depth = Parameter().Get().slideBufferSecond;
	for ( int i = 0; i < Input::variationCount; ++i )
	{
		if ( dashes[i].IsTriggered( depth ) ) { return i; }
	}

	return -1;
}
int  Player::InputManager::IndexOfShiftingGun() const
{
	for ( int i = 0; i < Input::variationCount; ++i )
	{
		if ( shiftGuns[i].first && !shiftGuns[i].second )
		{
			return i;
		}
	}

	return -1;
}
#if USE_IMGUI
void Player::InputManager::ShowImGuiNode( const std::string &nodeCaption )
{
	if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
	// else

	auto ShowBuffer = []( const char *columnId, std::array<::Input::BufferedInput, Input::variationCount> *p )
	{
		std::string caption;

		ImGui::Columns( Input::variationCount, columnId );
		ImGui::Separator();

		// Header
		for ( int i = 0; i < Input::variationCount; ++i )
		{
			caption = "Key" + Donya::MakeArraySuffix( i );
			ImGui::Text( caption.c_str() );

			ImGui::NextColumn();
		}
		ImGui::Separator();

		// Content
		for ( int i = 0; i < Input::variationCount; ++i )
		{
			p->at( i ).ShowImGuiNode( nullptr );
			ImGui::NextColumn();
		}

		ImGui::Columns( 1 );
		ImGui::Separator();
	};

	if ( ImGui::TreeNode( u8"ジャンプ" ) )
	{
		ShowBuffer( "COL_Jump", &jumps );

		ImGui::TreePop();
	}
	if ( ImGui::TreeNode( u8"ショット" ) )
	{
		ShowBuffer( "COL_Shot", &shots );

		ImGui::TreePop();
	}
	if ( ImGui::TreeNode( u8"ダッシュ" ) )
	{
		ShowBuffer( "COL_Dash", &dashes );

		ImGui::TreePop();
	}

	ImGui::TreePop();
}
#endif // USE_IMGUI

void Player::MotionManager::Init()
{
	prevKind = currKind = MotionKind::Jump_Fall;

	auto resource = GetModelOrNullptr();
	model.Initialize( resource );
	AssignPose( currKind );

	
	shotAnimator.ResetTimer();
	shotAnimator.DisableLoop();
	shotWasCharged = false;
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

	model.AdvanceInterpolation( elapsedTime );
	
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

	pRenderer->Render( model.pResource->model, model.GetCurrentPose() );

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
bool Player::MotionManager::NowShotPoses() const
{
	return shouldPoseShot;
}
void Player::MotionManager::QuitShotMotion()
{
	shouldPoseShot = false;
	shotAnimator.ResetTimer();
}
void Player::MotionManager::OverwriteLerpSecond( float newSecond )
{
	model.SetInterpolationSecond( newSecond );
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
		shotWasCharged = IsFullyCharged( inst.shotManager.ChargeLevel() );
		shotAnimator.ResetTimer();
	}

	if ( !shouldPoseShot ) { return; }
	// else

	const auto &data = Parameter().Get();

	MotionKind applyKind = MotionKind::Shot;
	if ( inst.pMover && inst.pMover->NowGrabbingLadder( inst ) )
	{
		applyKind	= ( inst.lookingSign < 0.0f )
					? MotionKind::LadderShotLeft
					: MotionKind::LadderShotRight;
	}

	if ( shotWasCharged )
	{
		// Charged versions are located next
		size_t intKind = scast<size_t>( applyKind );
		intKind++;
		intKind = std::min( intKind, MOTION_COUNT - 1 );
		applyKind = scast<MotionKind>( intKind );
	}

	ApplyPartMotion( inst, elapsedTime, applyKind );
}
void Player::MotionManager::ApplyPartMotion( Player &inst, float elapsedTime, MotionKind useMotionKind )
{
	if ( !model.pResource ) { return; }
	// else

	const auto &data	= Parameter().Get();
	const auto &holder	= model.pResource->motionHolder;

	const auto foundItr = data.partMotions.find( scast<int>( useMotionKind ) );
	if ( foundItr == data.partMotions.end() )
	{
		shouldPoseShot = false;
		return;
	}

	const ModelHelper::PartApply partData = foundItr->second;

	const size_t motionIndex = holder.FindMotionIndex( partData.motionName );
	if ( holder.GetMotionCount() <= motionIndex )
	{
		shouldPoseShot = false;
		return;
	}
	// else

	// Update "shotAnimator" and "shotPose" by "motionIndex"
	{
		const auto &motion = holder.GetMotion( motionIndex );
		shotAnimator.SetRepeatRange( motion );

		const size_t motionKindIndex	= scast<size_t>( useMotionKind );
		const float  motionAcceleration	= ( data.animePlaySpeeds.size() <= motionKindIndex ) ? 1.0f : data.animePlaySpeeds[motionKindIndex];
		shotAnimator.Update( fabsf( elapsedTime ) * motionAcceleration );

		shotPose.AssignSkeletal( shotAnimator.CalcCurrentPose( motion ) );
	}

	Donya::Model::Pose &refCurrentPose = model.GetCurrentPose();

	std::vector<Donya::Model::Animation::Node> editNodes = refCurrentPose.GetCurrentPose();
	assert( shotPose.HasCompatibleWith( editNodes ) );

	std::vector<size_t> applyBoneIndices{};
	for ( const auto &rootName : partData.applyRootBoneNames )
	{
		ExplorePartBone( &applyBoneIndices, editNodes, rootName );
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

	const std::vector<Donya::Model::Animation::Node> &destNodes = shotPose.GetCurrentPose();
	for ( auto &i : applyBoneIndices )
	{
		auto &node = editNodes[i];
		auto &dest = destNodes[i];

		node.bone  = dest.bone;
		node.local = dest.local;

		// If the current node is the root bone of apply targets
		if ( IsTargetRootName( node.bone.name ) )
		{
			const auto &n = node.global;
			const auto &d = dest.global;

			Donya::Vector4x4 blend;
			// Apply destination's S and R
			blend._11 = d._11;	blend._12 = d._12;	blend._13 = d._13;
			blend._21 = d._21;	blend._22 = d._22;	blend._23 = d._23;
			blend._31 = d._31;	blend._32 = d._32;	blend._33 = d._33;

			// Blend the translations
			blend._41 = Donya::Lerp( n._41, d._41, partData.rootTranslationBlendPercent.x );
			blend._42 = Donya::Lerp( n._42, d._42, partData.rootTranslationBlendPercent.y );
			blend._43 = Donya::Lerp( n._43, d._43, partData.rootTranslationBlendPercent.z );
			node.global = blend;
		}
		else
		{
			node.global = ( node.bone.parentIndex == -1 )
						? dest.local
						: dest.local * editNodes[node.bone.parentIndex].global;
		}
	}

	refCurrentPose.AssignSkeletal( editNodes );
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

	if ( kind != prevKind )
	{
		const auto &transSeconds = Parameter().Get().animeTransSeconds;
		const bool indexIsSafe = ( 0 <= motionIndex && motionIndex < scast<int>( transSeconds.size() ) );
		if ( indexIsSafe )
		{
			model.SetInterpolationSecond( transSeconds[motionIndex] );
		}
		else
		{
			model.SetInterpolationSecond( 0.0f );
		}
	}

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
	case MotionKind::Winning:			return false;
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

	if ( inst.pMover && inst.pMover->NowWinning( inst ) )
	{
		return MotionKind::Winning;
	}
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
	if ( inst.inputManager.CurrentMoveDirection().y < 0.0f ) { return MotionKind::Brace; }
	// else
	return MotionKind::Idle;
}

Player::ShotManager::~ShotManager()
{
	Uninit();
}
void Player::ShotManager::Init()
{
	chargeLevel		= ShotLevel::Normal;
	chargeSecond	= 0.0f;
	currUseShot		= false;
	prevUseShot		= false;
	fxComplete.Disable();
	fxLoop.Disable();

	StopLoopSFXIfPlaying();
}
void Player::ShotManager::Uninit()
{
	StopLoopSFXIfPlaying( /* forcely = */ true );
}
void Player::ShotManager::Update( const Player &inst, float elapsedTime )
{
	// Make do not release/charge when pausing
	if ( IsZero( elapsedTime )	) { return; }
	if ( inst.NowMiss()			) { return; }
	// else

	const ShotLevel oldChargeLevel = chargeLevel;
	// If calculate the "chargeLevel" after ChargeUpdate(),
	// the level will be zero(ShotLevel::Normal) absolutely when fire timing, because that timing is the input was released.
	// So I must calculate it before the update. It will not be late for one frame by this.
	chargeLevel		= CalcChargeLevel  ( chargeSecond );
	destColor		= CalcEmissiveColor( chargeSecond );
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
			AssignLoopFX( Effect::Kind::Charge_Loop_Charged );
			Donya::Sound::Play( Music::Charge_Complete );
		}
	}

	ChargeUpdate( inst, elapsedTime );

	SetFXPosition( inst.GetPosition() );
}
void Player::ShotManager::ChargeFully()
{
	constexpr int maxLevelIndex = scast<int>( ShotLevel::Strong );

	const auto &chargeParams = Parameter().Get().chargeParams;
	chargeSecond = chargeParams[maxLevelIndex].chargeSecond + 1.0f;
}
void Player::ShotManager::SetFXPosition( const Donya::Vector3 &wsPos )
{
	fxComplete	.SetPosition( wsPos );
	fxLoop		.SetPosition( wsPos );
}
bool Player::ShotManager::IsShotRequested( const Player &inst ) const
{
	if ( NowTriggered( inst ) ) { return true; }
	// else

	const bool allowReleaseFire = ( inst.pGun && inst.pGun->AllowFireByRelease( chargeLevel ) );
	if ( !allowReleaseFire ) { return false; }
	// else

	const bool released = ( !currUseShot && prevUseShot );
	return released;
}
bool Player::ShotManager::NowTriggered( const Player &inst ) const
{
	return inst.inputManager.NowTriggerShot() || ( currUseShot && !prevUseShot );
}
Player::ShotLevel Player::ShotManager::CalcChargeLevel( float chargingSecond ) const
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
		if ( chargeParams[scast<int>( it )].chargeSecond <= chargingSecond )
		{
			return it;
		}
	}

	return ShotLevel::Normal;
}
Donya::Vector3 Player::ShotManager::CalcEmissiveColor( float chargingSecond ) const
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
		if ( chargingSecond < itr->chargeSecond ) { continue; }
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

	const float angle		= ToRadian( chargingSecond * 360.0f );
	const float cycleSpeed	= 1.0f / pParam->emissiveCycleSecond;
	const float sin_01		= ( sinf( angle * cycleSpeed ) + 1.0f ) * 0.5f;
	const float sinRange	= 1.0f - pParam->emissiveMinBias;
	const float easeFactor	= ( sin_01 * sinRange ) + pParam->emissiveMinBias;
	const float colorFactor = Donya::Easing::Ease( pParam->emissiveEaseKind, pParam->emissiveEaseType, easeFactor );

	return pParam->emissiveColor.Product( colorFactor );
}
void Player::ShotManager::AssignLoopFX( Effect::Kind kind )
{
	fxLoop.Stop();
	fxLoop.Disable();

	// Actual position will set at the end of Update()
	constexpr Donya::Vector3 generatePos = Donya::Vector3::Zero();
	fxLoop = Effect::Handle::Generate( kind, generatePos );
}
void Player::ShotManager::ChargeUpdate( const Player &inst, float elapsedTime )
{
	prevUseShot = currUseShot;
	currUseShot = inst.inputManager.NowUseShot();

	if ( NowTriggered( inst ) )
	{
		chargeSecond = 0.0f;
		StopLoopSFXIfPlaying();
		return;
	}
	// else

	const bool chargeable = ( inst.pGun && inst.pGun->Chargeable() );
	if ( chargeable && currUseShot )
	{
		chargeSecond += elapsedTime;

		const bool shouldShow = ( CalcChargeLevel( chargeSecond ) != ShotLevel::Normal );
		if ( shouldShow )
		{
			PlayLoopSFXIfStopping();
		}

		return;
	}
	// else

	chargeSecond = 0.0f;
	StopLoopSFXIfPlaying();
}
void Player::ShotManager::PlayLoopSFXIfStopping()
{
	if ( playingChargeSE					) { return; }
	if ( chargeLevel == ShotLevel::Normal	) { return; }
	// else

	AssignLoopFX( Effect::Kind::Charge_Loop );

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
void Player::Flusher::Start( float flushingSecond )
{
	workingSecond	= flushingSecond;
	timer			= 0.0f;
	fxHurt			= Effect::Handle::Generate( Effect::Kind::HurtDamage, {} );
}
void Player::Flusher::Update( const Player &inst, float elapsedTime )
{
	timer += elapsedTime;
	SetFXPosition( inst.GetPosition() );
}
void Player::Flusher::SetFXPosition( const Donya::Vector3 &wsPos )
{
	fxHurt.SetPosition( wsPos );
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
	return ( timer < workingSecond ) ? true : false;
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
	const auto prevPos = inst.GetPosition();

	const auto movement		= inst.velocity * elapsedTime;
	const auto aroundSolids	= inst.FetchAroundSolids( inst.GetHitBox(), movement, terrain );
	inst.Actor::MoveX( movement.x, aroundSolids );
	inst.Actor::MoveZ( movement.z, aroundSolids );

	const auto currPos = inst.GetPosition();
	const auto &input = inst.inputManager;
	if ( input.NowHeading() )
	{
		const Donya::Vector3 dest = input.HeadingDestinationOrOrigin();

		const auto prevDiff		= dest - prevPos;
		const auto currDiff		= dest - currPos;
		const auto nowOveredX	= ( Donya::SignBit( currDiff.x ) != Donya::SignBit( prevDiff.x ) ) || IsZero( currDiff.x );
		const auto nowOveredZ	= ( Donya::SignBit( currDiff.z ) != Donya::SignBit( prevDiff.z ) ) || IsZero( currDiff.z );
		if ( nowOveredX ) { inst.body.pos.x = dest.x; }
		if ( nowOveredZ ) { inst.body.pos.z = dest.z; }
	}

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
	inst.MoveHorizontal( elapsedTime );
	MoveVertical( inst, elapsedTime, terrain ); // My method

	inst.ShotIfRequested( elapsedTime );

	const Donya::Vector2 &moveDir = inst.inputManager.CurrentMoveDirection();

	// Try to grabbing ladder if the game time is not pausing
	if ( !gotoSlide && !IsZero( elapsedTime ) )
	{
		const auto pLadder = inst.FindGrabbingLadderOrNullptr( moveDir.y, terrain );
		if ( pLadder )
		{
			inst.pTargetLadder = pLadder;
			gotoLadder = true;
		}
	}

	braceOneself = ( moveDir.y < 0.0f && inst.onGround && IsZero( inst.velocity.x ) );
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
void Player::Normal::MoveVertical( Player &inst, float elapsedTime, const Map &terrain )
{
	// Deformity of inst.MoveVertical()

	// Make to can not act if game time is pausing
	if ( IsZero( elapsedTime ) )
	{
		inst.Fall( elapsedTime );
		return;
	}
	// else

	const auto &input = inst.inputManager;

	// The buffer-input process of NowUseDash() will discards a recorded input.
	// So if you evaluate the NowUseDash() when can not use the slide,
	// the buffered input will be discarded wastefully.
	const bool useSlide = inst.onGround && input.NowUseDash();

	// Jump condition and resolve vs slide condition
	if ( inst.WillUseJump() )
	{
		const bool pressDown = input.CurrentMoveDirection().y < 0.0f;
		if ( pressDown )
		{
			gotoSlide = true;

			// Certainly doing Fall() if do not jump
			inst.Fall( elapsedTime );

			// Make do not take this jump input in next status
			inst.inputManager.DetainNowJumpInput();

			return;
		}
		// else

		inst.Jump();

		// Enable the inertial-like jump even if was inputted the "jump" and "slide" in same time
		if ( useSlide )
		{
			inst.wasJumpedWhileSlide = true;
			inst.GenerateSlideEffects();
		}

		return;
	}
	// else

	if ( useSlide )
	{
		gotoSlide = true;
	}

	inst.Fall( elapsedTime );
}

void Player::Slide::Init( Player &inst )
{
	MoverBase::Init( inst );

	nextStatus	= Destination::None;
	timer		= 0.0f;

	inst.lookingSign = Donya::SignBitF( inst.orientation.LocalFront().x );
	if ( IsZero( inst.lookingSign ) ) { inst.lookingSign = 1.0f; } // Fail safe

	inst.velocity.x = Parameter().Get().slideMoveSpeed * inst.lookingSign;
	inst.velocity.y = 0.0f;
	inst.velocity.z = 0.0f;
	inst.UpdateOrientation( /* lookingRight = */ ( inst.lookingSign < 0.0f ) ? false : true );

	inst.GenerateSlideEffects();
}
void Player::Slide::Uninit( Player &inst )
{
	MoverBase::Uninit( inst );

	inst.velocity.x = 0.0f;
}
void Player::Slide::Update( Player &inst, float elapsedTime, const Map &terrain )
{
	timer += elapsedTime;

	const Donya::Vector2 &inputDir = inst.inputManager.CurrentMoveDirection();

	const int  horizontalInputSign	= Donya::SignBit( inputDir.x );
	const bool moveToBackward		= ( horizontalInputSign != 0 ) && ( horizontalInputSign != Donya::SignBit( inst.lookingSign ) );
	const bool useJump				= inst.WillUseJump();
	const auto pGrabbingLadder		= inst.FindGrabbingLadderOrNullptr( inputDir.y, terrain );

	const bool slideIsEnd =
					( Parameter().Get().slideMoveSecond <= timer )
				||	( moveToBackward	)
				||	( useJump			)
				||	( pGrabbingLadder	)
				||	( !inst.onGround	)
				;

	if ( slideIsEnd && !IsZero( elapsedTime ) ) // Make to can not act if game time is pausing
	{
		const Donya::Collision::Box3F normalBody = inst.GetNormalBody( /* ofHurtBox = */ false );
		
		if ( inst.WillCollideToAroundTiles( normalBody, inst.velocity * elapsedTime, terrain ) )
		{
			// I can not finish the sliding now, because I will be buried.
			
			// And discard the jump input for prevent the continuously trying jump.
			if ( useJump )
			{
				inst.inputManager.DetainNowJumpInput();

				// But the input must reflect to action. The player will jump while slide.
				inst.Jump();
			}

			if ( moveToBackward )
			{
				inst.velocity.x		*= -1.0f;
				inst.lookingSign	*= -1.0f;

				const auto halfTurn = Donya::Quaternion::Make( Donya::Vector3::Up(), ToRadian( 180.0f ) );
				inst.orientation.RotateBy( halfTurn );
				inst.orientation.Normalize();
			}
		}
		else
		{
			// I can finish the sliding here.

			if ( pGrabbingLadder )
			{
				inst.pTargetLadder = pGrabbingLadder;
				nextStatus = Destination::Ladder;
			}
			else
			{
				nextStatus = Destination::Normal;
			}
		}
	}

	if ( !IsZero( inst.velocity.x ) )
	{
		inst.lookingSign = Donya::SignBitF( inst.velocity.x );
	}

	// If the "slideIsEnd" to be true by jump input, the jump will be fired in here.
	inst.MoveVertical( elapsedTime );

	inst.ShotIfRequested( elapsedTime );

	MotionUpdate( inst, elapsedTime );
}
void Player::Slide::Move( Player &inst, float elapsedTime, const Map &terrain, float roomLeftBorder, float roomRightBorder )
{
	MoveOnlyHorizontal( inst, elapsedTime, terrain, roomLeftBorder, roomRightBorder );
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
	case Destination::Ladder: return [&inst]() { inst.AssignMover<GrabLadder>(); };
	default: break;
	}

	// Fail safe
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

	// Prevent to be exposed the above reversing when grabbing a ladder when the player poses the shot motion
	inst.motionManager.QuitShotMotion();

	inst.velocity	= 0.0f;
	LookToFront( inst );

	inst.onGround				= false;
	inst.wasJumpedWhileSlide	= false;

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

	// Shot pose's face looks to side.
	// So I want to prevent the face looks here or back side when end the grab motion,
	// by interpolate the motion as immediately.
	if ( inst.motionManager.NowShotPoses() )
	{
		inst.motionManager.OverwriteLerpSecond( 0.0f );
		// Then apply that overwrite
		MotionUpdate( inst, 0.0001f, /* stopAnimation = */ true );
	}
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

	// The NowUnderShotLag() result may different from above condition because the "shotLagSecond" is changed.
	if ( !NowUnderShotLag() )
	{
		const Donya::Vector2 &inputDir = inst.inputManager.CurrentMoveDirection();
		inst.velocity.y = data.ladderMoveSpeed * inputDir.y;
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
	shotLagSecond = data.ladderShotLagSecond;

	const int sideInputSign = Donya::SignBit( inst.inputManager.CurrentMoveDirection().x );
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

	if ( inst.onGround ) { return ReleaseWay::Dismount; }
	// else

	const auto &input = inst.inputManager;
	
	// Condition of releasing by jump input
	const bool releasible =	( input.CurrentMoveDirection().y <= 0.0f )	// Prevent a grab-release loop by press keeping the jump and the up input
							&& !IsZero( elapsedTime );					// Make to can not act if game time is pausing
	if ( input.NowUseJump() && releasible )
	{
		// Make do not take this jump input in next status.
		inst.inputManager.DetainNowJumpInput();

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
	// else


	if ( input.NowHeading() )
	{
		const Donya::Vector3 diff = input.HeadingDestinationOrOrigin() - inst.GetPosition();
		if ( !Donya::SignBit( diff.y ) )
		{
			return ReleaseWay::Release;
		}
	}

	return releaseWay;
}

void Player::KnockBack::Init( Player &inst )
{
	MoverBase::Init( inst );

	const auto &data = Parameter().Get();

	const bool knockedFromRight =
	#if 0 // CONSIDER_IMPACT_DIRECTION
		( inst.pReceivedDamage && inst.pReceivedDamage->knockedFromRight )
	#else
		( inst.lookingSign < 0.0f ) ? false : true
	#endif // CONSIDER_IMPACT_DIRECTION
		;

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
	const auto &knockBackSecoonds = Parameter().Get().knockBackSecond;
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

	// Release some bullet instance
	if ( inst.pGun ) { inst.pGun->Uninit( inst ); }

	// Stop charging Effect/Sound
	inst.shotManager.Uninit();

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
	inst.motionManager.QuitShotMotion();

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
	inst.AssignGun<BusterGun>(); // Discard shield if the ShieldGun is assigned

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

void Player::WinningPose::Init( Player &inst )
{
	MoverBase::Init( inst );

	inst.velocity		= Donya::Vector3::Zero();
	inst.hurtBox.exist	= false;
	inst.motionManager.QuitShotMotion();
	inst.AssignGun<BusterGun>(); // Discard shield if the ShieldGun is assigned

	inst.UpdateOrientation( /* lookingRight = */ true );
}
void Player::WinningPose::Uninit( Player &inst )
{
	MoverBase::Uninit( inst );

	inst.UpdateOrientation( /* lookingRight = */ ( inst.lookingSign < 0.0f ) ? false : true );
}
void Player::WinningPose::Update( Player &inst, float elapsedTime, const Map &terrain )
{
	MotionUpdate( inst, elapsedTime );
}
void Player::WinningPose::Move( Player &inst, float elapsedTime, const Map &terrain, float roomLeftBorder, float roomRightBorder )
{
	// No op
}
bool Player::WinningPose::ShouldChangeMover( const Player &inst ) const
{
	return inst.motionManager.WasCurrentMotionEnded();
}
std::function<void()> Player::WinningPose::GetChangeStateMethod( Player &inst ) const
{
	return [&inst]() { inst.AssignMover<Normal>(); };
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
Donya::Vector3 Player::GunBase::GetThemeColor() const
{
	const auto &source = Parameter().Get().themeColors;
	const size_t sourceCount = source.size();
	if ( sourceCount != scast<size_t>( Definition::WeaponKind::WeaponCount ) ) { return defaultThemeColor; }
	// else

	const size_t index = scast<size_t>( GetKind() );
	if ( sourceCount <= index ) { return defaultThemeColor; }
	// else

	return source[index];
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
	if ( !pInstance || pInstance->WasProtected() )
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
	const Donya::Vector2 stick = input.CurrentMoveDirection().Unit();
	if ( stick.IsZero() )
	{
		return	( inst.lookingSign < 0.0f ) // Except the front
				? -Donya::Vector3::Right()
				: Donya::Vector3::Right();
	}
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
	pInstance->AllowRemovingByOutOfScreen();
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
	wasJumpedWhileSlide	= false;

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


	availableWeapon.Reset();
	const auto &saveData = SaveData::Admin::Get().NowData();
	ApplyAvailableWeapon( saveData.availableWeapons );

	AssignGun<BusterGun>();
}
void Player::Uninit()
{
	shotManager.Uninit();
	if ( pMover ) { pMover->Uninit( *this ); }
	pTargetLadder.reset();
}
void Player::Update( float elapsedTime, const Input &input, const Map &terrain )
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

	inputManager.Update( *this, elapsedTime, input );

	hurtBox.UpdateIgnoreList( elapsedTime );

	shotManager.Update( *this, elapsedTime );

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

	ShiftGunIfNeeded( elapsedTime );
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

	const Donya::Vector3 movedPos = GetPosition();
	shotManager		.SetFXPosition( movedPos );
	invincibleTimer	.SetFXPosition( movedPos );
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
void Player::ChargeFully()
{
	shotManager.ChargeFully();
}
void Player::ApplyAvailableWeapon( const Definition::WeaponAvailableStatus &newStatus )
{
	availableWeapon = newStatus;
}
void Player::ApplyAvailableWeapon( const Definition::WeaponKind &kind )
{
	availableWeapon.Activate( kind );
}
bool Player::NowMiss() const
{
	return ( !pMover ) ? true : ( pMover->NowMiss( *this ) ) ? true : false;
}
bool Player::NowGrabbingLadder() const
{
	return ( pMover && pMover->NowGrabbingLadder( *this ) ) ? true : false;
}
bool Player::NowWinningPose() const
{
	return ( pMover && pMover->NowWinning( *this ) ) ? true : false;
}
int  Player::GetCurrentHP() const
{
	return currentHP;
}
Donya::Vector3 Player::GetVelocity() const
{
	return velocity;
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
Donya::Vector3			Player::GetThemeColor() const
{
	return ( pGun ) ? pGun->GetThemeColor() : defaultThemeColor;
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
void Player::PerformWinning()
{
	AssignMover<WinningPose>();
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
		invincibleTimer.Start( Parameter().Get().invincibleSecond );

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
void Player::AssignGunByKind( Definition::WeaponKind kind )
{
	using WP = Definition::WeaponKind;

	_ASSERT_EXPR( 2 == scast<int>( WP::WeaponCount ), L"Please add the new kind's case to switch statement!" );

	switch ( kind )
	{
	case WP::Buster:
		AssignGun<BusterGun>();
		return;
	case WP::SkullShield:
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
std::shared_ptr<const Tile> Player::FindGrabbingLadderOrNullptr( float verticalInput, const Map &terrain ) const
{
	const int inputSign = Donya::SignBit( verticalInput );
	if ( inputSign == 0 ) { return nullptr; }
	// else

	auto IsLadder = [&]( const std::shared_ptr<const Tile> &targetTile )
	{
		return ( targetTile && targetTile->GetID() == StageFormat::Ladder ) ? true : false;
	};
	auto FindLadderOrNullptr = [&]( const Donya::Collision::Box3F &wsVerifyArea )->std::shared_ptr<const Tile>
	{
		const auto targetTiles = terrain.GetPlaceTiles( wsVerifyArea );
		for ( const auto &tile : targetTiles )
		{
			if ( IsLadder( tile ) )
			{
				return tile;
			}
		}

		return nullptr;
	};

	std::shared_ptr<const Tile> tile = nullptr;

	if ( inputSign == 1 )
	{
		// Search around the my body
		tile = FindLadderOrNullptr( GetLadderGrabArea() );
		if ( tile ) { return tile; }
		// else

		// Search around above if still does not grabbing

		const Donya::Vector3 headOffset{ 0.0f, body.size.y, 0.0f };
		const Donya::Vector3 abovePosition = GetPosition() + headOffset;
		tile = terrain.GetPlaceTileOrNullptr( abovePosition );
		return ( IsLadder( tile ) ) ? tile : nullptr;
	}
	// else

	// Search around the my foot if standing
	if ( onGround )
	{
		constexpr Donya::Vector3 verticalOffset{ 0.0f, Tile::unitWholeSize, 0.0f };
		const     Donya::Vector3 underPosition = GetPosition() - verticalOffset;
		tile = terrain.GetPlaceTileOrNullptr( underPosition );
		return ( IsLadder( tile ) ) ? tile : nullptr;
	}
	// else

	return nullptr;
}
std::vector<Donya::Collision::Box3F> Player::FetchAroundSolids( const Donya::Collision::Box3F &body, const Donya::Vector3 &movement, const Map &terrain ) const
{
	const auto aroundTiles	= terrain.GetPlaceTiles( body, movement );
		  auto aroundSolids	= Map::ToAABBSolids( aroundTiles, terrain, body );
	Donya::AppendVector( &aroundSolids, terrain.GetExtraSolids() );

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

	const Donya::Vector2 &inputDir = inputManager.CurrentMoveDirection();
	const float  speed = ( wasJumpedWhileSlide ) ? data.inertialMoveSpeed : data.moveSpeed;
	velocity.x = speed * inputDir.x;

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
		Jump();
	}
	else
	{
		Fall( elapsedTime );
	}
}
bool Player::NowShotable( float elapsedTime ) const
{
	if ( IsZero( elapsedTime )				) { return false; } // If game time is pausing
	if ( !pMover							) { return false; }
	if ( pMover->NowKnockBacking( *this )	) { return false; }
	if ( pMover->NowAppearing	( *this )	) { return false; }
	if ( pMover->NowWinning		( *this )	) { return false; }
	if ( inputManager.NowHeading()			) { return false; } // It will be true when such as performance. Charge is allowed, but Shot is not allowed.
	// else

	const bool generatable = ( Bullet::Buster::GetLivingCount() < Parameter().Get().maxBusterCount );
	return ( generatable ) ? true : false;
}
void Player::ShotIfRequested( float elapsedTime )
{
	if ( !pGun									) { return; }
	if ( !shotManager.IsShotRequested( *this )	) { return; }
	if ( !NowShotable( elapsedTime )			) { return; }
	// else

	pGun->Fire( *this, inputManager );
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

	onGround			= false;
	wasJumpedWhileSlide	= prevSlidingStatus;
	velocity.y			= data.jumpStrength;
	nowGravity			= data.gravityRising;

	Donya::Sound::Play( Music::Player_Jump );
}
bool Player::Jumpable() const
{
	if ( pMover && pMover->NowSliding( *this ) )
	{
		const bool pressDown = ( inputManager.CurrentMoveDirection().y < 0.0f );
		if ( pressDown ) { return false; }
		// else
	}

	return ( onGround && inputManager.NowJumpable() ) ? true : false;
}
bool Player::WillUseJump() const
{
	return ( Jumpable() && inputManager.NowUseJump() );
}
void Player::Fall( float elapsedTime )
{
	const auto &data = Parameter().Get();


	if ( onGround )
	{
		nowGravity = data.gravityRising;
	}
	else
	{
		const float  &acceleration = ( 0.0f <= velocity.y ) ? data.gravityRisingAccel : data.gravityFallingAccel;
		nowGravity += acceleration * elapsedTime;
	}

	nowGravity = std::max( -data.gravityMax, nowGravity );


	const float oldVSpeed = velocity.y;

	// Control the Y velocity when the moment that jump input now released
	const bool nowReleaseMoment = inputManager.NowReleaseJump();
	if ( nowReleaseMoment && 0.0f < velocity.y ) // Enable only rising
	{
		velocity.y = std::min( velocity.y, data.jumpCancelledVSpeedMax );
	}
	else // Apply the gravity as usually
	{
		velocity.y -= nowGravity * elapsedTime;
	}
	velocity.y = std::max( -data.maxFallSpeed, velocity.y );

	if ( Donya::SignBit( velocity.y ) != Donya::SignBit( oldVSpeed ) )
	{
		nowGravity = ( 0.0f <= velocity.y ) ? data.gravityRising : data.gravityFalling;
	}
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
	wasJumpedWhileSlide = false;
}
void Player::ShiftGunIfNeeded( float elapsedTime )
{
	if ( !pGun		) { return; }
	if ( NowMiss()	) { return; }
	// else

	// Make can not shift by button when pausing
	if ( IsZero( elapsedTime ) ) { return; }
	// else

	const int shiftSign = inputManager.NowShiftGun();
	if ( !shiftSign ) { return; }
	// else

	using WP = Definition::WeaponKind;
	constexpr int kindCount = scast<int>( WP::WeaponCount );

	int index = scast<int>( pGun->GetKind() );
	const int oldIndex = index;

	// Shift the kind to desired sign until available kind.
	// The loop is safe because the wrap-around process will makes also the zero(WeaponKind::Buster), and the zero is certainly available.
	do
	{
		index += shiftSign;
		// Loop the index
		while ( index <  0			) { index += kindCount; }
		while ( index >= kindCount	) { index -= kindCount; }
	}
	while ( !availableWeapon.IsAvailable( scast<WP>( index ) ) );

	if ( index != oldIndex )
	{
		AssignGunByKind( scast<WP>( index ) );
		Donya::Sound::Play( Music::Player_ShiftGun );
	}
}
void Player::GenerateSlideEffects() const
{
	Effect::Handle handle = Effect::Handle::Generate( Effect::Kind::Player_Slide_Begin, GetPosition() );
	handle.SetRotation( 0.0f, ToRadian( 90.0f ) * lookingSign, 0.0f );
	Effect::Admin::Get().AddCopy( handle ); // Leave management of the effect instance to admin
	
	Donya::Sound::Play( Music::Player_Dash );
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
	ImGui::Text			( u8"現在の銃口：%s",					pGun->GetGunName().c_str() );
	ImGui::Text			( u8"現在の重力：%5.3f", nowGravity );
	availableWeapon.ShowImGuiNode( u8"現在使用可能な武器" );

	inputManager.ShowImGuiNode( u8"入力状態" );

	ImGui::DragFloat3	( u8"ワールド座標",					&body.pos.x,	0.01f );
	ImGui::DragFloat3	( u8"速度",							&velocity.x,	0.01f );

	Donya::Vector3 front = orientation.LocalFront();
	ImGui::SliderFloat3	( u8"前方向",						&front.x,		-1.0f, 1.0f );
	orientation = Donya::Quaternion::LookAt( Donya::Vector3::Front(), front.Unit(), Donya::Quaternion::Freeze::Up );
	ImGui::SliderFloat	( u8"向いているＸ方向",				&lookingSign,	-1.0f, 1.0f );

	ImGui::Text			( u8"%d: チャージレベル",				scast<int>( shotManager.ChargeLevel() ) );
	ImGui::Text			( u8"%04.2f: ショット長押し秒数",		shotManager.ChargeSecond() );
	ImGui::Checkbox		( u8"地上にいるか",					&onGround );
	ImGui::Checkbox		( u8"慣性ジャンプ中か",				&wasJumpedWhileSlide );

	bool tmp{};
	tmp = pMover->NowKnockBacking( *this );
	ImGui::Checkbox		( u8"のけぞり中か",					&tmp );
	tmp = invincibleTimer.NowWorking();
	ImGui::Checkbox		( u8"無敵中か",						&tmp );

	if ( ImGui::Button( u8"登場演出再生" ) )
	{
		AssignMover<Appear>();
	}
	if ( ImGui::Button( u8"退場させる" ) )
	{
		AssignMover<Leave>();
	}
	if ( ImGui::Button( u8"ガッツポーズ再生" ) )
	{
		AssignMover<WinningPose>();
	}

	ImGui::TreePop();
}
#endif // USE_IMGUI
