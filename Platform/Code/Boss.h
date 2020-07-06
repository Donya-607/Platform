#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include <cereal/types/memory.hpp>
#include <cereal/types/polymorphic.hpp>
#include <cereal/types/unordered_map.hpp>

#include "Donya/ModelPose.h"
#include "Donya/ModelMotion.h"
#include "Donya/UseImGui.h"		// Use USE_IMGUI macro
#include "Donya/Serializer.h"
#include "Donya/Vector.h"

#include "CSVLoader.h"
#include "Damage.h"
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
		struct ModelSet
		{
			std::shared_ptr<ModelHelper::SkinningSet> pResource = nullptr;
			Donya::Model::Pose		pose;
			Donya::Model::Animator	animator;
		};
	private: // Seralize values
		InitializeParam initializer;
	protected:
		ModelSet				model;
		using Actor::body;					// VS a terrain
		Donya::Collision::Box3F	hurtBox;	// VS an attack
		Donya::Vector3			velocity;
		Donya::Quaternion		orientation;
		int						roomID		= Room::invalidID;
		int						hp			= 1;	// Alive if this is greater than 0(if 0 < hp)
		bool					isDead		= false;
		bool					wantRemove	= false;
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
				initializer
			);
			if ( 1 <= version )
			{
				// archive( CEREAL_NVP( x ) );
			}
		}
	public:
		virtual void Init( const InitializeParam &parameter );
		virtual void Uninit();
		virtual void Update( float elapsedTime, const Donya::Vector3 &wsTargetPos );
		virtual void PhysicUpdate( float elapsedTime, const std::vector<Donya::Collision::Box3F> &solids );
		virtual void Draw( RenderingHelper *pRenderer ) const;
		virtual void DrawHitBox( RenderingHelper *pRenderer, const Donya::Vector4x4 &matVP ) const;
	public:
		virtual bool NowDead()					const;
		virtual bool ShouldRemove()				const;
		using Actor::GetHitBox;
		Donya::Collision::Box3F	GetHurtBox()	const;
		virtual Kind GetKind()					const = 0;
		InitializeParam GetInitializer()		const;
		virtual Definition::Damage GetTouchDamage() const = 0;
		virtual void GiveDamage( const Definition::Damage &damage ) const;
	protected:
		/// <summary>
		/// Assign the specified motion to model.pose. The animator is not change.
		/// </summary>
		virtual void AssignMotion( int motionIndex );
		/// <summary>
		/// After this, the "pReceivedDamage" will be reset.
		/// </summary>
		virtual void ApplyReceivedDamageIfHas();
		/// <summary>
		/// Will be called if the hp below zero at ApplyReceivedDamageIfHas().
		/// </summary>
		virtual void DieMoment() = 0;
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
	private:
		std::unordered_map<int, std::unique_ptr<Base>> bossPtrs; // Key is room id
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive( CEREAL_NVP( bossPtrs ) );
			if ( 1 <= version )
			{
				// archive();
			}
		}
		static constexpr const char *ID = "Boss";
	public:
		bool Init( int stageNumber );
		void Uninit();
		void Update( float elapsedTime, const Donya::Vector3 &wsTargetPos );
		void PhysicUpdate( float elapsedTime, const std::vector<Donya::Collision::Box3F> &solids );
		void Draw( RenderingHelper *pRenderer ) const;
		void DrawHitBoxes( RenderingHelper *pRenderer, const Donya::Vector4x4 &matVP ) const;
	public:
		
	private:
		bool LoadBosses( int stageNumber, bool fromBinary );
		void RemoveBosses();
		void ClearAllBosses();
	#if USE_IMGUI
	private:
		void AddBoss( Kind kind, const InitializeParam &parameter, int roomID );
	public:
		void RemakeByCSV( const CSVLoader &loadedData, const House &house );
		void SaveBosses( int stageNumber, bool fromBinary );
	public:
		void ShowImGuiNode( const std::string &nodeCaption, int stageNo );
		void ShowInstanceNode( int roomID );
	#endif // USE_IMGUI
	};
}
CEREAL_CLASS_VERSION( Boss::InitializeParam,	0 )
CEREAL_CLASS_VERSION( Boss::Base,				0 )
CEREAL_REGISTER_TYPE( Boss::Base )
CEREAL_REGISTER_POLYMORPHIC_RELATION( Actor, Boss::Base )
CEREAL_CLASS_VERSION( Boss::Container,			0 )
