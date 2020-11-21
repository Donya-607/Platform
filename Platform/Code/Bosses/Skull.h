#pragma once

#include <array>
#include <memory>

#include "../Boss.h"
#include "../Bullet.h"	// Use Bullet::FireDesc and Shield
#include "../Damage.h"

namespace Boss
{
	class Skull : public Base
	{
	public:
		enum class MotionKind
		{
			Idle = 0,
			Shot_Ready,
			Shot_Recoil,
			Shot_End,
			Jump,
			Shield_Ready,
			Shield_Expand,
			Run,
			Appear_Begin,
			Appear_End,

			MotionCount
		};
	private:
		class MotionManager
		{
		private:
			MotionKind prevKind = MotionKind::Jump;
			MotionKind currKind = MotionKind::Jump;
		public:
			void Init( Skull &instance );
			void Update( Skull &instance, float elapsedTime );
		public:
			void ChangeMotion( Skull &instance, MotionKind nextMotionKind, bool resetTimerIfSameMotion = false );
			bool WasCurrentMotionEnded( const Skull &instance ) const;
			MotionKind CurrentMotionKind() const { return currKind; }
		private:
			int  ToMotionIndex( MotionKind kind ) const;
			bool AssignPose( Skull &instance, MotionKind kind );
			bool ShouldEnableLoop( MotionKind kind ) const;
		};
		class MoverBase
		{
		public:
			virtual ~MoverBase() = default;
		public:
			virtual void Init( Skull &instance );
			virtual void Uninit( Skull &instance );
			virtual void Update( Skull &instance, float elapsedTime, const Input &input );
			virtual void PhysicUpdate( Skull &instance, float elapsedTime, const Map &terrain );
			virtual bool NowAppearing( const Skull &instance ) const { return false; }
			virtual bool NowRecoverHPTiming( const Skull &instance ) const { return false; }
			virtual bool ShouldChangeMover( const Skull &instance ) const = 0;
			virtual std::function<void()> GetChangeStateMethod( Skull &instance ) const = 0;
		#if USE_IMGUI
			virtual std::string GetMoverName() const = 0;
		#endif // USE_IMGUI
		};
		class AppearPerformance : public MoverBase
		{
		private:
			float	oldTimeAfterLanding		= 0.0f;
			float	elapsedTimeAfterLanding	= 0.0f;
			bool	wasLanding				= false;
		public:
			void Init( Skull &instance ) override;
			void Update( Skull &instance, float elapsedTime, const Input &input ) override;
			void PhysicUpdate( Skull &instance, float elapsedTime, const Map &terrain ) override;
			bool NowAppearing( const Skull &instance ) const override;
			bool NowRecoverHPTiming( const Skull &instance ) const override;
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
		public:
			void Init( Skull &instance ) override;
			void Update( Skull &instance, float elapsedTime, const Input &input ) override;
			void PhysicUpdate( Skull &instance, float elapsedTime, const Map &terrain ) override;
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
		private:
			Donya::Vector3 CalcCurrentShieldPosition( const Skull &instance ) const;
			void GenerateShield( const Skull &instance, float lifeTimeSecond ) const;
		};
		class Run : public MoverBase
		{
		private:
			float	runTimer	= 0.0f;
			float	waitTimer	= 0.0f;
			float	arrivalTime	= 1.0f;
			bool	wasArrived	= false;
			Donya::Vector3 initialPos;
		public:
			void Init( Skull &instance ) override;
			void Update( Skull &instance, float elapsedTime, const Input &input ) override;
			void PhysicUpdate( Skull &instance, float elapsedTime, const Map &terrain ) override;
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
		Input							previousInput;
		std::array<Behavior, 2>			previousBehaviors{ Behavior::None }; // Contains: [0:One previous], [1:Two previous]
		Donya::Vector3					aimingPos;		// Used for store the target pos of some timing
		MotionManager					motionManager;
		std::unique_ptr<MoverBase>		pMover  = nullptr;
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
		void Init( const InitializeParam &parameter, int roomID, bool withAppearPerformance, const Donya::Collision::Box3F &wsRoomArea ) override;
		void Update( float elapsedTime, const Input &input ) override;
		void PhysicUpdate( float elapsedTime, const Map &terrain ) override;
		void Draw( RenderingHelper *pRenderer ) const override;
		void DrawHitBox( RenderingHelper *pRenderer, const Donya::Vector4x4 &matVP ) const override;
	public:
		bool				NowAppearing()			const override;
		bool				NowRecoverHPTiming()	const override;
		float				GetGravity()			const override;
		float				GetInvincibleSecond()	const override;
		float				GetInvincibleInterval()	const override;
		Kind				GetKind()				const override;
		Definition::Damage	GetTouchDamage()		const override;
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
	private:
		void Fall( float elapsedTime );
		void LookingToTarget( const Donya::Vector3 &wsTargetPos );
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
		float				invincibleSecond		= 0.5f;
		float				invincibleFlushInterval = 0.1f;	// Second
		
		float				appearRecoverHPTiming	= 1.0f;	// Second
		float				appearRoarTiming		= 1.0f;	// Second
		float				appearWaitMotionSec		= 1.0f;

		float				shotBeginLagSecond		= 1.0f;
		float				shotFireIntervalSecond	= 1.0f;
		float				shotEndLagSecond		= 1.0f;
		float				shotDegreeIncrement		= 10.0f;
		Donya::Vector2		shotDegreeLimit			{ 0.0f, 90.0f };	// In right side case. [X:Min][Y:Max] [0.0f:RIGHT][90.0f:UP]
		int					shotFireCount			= 3;
		Bullet::FireDesc	shotDesc;

		float				jumpDegree				= 60.0f;			// 0.0f <= degree <= 90.0f
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
		Donya::Vector3		shieldPosOffset			{ 0.0f, 0.0f, 0.0f };
		
		float				runSpeed				= 1.0f;
		float				runDestTakeDist			= 1.0f;				// Destination = target-pos - runDestTakeDist
		float				runEndLagSecond			= 1.0f;
		
		Definition::Damage	touchDamage;
		Donya::Vector3		hitBoxOffset			{ 0.0f, 0.0f, 0.0f };
		Donya::Vector3		hurtBoxOffset			{ 0.0f, 0.0f, 0.0f };
		Donya::Vector3		hitBoxSize				{ 1.0f, 1.0f, 1.0f };
		Donya::Vector3		hurtBoxSize				{ 1.0f, 1.0f, 1.0f };

		std::vector<float>	animePlaySpeeds;							// It size() == Skull::MotionKind::MotionCount
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
				archive( CEREAL_NVP( animePlaySpeeds ) );
			}
			if ( 7 <= version )
			{
				archive( CEREAL_NVP( shieldPosOffset ) );
			}
			if ( 8 <= version )
			{
				archive( CEREAL_NVP( shotDegreeLimit ) );
			}
			if ( 9 <= version )
			{
				archive
				(
					CEREAL_NVP( invincibleSecond		),
					CEREAL_NVP( invincibleFlushInterval	)
				);
			}
			if ( 10 <= version )
			{
				archive
				(
					CEREAL_NVP( appearRecoverHPTiming	),
					CEREAL_NVP( appearRoarTiming		),
					CEREAL_NVP( appearWaitMotionSec		)
				);
			}
			if ( 11 <= version )
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
CEREAL_CLASS_VERSION( Boss::SkullParam, 10 )
