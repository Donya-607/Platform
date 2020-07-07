#pragma once

#include "../Boss.h"
#include "../Damage.h"

namespace Boss
{
	class Skull : public Base
	{
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive
			(
				cereal::base_class<Base>( this )
			);
			if ( 1 <= version )
			{
				// archive();
			}
		}
	public:
		void Update( float elapsedTime, const Donya::Vector3 &wsTargetPos ) override;
	public:
		float				GetGravity()		const override;
		Kind				GetKind()			const override;
		Definition::Damage	GetTouchDamage()	const override;
	private:
		void DieMoment() override;
		void TransitionState( State nextState ) override;
		void UpdateState( float elapsedTime, const Donya::Vector3 &wsTargetPos );
	private:
		int  GetInitialHP() const override;
		void AssignMyBody( const Donya::Vector3 &wsPos ) override;
	private:
		void NormalInit();
		void NormalUpdate( float elapsedTime, const Donya::Vector3 &wsTargetPos );
	public:
	#if USE_IMGUI
		/// <summary>
		/// Returns the return value of ImGui::TreeNode().
		/// </summary>
		bool ShowImGuiNode( const std::string &nodeCaption ) override;
	#endif // USE_IMGUI
	};

	struct SkullParam
	{
	public:
		int					hp				= 28;
		float				gravity			= 1.0f;
		float				jumpHeight		= 1.0f;
		float				jumpTakeSeconds	= 1.0f;
		float				runSpeed		= 1.0f;
		Definition::Damage	touchDamage;
		Donya::Vector3		hitBoxOffset	{ 0.0f, 0.0f, 0.0f };
		Donya::Vector3		hurtBoxOffset	{ 0.0f, 0.0f, 0.0f };
		Donya::Vector3		hitBoxSize		{ 1.0f, 1.0f, 1.0f };
		Donya::Vector3		hurtBoxSize		{ 1.0f, 1.0f, 1.0f };
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive
			(
				CEREAL_NVP( hp				),
				CEREAL_NVP( touchDamage		),
				CEREAL_NVP( hitBoxOffset	),
				CEREAL_NVP( hurtBoxOffset	),
				CEREAL_NVP( hitBoxSize		),
				CEREAL_NVP( hurtBoxSize		)
			);

			if ( 1 <= version )
			{
				archive
				(
					CEREAL_NVP( gravity			),
					CEREAL_NVP( jumpHeight		),
					CEREAL_NVP( jumpTakeSeconds	),
					CEREAL_NVP( runSpeed		)
				);
			}
			if ( 2 <= version )
			{
				// archive( CEREAL_NVP( x ) );
			}
		}
	public:
	#if USE_IMGUI
		void ShowImGuiNode();
	#endif // USE_IMGUI
	};
}
CEREAL_CLASS_VERSION( Boss::Skull, 0 )
CEREAL_REGISTER_TYPE( Boss::Skull )
CEREAL_REGISTER_POLYMORPHIC_RELATION( Boss::Base, Boss::Skull )
CEREAL_CLASS_VERSION( Boss::SkullParam, 1 )
