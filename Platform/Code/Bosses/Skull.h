#pragma once

#include <array>
#include <memory>

#include "../Boss.h"
#include "../Bullet.h"	// Use Bullet::FireDesc
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
			virtual void Update( Skull &instance, float elapsedTime, const Input &input );
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
			void Update( Skull &instance, float elapsedTime, const Input &input ) override;
			void PhysicUpdate( Skull &instance, float elapsedTime, const std::vector<Donya::Collision::Box3F> &solids ) override;
			bool ShouldChangeMover( const Skull &instance ) const override;
			std::function<void()> GetChangeStateMethod( Skull &instance ) const override;
		#if USE_IMGUI
			std::string GetMoverName() const override;
		#endif // USE_IMGUI
		};
		class DetectTargetAction : public MoverBase
		{
		private: // HACK: This enumeration can be removed and replace to Skull::Behavior(but that definition must be move to top)
			enum class Destination
			{
				None,
				Shot,
				Jump
			};
			Destination nextState = Destination::None;
		public:
			void Init( Skull &instance ) override;
			void Update( Skull &instance, float elapsedTime, const Input &input ) override;
			bool ShouldChangeMover( const Skull &instance ) const override;
			std::function<void()> GetChangeStateMethod( Skull &instance ) const override;
		#if USE_IMGUI
			std::string GetMoverName() const override;
		#endif // USE_IMGUI
		private:
			bool IsContinuingSameAction( const Skull &inst ) const;
		};
		class Shot : public MoverBase
		{
		private:
			float	timer		= 0.0f;
			float	interval	= 0.0f;
			int		fireCount	= 0;
			bool	wasFinished	= false;
		public:
			void Init( Skull &instance ) override;
			void Update( Skull &instance, float elapsedTime, const Input &input ) override;
			bool ShouldChangeMover( const Skull &instance ) const override;
			std::function<void()> GetChangeStateMethod( Skull &instance ) const override;
		#if USE_IMGUI
			std::string GetMoverName() const override;
		#endif // USE_IMGUI
		private:
			void Fire( Skull &instance, float elapsedTime, const Input &input ) const;
		};
		class Jump : public MoverBase
		{
		private:
			bool wasLanding = false;
		public:
			void Init( Skull &instance ) override;
			void Update( Skull &instance, float elapsedTime, const Input &input ) override;
			void PhysicUpdate( Skull &instance, float elapsedTime, const std::vector<Donya::Collision::Box3F> &solids ) override;
			bool ShouldChangeMover( const Skull &instance ) const override;
			std::function<void()> GetChangeStateMethod( Skull &instance ) const override;
		#if USE_IMGUI
			std::string GetMoverName() const override;
		#endif // USE_IMGUI
		};
		class Shield : public MoverBase
		{
		private:
			float	timer			= 0.0f;
			float	protectSecond	= 1.0f;
			bool	nowProtected	= false;
		public:
			void Init( Skull &instance ) override;
			void Update( Skull &instance, float elapsedTime, const Input &input ) override;
			bool ShouldChangeMover( const Skull &instance ) const override;
			std::function<void()> GetChangeStateMethod( Skull &instance ) const override;
		#if USE_IMGUI
			std::string GetMoverName() const override;
		#endif // USE_IMGUI
		};
		class Run : public MoverBase
		{
		private:
			float	timer		= 0.0f;
			bool	wasArrived	= false;
			Donya::Vector3 initialPos;
		public:
			void Init( Skull &instance ) override;
			void Update( Skull &instance, float elapsedTime, const Input &input ) override;
			void PhysicUpdate( Skull &instance, float elapsedTime, const std::vector<Donya::Collision::Box3F> &solids ) override;
			bool ShouldChangeMover( const Skull &instance ) const override;
			std::function<void()> GetChangeStateMethod( Skull &instance ) const override;
		#if USE_IMGUI
			std::string GetMoverName() const override;
		#endif // USE_IMGUI
		private:
			int GetCurrentDirectionSign( const Skull &instance ) const;
		};
	private:
		enum class Behavior
		{
			None,
			Shot,
			Jump
		};
	private:
		Input						previousInput;
		std::array<Behavior, 2>		previousBehaviors{ Behavior::None }; // Contains: [0:One previous], [1:Two previous]
		Donya::Vector3				aimingPos;		// Used for store the target pos of some timing
		std::unique_ptr<MoverBase>	pMover = nullptr;
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
		void Update( float elapsedTime, const Input &input ) override;
		void PhysicUpdate( float elapsedTime, const std::vector<Donya::Collision::Box3F> &solids ) override;
	public:
		float				GetGravity()		const override;
		Kind				GetKind()			const override;
		Definition::Damage	GetTouchDamage()	const override;
	private:
		int  GetInitialHP() const override;
		void AssignMyBody( const Donya::Vector3 &wsPos ) override;
		void RegisterPreviousBehavior( Behavior behavior );
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
		int					hp						= 28;
		float				gravity					= 1.0f;
		
		float				shotBeginLagSecond		= 1.0f;
		float				shotFireIntervalSecond	= 1.0f;
		float				shotEndLagSecond		= 1.0f;
		float				shotDegreeIncrement		= 10.0f;
		int					shotFireCount			= 3;
		Bullet::FireDesc	shotDesc;

		float				jumpDegree				= 60.0f; // 0.0f <= degree <= 90.0f
		float				jumpVerticalHeight		= 1.0f;
		std::vector<float>	jumpDestLengths;

		struct RandomElement
		{
			int		bias	= 1;	// greater than zero
			float	second	= 1.0f;
		private:
			friend class cereal::access;
			template<class Archive>
			void serialize( Archive &archive, std::uint32_t version )
			{
				archive
				(
					CEREAL_NVP( bias	),
					CEREAL_NVP( second	)
				);

				if ( 1 <= version )
				{
					// archive( CEREAL_NVP( x ) );
				}
			}
		};
		std::vector<RandomElement> shieldProtectSeconds;
		float				shieldBeginLagSecond	= 1.0f;
		float				shieldEndLagSecond		= 1.0f;
		
		float				runSpeed				= 1.0f;
		float				runDestTakeDist			= 1.0f; // Destination = target-pos - runDestTakeDist
		float				runEndLagSecond			= 1.0f;
		
		Definition::Damage	touchDamage;
		Donya::Vector3		hitBoxOffset			{ 0.0f, 0.0f, 0.0f };
		Donya::Vector3		hurtBoxOffset			{ 0.0f, 0.0f, 0.0f };
		Donya::Vector3		hitBoxSize				{ 1.0f, 1.0f, 1.0f };
		Donya::Vector3		hurtBoxSize				{ 1.0f, 1.0f, 1.0f };
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
					CEREAL_NVP( gravity		),
					CEREAL_NVP( runSpeed	)
				);
			}
			if ( 2 <= version )
			{
				archive
				(
					CEREAL_NVP( jumpDegree		),
					CEREAL_NVP( jumpDestLengths	)
				);
			}
			if ( 3 <= version )
			{
				archive( CEREAL_NVP( jumpVerticalHeight ) );
			}
			if ( 4 <= version )
			{
				archive
				(
					CEREAL_NVP( shotBeginLagSecond		),
					CEREAL_NVP( shotFireIntervalSecond	),
					CEREAL_NVP( shotEndLagSecond		),
					CEREAL_NVP( shotFireCount			),
					CEREAL_NVP( shotDesc				),
					CEREAL_NVP( shieldProtectSeconds	),
					CEREAL_NVP( shieldBeginLagSecond	),
					CEREAL_NVP( shieldEndLagSecond		),
					CEREAL_NVP( runDestTakeDist			),
					CEREAL_NVP( runEndLagSecond			)
				);
			}
			if ( 5 <= version )
			{
				archive ( CEREAL_NVP( shotDegreeIncrement ) );
			}
			if ( 6 <= version )
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
CEREAL_CLASS_VERSION( Boss::SkullParam, 5 )
