#pragma once

#include <memory>

#include "../Boss.h"
#include "../Damage.h"

namespace Boss
{
	class Skull : public Base
	{
	private:
		class MoverBase
		{
		public:
			virtual ~MoverBase() = default;
		public:
			virtual void Init( Skull &instance );
			virtual void Uninit( Skull &instance );
			virtual void Update( Skull &instance, float elapsedTime, const Donya::Vector3 &wsTargetPos );
			virtual void PhysicUpdate( Skull &instance, float elapsedTime, const std::vector<Donya::Collision::Box3F> &solids );
			virtual bool ShouldChangeMover( const Skull &instance ) const = 0;
			virtual std::function<void()> GetChangeStateMethod( Skull &instance ) const = 0;
		#if USE_IMGUI
			virtual std::string GetMoverName() const = 0;
		#endif // USE_IMGUI
		};
		class AppearPerformance : public MoverBase
		{
		private:
			bool wasLanding = false;
		public:
			void Init( Skull &instance ) override;
			void Update( Skull &instance, float elapsedTime, const Donya::Vector3 &wsTargetPos ) override;
			void PhysicUpdate( Skull &instance, float elapsedTime, const std::vector<Donya::Collision::Box3F> &solids ) override;
			bool ShouldChangeMover( const Skull &instance ) const override;
			std::function<void()> GetChangeStateMethod( Skull &instance ) const override;
		#if USE_IMGUI
			std::string GetMoverName() const override;
		#endif // USE_IMGUI
		};
		class DetectTargetAction : public MoverBase
		{
		public:
			void Init( Skull &instance );
			void Update( Skull &instance, float elapsedTime, const Donya::Vector3 &wsTargetPos );
			bool ShouldChangeMover( const Skull &instance ) const;
			std::function<void()> GetChangeStateMethod( Skull &instance ) const;
		#if USE_IMGUI
			std::string GetMoverName() const override;
		#endif // USE_IMGUI
		};
		class Shot : public MoverBase
		{
		public:
			void Init( Skull &instance );
			void Update( Skull &instance, float elapsedTime, const Donya::Vector3 &wsTargetPos );
			bool ShouldChangeMover( const Skull &instance ) const;
			std::function<void()> GetChangeStateMethod( Skull &instance ) const;
		#if USE_IMGUI
			std::string GetMoverName() const override;
		#endif // USE_IMGUI
		};
		class Jump : public MoverBase
		{
		public:
			void Init( Skull &instance );
			void Update( Skull &instance, float elapsedTime, const Donya::Vector3 &wsTargetPos );
			bool ShouldChangeMover( const Skull &instance ) const;
			std::function<void()> GetChangeStateMethod( Skull &instance ) const;
		#if USE_IMGUI
			std::string GetMoverName() const override;
		#endif // USE_IMGUI
		};
		class Shield : public MoverBase
		{
		public:
			void Init( Skull &instance );
			void Update( Skull &instance, float elapsedTime, const Donya::Vector3 &wsTargetPos );
			bool ShouldChangeMover( const Skull &instance ) const;
			std::function<void()> GetChangeStateMethod( Skull &instance ) const;
		#if USE_IMGUI
			std::string GetMoverName() const override;
		#endif // USE_IMGUI
		};
		class Run : public MoverBase
		{
		public:
			void Init( Skull &instance );
			void Update( Skull &instance, float elapsedTime, const Donya::Vector3 &wsTargetPos );
			bool ShouldChangeMover( const Skull &instance ) const;
			std::function<void()> GetChangeStateMethod( Skull &instance ) const;
		#if USE_IMGUI
			std::string GetMoverName() const override;
		#endif // USE_IMGUI
		};
	private:
		std::unique_ptr<MoverBase> pMover = nullptr;
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
		void Init( const InitializeParam &parameter, int roomID, const Donya::Collision::Box3F &wsRoomArea ) override;
		void Update( float elapsedTime, const Donya::Vector3 &wsTargetPos ) override;
		void PhysicUpdate( float elapsedTime, const std::vector<Donya::Collision::Box3F> &solids ) override;
	public:
		float				GetGravity()		const override;
		Kind				GetKind()			const override;
		Definition::Damage	GetTouchDamage()	const override;
	private:
		int  GetInitialHP() const override;
		void AssignMyBody( const Donya::Vector3 &wsPos ) override;
	private:
		template<class Mover>
		void AssignMover()
		{
			if ( pMover )
			{
				pMover->Uninit( *this );
				pMover.reset();
			}

			pMover = std::make_unique<Mover>();
			pMover->Init( *this );
		}
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
