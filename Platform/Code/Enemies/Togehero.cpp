#include "Togehero.h"

#include "../Bullet.h"
#include "../Parameter.h"

namespace Enemy
{
	namespace Parameter
	{
		static ParamOperator<TogeheroParam> togeheroParam{ "Togehero", "Enemy/" };
		const TogeheroParam &GetTogehero()
		{
			return togeheroParam.Get();
		}

		namespace Impl
		{
			void LoadTogehero()
			{
				togeheroParam.LoadParameter();
			}
		#if USE_IMGUI
			void UpdateTogehero( const std::string &nodeCaption )
			{
				togeheroParam.ShowImGuiNode( nodeCaption );
			}
		#endif // USE_IMGUI
		}
	}

	void Togehero::Init( const InitializeParam &parameter, const Donya::Vector3 &wsTargetPos, const Donya::Collision::Box3F &wsScreenHitBox )
	{
		Base::Init( parameter, wsTargetPos, wsScreenHitBox );
		
		prevIncludeSecond = 0.0f;
		currIncludeSecond = 0.0f;

		// Unused memebrs
		body.exist		= false;
		hurtBox.exist	= false;
		orientation		= Donya::Quaternion::Identity();
	}
	void Togehero::Update( float elapsedTime, const Donya::Vector3 &wsTargetPos, const Donya::Collision::Box3F &wsScreen )
	{
	#if USE_IMGUI
		// Apply for be able to see an adjustment immediately
		{
			AssignMyBody( body.pos );
		}
	#endif // USE_IMGUI

		const auto &data = Parameter::GetTogehero();

		prevIncludeSecond = currIncludeSecond;
		if ( Donya::Collision::IsHit( body, wsTargetPos ) )
		{ currIncludeSecond += elapsedTime; }
		else
		{ currIncludeSecond = 0.0f; }

		const bool beginningToInclude	= ( 0.0f < currIncludeSecond && prevIncludeSecond <= 0.0f );
		const bool includesLongTime		= ( data.generateInterval <= currIncludeSecond );
		if ( beginningToInclude || includesLongTime )
		{
			Generate( wsTargetPos );
			if ( includesLongTime )
			{
				const float rem = fmodf( currIncludeSecond, data.generateInterval );
				currIncludeSecond = ( IsZero( rem ) ) ? 0.0001f : rem;
			}
		}
	}
	void Togehero::PhysicUpdate( float elapsedTime, const Map &terrain, bool considerBodyExistence )
	{
		// No op
	}
	void Togehero::Draw( RenderingHelper *pRenderer ) const
	{
		// No op
	}
	void Togehero::DrawHitBox( RenderingHelper *pRenderer, const Donya::Vector4x4 &matVP ) const
	{
		if ( !pRenderer ) { return; }
		// else

	#if DEBUG_MODE
		constexpr Donya::Vector4 color = Donya::Vector4{ 0.0f, 0.3137f, 0.7529f, 0.6f };

		const Donya::Vector4x4 matW = MakeWorldMatrix
		(
			body.size * 2.0f,
			/* enableRotation = */ false,
			body.WorldPosition()
		);

		Donya::Model::Cube::Constant constant;
		constant.matWorld		= matW;
		constant.matViewProj	= matVP;
		constant.drawColor		= color;
		constant.lightDirection	= -Donya::Vector3::Up();

		pRenderer->ProcessDrawingCube( constant );
	#endif // DEBUG_MODE
	}
	Kind Togehero::GetKind() const { return Kind::Togehero; }
	Definition::Damage Togehero::GetTouchDamage() const
	{
		return Parameter::GetTogehero().basic.touchDamage;
	}
	int  Togehero::GetInitialHP() const
	{
		return Parameter::GetTogehero().basic.hp;
	}
	void Togehero::AssignMyBody( const Donya::Vector3 &wsPos )
	{
		const auto &data = Parameter::GetTogehero().basic;
		body.pos		= wsPos;
		body.offset		= data.hitBoxOffset;
		body.size		= data.hitBoxSize;
		hurtBox.pos		= wsPos;
		hurtBox.offset	= data.hurtBoxOffset;
		hurtBox.size	= data.hurtBoxSize;

		body.exist		= false;
		hurtBox.exist	= false;
	}
	void Togehero::Generate( const Donya::Vector3 &wsTargetPos ) const
	{
		const auto &data = Parameter::GetTogehero();

		const Donya::Vector3 offset{ data.generatePosOffsetH, 0.0f, 0.0f };

		Bullet::FireDesc desc;
		desc.kind			= Bullet::Kind::TogeheroBody;
		desc.initialSpeed	= data.bodySpeed;
		desc.position		= wsTargetPos + offset;
		desc.direction		= ( wsTargetPos - desc.position ).Unit();
		desc.owner			= hurtBox.id;

		Bullet::Admin::Get().RequestFire( desc );
	}
#if USE_IMGUI
	bool Togehero::ShowImGuiNode( const std::string &nodeCaption )
	{
		if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return false; }
		// else

		Base::ShowImGuiNode( u8"基底部分" );

		if ( ImGui::TreeNode( u8"派生部分" ) )
		{
			ImGui::DragFloat( u8"生成間隔タイマ", &currIncludeSecond, 0.01f );

			ImGui::TreePop();
		}

		ImGui::TreePop();
		return true;
	}
	void TogeheroParam::ShowImGuiNode()
	{
		ImGui::Text( u8"当たり判定の設定のみ有効です" );
		const auto oldBasic = basic;
		basic.ShowImGuiNode( u8"汎用設定" );
		{
			basic.hp = INT_MAX;
			basic.touchDamage.amount	= 0;
			basic.touchDamage.type		= Definition::Damage::Type::None;

			// To be same the hitBox and hurtBox parameters
			
			if ( basic.hurtBoxOffset != oldBasic.hurtBoxOffset )
			{ basic.hitBoxOffset = basic.hurtBoxOffset; }
			else
			{ basic.hurtBoxOffset = basic.hitBoxOffset; }

			if ( basic.hurtBoxSize != oldBasic.hurtBoxSize )
			{ basic.hitBoxSize = basic.hurtBoxSize; }
			else
			{ basic.hurtBoxSize = basic.hitBoxSize; }
		}


		ImGui::DragFloat( u8"生成間隔（秒）",			&generateInterval,		0.01f	);
		ImGui::DragFloat( u8"生成位置のオフセット",	&generatePosOffsetH,	0.01f	);		
		ImGui::DragFloat( u8"実体の移動速度",			&bodySpeed,				0.01f	);
		bodySpeed			= std::max( 0.01f, bodySpeed			);
		generateInterval	= std::max( 0.01f, generateInterval		);
		generatePosOffsetH	= std::max( 0.01f, generatePosOffsetH	);
	}
#endif // USE_IMGUI
}
