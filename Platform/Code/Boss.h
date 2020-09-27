#pragma once

#include <memory>
#include <string>
#include <vector>

#undef max
#undef min

#include <cereal/types/memory.hpp>
#include <cereal/types/polymorphic.hpp>
#include <cereal/types/vector.hpp>

#include "Donya/Collision.h"
#include "Donya/ModelPose.h"
#include "Donya/ModelMotion.h"
#include "Donya/UseImGui.h"		// Use USE_IMGUI macro
#include "Donya/Serializer.h"
#include "Donya/Vector.h"

#include "CSVLoader.h"
#include "Damage.h"
#include "Map.h"
#include "ModelHelper.h"
#include "ObjectBase.h"
#include "Room.h"				// Use Room::invalidID

namespace Boss
{
	enum class Kind
	{
		Skull,

		KindCount
	};

	struct SkullParam;
	namespace Parameter
	{
		void Load();

		const SkullParam &GetSkull();

	#if USE_IMGUI
		void Update( const std::string &nodeCaption );
	#endif // USE_IMGUI
		namespace Impl
		{
			void LoadSkull();
		#if USE_IMGUI
			void UpdateSkull( const std::string &nodeCaption );
		#endif // USE_IMGUI
		}
	}

	bool LoadResource();

	struct Input
	{
		Donya::Vector3 wsTargetPos;
		Donya::Vector2 controllerInputDirection;
		bool pressJump = false;
		bool pressShot = false;
	};

