#pragma once

#include <memory>
#include <string>
#include <vector>

#include <cereal/types/memory.hpp>
#include <cereal/types/polymorphic.hpp>
#include <cereal/types/vector.hpp>

#include "Donya/UseImGui.h"		// Use USE_IMGUI macro
#include "Donya/Serializer.h"
#include "Donya/Template.h"		// Use Singleton<>
#include "Donya/Vector.h"

#include "CSVLoader.h"
#include "Damage.h"
#include "Map.h"
#include "ObjectBase.h"

namespace Enemy
{
	enum class Kind
	{
		SuperBallMachine,
		Togehero,
		// SkeletonJoe,
		// ShieldAttacker,
		// Battonton,
		// SkullMet,
		// Imorm,
		
		KindCount
	};

	struct SuperBallMachineParam;
	struct TogeheroParam;
	namespace Parameter
	{
		void Load();

		const SuperBallMachineParam	&GetSuperBallMachine();
		const TogeheroParam			&GetTogehero();

	#if USE_IMGUI
		void Update( const std::string &nodeCaption );
	#endif // USE_IMGUI
		namespace Impl
		{
			void LoadSuperBallMachine();
			void LoadTogehero();
		#if USE_IMGUI
			void UpdateSuperBallMachine( const std::string &nodeCaption );
			void UpdateTogehero( const std::string &nodeCaption );
		#endif // USE_IMGUI
		}
	}

	bool LoadResource();

