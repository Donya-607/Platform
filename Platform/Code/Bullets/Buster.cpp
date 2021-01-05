#include "Buster.h"

#include "../Donya/Sound.h"

#include "../Effect/EffectAdmin.h"
#include "../Effect/EffectKind.h"
#include "../Music.h"
#include "../Parameter.h"
#include "../PointLightStorage.h"

namespace Bullet
{
	namespace Parameter
	{
		static ParamOperator<BusterParam> busterParam{ "Buster", "Bullet/" };

		const BusterParam &GetBuster()
		{
			return busterParam.Get();
		}

		namespace Impl
		{
			void LoadBuster()
			{
				busterParam.LoadParameter();
			}
		#if USE_IMGUI
			void UpdateBuster( const std::string &nodeCaption )
			{
				busterParam.ShowImGuiNode( nodeCaption );
			}
		#endif // USE_IMGUI
		}
	}

	namespace
	{
		constexpr size_t chargeLevelCount = scast<size_t>( Player::ShotLevel::LevelCount );
		constexpr const char *GetChargeLevelName( Player::ShotLevel level )
		{
			switch ( level )
			{
			case Player::ShotLevel::Normal:		return "0_Normal";
			case Player::ShotLevel::Tough:		return "1_Tough"; 
			case Player::ShotLevel::Strong:		return "2_Strong";
			default: break;
			}

			return "ERROR_LEVEL";
		}

		const BusterParam::PerLevel *GetParamLevelOrNullptr( Player::ShotLevel level )
		{
			const size_t index = scast<size_t>( level );
			if ( chargeLevelCount <= index )
			{
				_ASSERT_EXPR( 0, L"Error: Passed level is out of range!" );
				return nullptr;
			}
			// else

			const auto &data = Parameter::GetBuster();
			return &data.params[index];
		}
	}

	int  Buster::livingCount = 0;
	int  Buster::GetLivingCount()
	{
		return livingCount;
	}
	void Buster::Init( const FireDesc &parameter )
	{
		livingCount++;

		using Level = Player::ShotLevel;

		if ( parameter.pAdditionalDamage )
		{
			// The Player attaches the charged damage to "pAdditionalDamage->amount"
			// by casting from Player::ShotLevel(Level) to int,
			// so it process is inverse conversion.

			const int currentLevel = std::min
			(
				scast<int>( chargeLevelCount - 1 ),
				std::max( 0, parameter.pAdditionalDamage->amount )
			);
			chargeLevel = scast<Level>( currentLevel );
		}
		else
		{
			chargeLevel = Level::Normal;
		}

		FireDesc adjusted  = parameter;

		const auto *pLevel = GetParamLevelOrNullptr( chargeLevel );
		if ( pLevel )
		{
			adjusted.initialSpeed *= pLevel->accelRate;
		}

		Base::Init( adjusted );
	}
	void Buster::Uninit()
	{
		livingCount--;
	}
	void Buster::Update( float elapsedTime, const Donya::Collision::Box3F &wsScreenHitBox )
	{
		Base::Update( elapsedTime, wsScreenHitBox );

		const auto *pLevel = GetParamLevelOrNullptr( chargeLevel );
		const float animePlaySpeed = ( pLevel ) ? pLevel->basic.animePlaySpeed : 1.0f;
		UpdateMotionIfCan( elapsedTime * animePlaySpeed, scast<int>( chargeLevel ) );

		if ( pLevel )
		{
			auto lightSource = pLevel->lightSource;
			lightSource.wsPos += GetPosition();
			PointLightStorage::Get().RegisterIfThereSpace( lightSource );
		}

		if ( Player::IsFullyCharged( chargeLevel ) )
		{
			generationTimer += elapsedTime;

			const float &generateInterval = Parameter::GetBuster().chargedTracingInterval;
			if ( generateInterval <= generationTimer )
			{
				generationTimer = 0.0f;

				Effect::Handle handle = Effect::Handle::Generate( Effect::Kind::ChargedBustersTracing, GetPosition() );
				handle.SetRotation( orientation );
				Effect::Admin::Get().AddCopy( handle ); // Leave management of the effect instance to admin
			}
		}
	}
	Kind Buster::GetKind() const
	{
		return Kind::Buster;
	}
	void Buster::GenerateCollidedEffect() const
	{
		const auto kind = ( Player::IsFullyCharged( chargeLevel ) ) ? Effect::Kind::Hit_ChargedBuster : Effect::Kind::Hit_Buster;
		Effect::Admin::Get().GenerateInstance( kind, GetPosition() );
	}
	void Buster::PlayCollidedSE() const
	{
		Donya::Sound::Play( Music::Bullet_HitBuster );
	}
	Definition::Damage Buster::GetDamageParameter() const
	{
		const auto *pLevel = GetParamLevelOrNullptr( chargeLevel );
		if ( !pLevel )
		{
			Definition::Damage empty{};
			empty.amount	= 0;
			empty.type		= Definition::Damage::Type::None;
			return empty;
		}
		// else

		return pLevel->basic.damage;
	}
	void Buster::AssignBodyParameter( const Donya::Vector3 &wsPos )
	{
		const auto *pLevel = GetParamLevelOrNullptr( chargeLevel );
		body.pos	= wsPos;
		body.offset	= ( pLevel ) ? orientation.RotateVector( pLevel->basic.hitBoxOffset ) : Donya::Vector3::Zero();
		body.size	= ( pLevel ) ? pLevel->basic.hitBoxSize : Donya::Vector3::Zero();
	}
#if USE_IMGUI
	void Buster::ShowImGuiNode( const std::string &nodeCaption )
	{
		Base::ShowImGuiNode( nodeCaption );

		/*
		if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
		// else

		Base::ShowImGuiNode( u8"基底部分" );

		if ( ImGui::TreeNode( u8"派生部分" ) )
		{
			ImGui::TreePop();
		}

		ImGui::TreePop();
		*/
	}
	void BusterParam::ShowImGuiNode()
	{
		ImGui::DragFloat( u8"チャージ弾の残像の生成間隔（秒）", &chargedTracingInterval, 0.01f );
		chargedTracingInterval = std::max( 0.0f, chargedTracingInterval );

		if ( params.size() != chargeLevelCount )
		{
			params.resize( chargeLevelCount );
		}
		using Level = Player::ShotLevel;
		for ( size_t i = 0; i < chargeLevelCount; ++i )
		{
			if ( !ImGui::TreeNode( GetChargeLevelName( scast<Level>( i ) ) ) ) { continue; }
			// else

			auto &elem = params[i];
			ImGui::Text( u8"（ダメージ量は生成時に +%d されます）", i );
			elem.basic.ShowImGuiNode( u8"汎用設定" );

			ImGui::Helper::ShowPointLightNode( u8"光源設定", &elem.lightSource );

			ImGui::DragFloat( u8"速度倍率", &elem.accelRate, 0.01f );
			ImGui::Text( u8"（生成時の速度に掛け算されます）" );

			ImGui::TreePop();
		}
	}
#endif // USE_IMGUI
}