	struct InitializeParam
	{
		Donya::Vector3	wsPos;
		bool			lookingRight = true;
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive
			(
				CEREAL_NVP( wsPos			),
				CEREAL_NVP( lookingRight	)
			);
			if ( 1 <= version )
			{
				// archive();
			}
		}
	public:
	#if USE_IMGUI
		void ShowImGuiNode( const std::string &nodeCaption );
	#endif // USE_IMGUI
	};

	class Base : public Actor
	{
	protected:
		class Flusher
		{
		private:
			float workingSeconds	= 0.0f;
			float timer				= 0.0f;
		public:
			void Start( float flushingSeconds );
			void Update( float elapsedTime );
			bool Drawable( float flushingInterval ) const;
			/// <summary>
			/// It means now invincible.
			/// </summary>
			bool NowWorking() const;
		};
	protected: // Seralize values
		InitializeParam					initializer;
		int								roomID		= Room::invalidID;
		Donya::Collision::Box3F			roomArea;	// World space
	protected:
		ModelHelper::SkinningOperator	model;
		using					 Actor::body;		// VS a terrain
		Donya::Collision::Box3F			hurtBox;	// VS an attack
		using					 Actor::orientation;
		Donya::Vector3					velocity;
		Flusher							invincibleTimer;
		int								hp			= 1;	// Alive if this is greater than 0(if 0 < hp)
		bool							isDead		= false;
		bool							wantRemove	= false;
		bool							onGround	= false;
		mutable std::unique_ptr<Definition::Damage> pReceivedDamage	= nullptr; // Will be made at GiveDamage()
	public:
		Base() = default;
		virtual ~Base() = default;
		Base( const Base &  ) = delete;
		Base(       Base && ) = delete;
		Base &operator  = ( const Base &  ) = delete;
		Base &operator  = (       Base && ) = delete;
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive
			(
				cereal::base_class<Actor>( this ),
				CEREAL_NVP( initializer	),
				CEREAL_NVP( roomID		)
			);
			if ( 1 <= version )
			{
				// archive( CEREAL_NVP( x ) );
			}
		}
	public:
		virtual void Init( const InitializeParam &parameter, int roomID, const Donya::Collision::Box3F &wsRoomArea );
		virtual void Uninit();
		virtual void Update( float elapsedTime, const Input &input );
		virtual void PhysicUpdate( float elapsedTime, const Map &terrain );
		virtual void Draw( RenderingHelper *pRenderer ) const;
		virtual void DrawHitBox( RenderingHelper *pRenderer, const Donya::Vector4x4 &matVP ) const;
	public:
		virtual bool				NowDead()			const;
		virtual bool				ShouldRemove()		const;
		virtual int					GetCurrentHP()		const;
		virtual int					GetRoomID()			const;
		using				 Actor::GetHitBox;
		Donya::Collision::Box3F		GetHurtBox()		const;
		Donya::Vector3				GetVelocity()		const;
		/// <summary>
		/// Returns absolute value
		/// </summary>
		virtual float				GetGravity()		const = 0;
		virtual float				GetInvincibleSecond()	const = 0;
		virtual float				GetInvincibleInterval()	const = 0;
		virtual Kind				GetKind()			const = 0;
		InitializeParam				GetInitializer()	const;
		virtual Definition::Damage	GetTouchDamage()	const = 0;
		virtual bool				NowProtecting()		const;
		virtual void				GiveDamage( const Definition::Damage &damage ) const;
		/// <summary>
		/// GiveDamage() is not apply damage as immediately, so if you wanna know to will dead by GiveDamage(), you should use this instead of NowDead().
		/// It may return false even when NowDead() is true.
		/// </summary>
		virtual bool				WillDie()			const;
	protected:
		/// <summary>
		/// After this, the "pReceivedDamage" will be reset.
		/// </summary>
		virtual void ApplyReceivedDamageIfHas();
		/// <summary>
		/// Will be called if the hp below zero at ApplyReceivedDamageIfHas().
		/// </summary>
		virtual void DieMoment();
	protected:
		void UpdateInvincibleExistence();
		void UpdateOrientation( bool lookingRight );
		void UpdateMotionIfCan( float elapsedTime, int motionIndex );
		std::vector<Donya::Collision::Box3F> FetchSolidsByBody( const Map &terrain, const Donya::Collision::Box3F &hitBoxVSTerrain, float elapsedTime, const Donya::Vector3 &currentVelocity );
		/// <summary>
		/// Returns the return value of Actor::MoveX().
		/// </summary>
		virtual int MoveOnlyX( float elapsedTime, const std::vector<Donya::Collision::Box3F> &solids );
		/// <summary>
		/// Returns the return value of Actor::MoveY().
		/// </summary>
		virtual int MoveOnlyY( float elapsedTime, const std::vector<Donya::Collision::Box3F> &solids );
		/// <summary>
		/// Returns the return value of Actor::MoveZ().
		/// </summary>
		virtual int MoveOnlyZ( float elapsedTime, const std::vector<Donya::Collision::Box3F> &solids );
		virtual void AppearInit();
		virtual void AppearUpdate( float elapsedTime, const Input &input );
		/// <summary>
		/// Returns true when landing to ground.
		/// </summary>
		virtual bool AppearPhysicUpdate( float elapsedTime, const Map &terrain );
	protected:
		virtual int GetInitialHP() const = 0;
		virtual void AssignMyBody( const Donya::Vector3 &wsPos ) = 0;
		virtual Donya::Vector4x4 MakeWorldMatrix( const Donya::Vector3 &scale, bool enableRotation, const Donya::Vector3 &translation ) const;
	public:
	#if USE_IMGUI
		/// <summary>
		/// Returns the return value of ImGui::TreeNode().
		/// </summary>
		virtual bool ShowImGuiNode( const std::string &nodeCaption );
	#endif // USE_IMGUI
	};
	

	/// <summary>
	/// A container of all Enemies.
	/// </summary>
	class Container
	{
	public:
		// Note: This structure is used for delay making a instance.
		struct BossSet
		{
			int		roomID	= Room::invalidID;
			Kind	kind	= Kind::KindCount;
			InitializeParam			initializer;
			Donya::Collision::Box3F	roomArea;
			std::shared_ptr<Base>	pBoss = nullptr;
		private:
			friend class cereal::access;
			template<class Archive>
			void serialize( Archive &archive, std::uint32_t version )
			{
				archive
				(
					CEREAL_NVP( roomID		),
					CEREAL_NVP( kind		),
					CEREAL_NVP( initializer	),
					CEREAL_NVP( roomArea	),
					CEREAL_NVP( pBoss		)
				);
				if ( 1 <= version )
				{
					// archive();
				}
			}
		};
	private: // shared_ptr<> make be able to copy
		std::vector<BossSet> bosses;
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive( CEREAL_NVP( bosses ) );
			if ( 1 <= version )
			{
				// archive();
			}
		}
		static constexpr const char *ID = "Boss";
	public:
		bool Init( int stageNumber );
		void Uninit();
		void Update( float elapsedTime, const Input &input );
		void PhysicUpdate( float elapsedTime, const Map &terrain );
		void Draw( RenderingHelper *pRenderer ) const;
		void DrawHitBoxes( RenderingHelper *pRenderer, const Donya::Vector4x4 &matVP ) const;
	public:
		/// <summary>
		/// Returns true if at least one boss exists in specified room.<para></para>
		/// ( !IsThereIn ) &lt; A boss is not exist<para></para>
		/// ( IsThereIn &amp; !IsAliveIn ) &lt; A boss is now die performance<para></para>
		/// ( IsThereIn &amp; IsAliveIn ) &lt; A boss is alive<para></para>
		/// </summary>
		bool IsThereIn( int roomID ) const;
		/// <summary>
		/// Returns true if at least one boss exists, and that now alive in specified room.<para></para>
		/// ( !IsThereIn ) &lt; A boss is not exist<para></para>
		/// ( IsThereIn &amp; !IsAliveIn ) &lt; A boss is now die performance<para></para>
		/// ( IsThereIn &amp; IsAliveIn ) &lt; A boss is alive<para></para>
		/// </summary>
		bool IsAliveIn( int roomID ) const;
		/// <summary>
		/// Returns the first living boss found in the specified room. Or nullptr if not found.
		/// </summary>
		std::shared_ptr<const Base> GetBossOrNullptr( int roomID ) const;
		void StartupBossIfStandby( int roomID );
		size_t GetBossCount() const;
	private:
		bool LoadBosses( int stageNumber, bool fromBinary );
		void AppearBoss( size_t appearIndex );
		void RemoveBosses();
		void ClearAllBosses();
	#if USE_IMGUI
	private:
		void AppendBoss( int roomID, const Donya::Collision::Box3F &wsRoomArea, Kind kind, const InitializeParam &parameter );
	public:
		BossSet GetBossSet( size_t instanceIndex );
		void RemakeByCSV( const CSVLoader &loadedData, const House &house );
		void SaveBosses( int stageNumber, bool fromBinary );
	public:
		void ShowImGuiNode( const std::string &nodeCaption, int stageNo );
		void ShowInstanceNode( size_t instanceIndex );
	#endif // USE_IMGUI
	};
}
CEREAL_CLASS_VERSION( Boss::InitializeParam,	0 )
CEREAL_CLASS_VERSION( Boss::Base,				0 )
CEREAL_REGISTER_TYPE( Boss::Base )
CEREAL_REGISTER_POLYMORPHIC_RELATION( Actor, Boss::Base )
CEREAL_CLASS_VERSION( Boss::Container,			0 )