	struct InitializeParam
	{
		enum class LookDirection
		{
			ToTarget,
			Right,
			Left
		};
	public:
		Donya::Vector3	wsPos;
		LookDirection	lookDirection = LookDirection::ToTarget;
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive
			(
				CEREAL_NVP( wsPos			)
				//, CEREAL_NVP( lookingRight	)
			);
			if ( 1 <= version )
			{
				archive( CEREAL_NVP( lookDirection ) );
			}
			if ( 2 <= version )
			{
				// archive( CEREAL_NVP( x ) );
			}
		}
	public:
	#if USE_IMGUI
		void ShowImGuiNode( const std::string &nodeCaption );
	#endif // USE_IMGUI
	};

	class Base : public Actor
	{
	private: // Seralize values
		InitializeParam initializer;
	protected:
		ModelHelper::SkinningOperator	model;
		using					 Actor::body;		// VS a terrain
		Donya::Collision::Box3F			hurtBox;	// VS an attack
		using					 Actor::orientation;
		Donya::Vector3					velocity;
		int								hp					= 1;	// Alive if this is greater than 0(if 0 < hp)
		bool							wantRemove			= false;
		bool							waitForRespawn		= false;
		bool							onOutSidePrevious	= true;	// Used for judging to respawn
		bool							onOutSideCurrent	= true;	// Used for judging to respawn
		// shared_ptr<> make be able to copy
		mutable std::shared_ptr<Definition::Damage> pReceivedDamage	= nullptr; // Will be made at GiveDamage()
	public:
		Base() = default;
		Base( const Base &  ) = default;
		Base(       Base && ) = default;
		Base &operator  = ( const Base &  ) = default;
		Base &operator  = (       Base && ) = default;
		virtual ~Base() = default;
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive
			(
				cereal::base_class<Actor>( this ),
				CEREAL_NVP( initializer	),
				CEREAL_NVP( hp			)
			);
			if ( 1 <= version )
			{
				// archive( CEREAL_NVP( x ) );
			}
		}
	public:
		virtual void Init( const InitializeParam &parameter, const Donya::Vector3 &wsTargetPos, const Donya::Collision::Box3F &wsScreenHitBox );
		virtual void Uninit();
		virtual void Update( float elapsedTime, const Donya::Vector3 &wsTargetPos, const Donya::Collision::Box3F &wsScreenHitBox );
		virtual void PhysicUpdate( float elapsedTime, const Map &terrain );
		virtual void Draw( RenderingHelper *pRenderer ) const;
		virtual void DrawHitBox( RenderingHelper *pRenderer, const Donya::Vector4x4 &matVP ) const;
	public:
		virtual bool				ShouldRemove()		const;
		using				 Actor::GetHitBox;
		Donya::Collision::Box3F		GetHurtBox()		const;
		virtual Kind				GetKind()			const = 0;
		InitializeParam				GetInitializer()	const;
		virtual Definition::Damage	GetTouchDamage()	const = 0;
		virtual void				GiveDamage( const Definition::Damage &damage ) const;
		virtual bool				WillDie()			const;
	public:
		bool NowWaiting() const;
	protected:
		void UpdateMotionIfCan( float elapsedTime, int motionIndex );
		void UpdateOutSideState( const Donya::Collision::Box3F &wsScreenHitBox );
		bool OnOutSide() const;
		void BeginWaitIfActive();
		void RespawnIfSpawnable( const Donya::Vector3 &wsTargetPos );
		/// <summary>
		/// After this, the "pReceivedDamage" will be reset.
		/// </summary>
		virtual void ApplyReceivedDamageIfHas();
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
	class Admin : public Donya::Singleton<Admin>
	{
		friend Donya::Singleton<Admin>;
	private: // shared_ptr<> make be able to copy
		std::vector<std::shared_ptr<Base>> enemyPtrs;
	private:
		Admin() = default;
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive( CEREAL_NVP( enemyPtrs ) );
			if ( 1 <= version )
			{
				// archive();
			}
		}
		static constexpr const char *ID = "Enemy";
	public:
		void Uninit();
		void Update( float elapsedTime, const Donya::Vector3 &wsTargetPos, const Donya::Collision::Box3F &wsScreenHitBox );
		void PhysicUpdate( float elapsedTime, const Map &terrain );
		void Draw( RenderingHelper *pRenderer ) const;
		void DrawHitBoxes( RenderingHelper *pRenderer, const Donya::Vector4x4 &matVP ) const;
	public:
		void ClearInstances();
		bool LoadEnemies( int stageNumber, const Donya::Vector3 &wsTargetPos, const Donya::Collision::Box3F &wsScreenHitBox, bool fromBinary );
	public:
		size_t GetInstanceCount() const;
		bool IsOutOfRange( size_t instanceIndex ) const;
		std::shared_ptr<const Base> GetInstanceOrNullptr( size_t instanceIndex ) const;
	private:
		void RemoveEnemiesIfNeeded();
	#if USE_IMGUI
		void AppendEnemy( Kind appendKind, const InitializeParam &parameter, const Donya::Vector3 &wsTargetPos, const Donya::Collision::Box3F &wsScreenHitBox );
	public:
		void RemakeByCSV( const CSVLoader &loadedData, const Donya::Vector3 &wsTargetPos, const Donya::Collision::Box3F &wsScreenHitBox );
		void SaveEnemies( int stageNumber, bool fromBinary );
	public:
		void ShowImGuiNode( const std::string &nodeCaption, int stageNo, const Donya::Vector3 &wsTargetPos, const Donya::Collision::Box3F &wsScreenHitBox );
		void ShowInstanceNode( size_t instanceIndex );
	#endif // USE_IMGUI
	};


	/// <summary>
	/// The basically parameter used for level design
	/// </summary>
	struct BasicParam
	{
	public:
		int					hp				= 1;
		Definition::Damage	touchDamage;
		Donya::Vector3		hitBoxOffset	{ 0.0f, 0.0f, 0.0f };
		Donya::Vector3		hitBoxSize		{ 1.0f, 1.0f, 1.0f };
		Donya::Vector3		hurtBoxOffset	{ 0.0f, 0.0f, 0.0f };
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
				// archive( CEREAL_NVP( x ) );
			}
		}
	public:
	#if USE_IMGUI
		void ShowImGuiNode( const std::string &nodeCaption );
	#endif // USE_IMGUI
	};
}
CEREAL_CLASS_VERSION( Enemy::InitializeParam,	1 )
CEREAL_CLASS_VERSION( Enemy::Base,				0 )
CEREAL_REGISTER_TYPE( Enemy::Base )
CEREAL_REGISTER_POLYMORPHIC_RELATION( Actor, Enemy::Base )
CEREAL_CLASS_VERSION( Enemy::Admin,				0 )
CEREAL_CLASS_VERSION( Enemy::BasicParam,		0 )
