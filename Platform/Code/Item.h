#pragma once

#include <vector>

#include "Donya/Template.h"	// Use Donya::Singleton<>
#include "Donya/UseImGui.h"
#include "Donya/Vector.h"

#include "Map.h"
#include "ObjectBase.h"

namespace Item
{
	enum class Kind
	{
		ExtraLife,
		LifeEnergy_Big,
		LifeEnergy_Small,

		KindCount
	};

	struct ItemParam;
	namespace Parameter
	{
		void Load();

	#if USE_IMGUI
		void Update( const std::string &nodeCaption );
	#endif // USE_IMGUI

		const ItemParam &GetItem();

		namespace Impl
		{
			void LoadItem();
		#if USE_IMGUI
			void UpdateItem( const std::string &nodeCaption );
		#endif // USE_IMGUI
		}
	}


	bool LoadResource();


	struct InitializeParam
	{
		Kind			kind		= Kind::ExtraLife;
		float			aliveSecond	= -1.0f; // A negative value means living infinitely
		Donya::Vector3	wsPos;
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive
			(
				CEREAL_NVP( kind		),
				CEREAL_NVP( aliveSecond	),
				CEREAL_NVP( wsPos		)
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


	class Item : public Actor
	{
	private: // Serialize members
		InitializeParam initializer;
	private:
		using Actor::body;					// VS a terrain
		Donya::Collision::Box3F	catchArea;	// VS the player
		Donya::Vector3			velocity;	// Z element is almost unused.
		Donya::Quaternion		orientation;
		float					aliveTimer = 0.0f;
		bool					wantRemove = false;
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive
			(
				cereal::base_class<Actor>( this ),
				CEREAL_NVP( initializer )
			);
			if ( 1 <= version )
			{
				// archive( CEREAL_NVP( x ) );
			}
		}
	public:
		void Init( const InitializeParam &parameter );
		void Uninit();
		void Update( float elapsedTime, const Donya::Vector3 &wsTargetPos, const Donya::Collision::Box3F &wsScreenHitBox );
		void PhysicUpdate( float elapsedTime, const Map &terrain );
		void Draw( RenderingHelper *pRenderer ) const;
		void DrawHitBox( RenderingHelper *pRenderer, const Donya::Vector4x4 &matVP ) const;
	public:
		bool					ShouldRemove()		const;
		using			 Actor::GetHitBox;
		Donya::Collision::Box3F	GetCatchArea()		const;
		Kind					GetKind()			const;
		InitializeParam			GetInitializer()	const;
	private:
		float GetGravity() const;
		void AssignMyBody( const Donya::Vector3 &wsPos );
		Donya::Vector4x4 MakeWorldMatrix( const Donya::Vector3 &scale, bool enableRotation, const Donya::Vector3 &translation ) const;
	public:
	#if USE_IMGUI
		/// <summary>
		/// Returns the return value of ImGui::TreeNode().
		/// </summary>
		bool ShowImGuiNode( const std::string &nodeCaption );
	#endif // USE_IMGUI
	};


	/// <summary>
	/// A container of all Items.
	/// </summary>
	class Admin : public Donya::Singleton<Admin>
	{
		friend Donya::Singleton<Admin>;
	private:
		std::vector<Item> items;
	private:
		Admin() = default;
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive( CEREAL_NVP( items ) );
			if ( 1 <= version )
			{
				// archive();
			}
		}
		static constexpr const char *ID = "Item";
	public:
		void Uninit();
		void Update( float elapsedTime, const Donya::Vector3 &wsTargetPos, const Donya::Collision::Box3F &wsScreenHitBox );
		void PhysicUpdate( float elapsedTime, const Map &terrain );
		void Draw( RenderingHelper *pRenderer ) const;
		void DrawHitBoxes( RenderingHelper *pRenderer, const Donya::Vector4x4 &matVP ) const;
	public:
		void ClearInstances();
		bool LoadItems( int stageNumber, bool fromBinary );
	public:
		size_t GetInstanceCount() const;
		bool IsOutOfRange( size_t instanceIndex ) const;
		/// <summary>
		/// Returns the pointer to an element of the internal vector, so you should be careful to do not changes the internal element count.
		/// </summary>
		const Item *GetInstanceOrNullptr( size_t instanceIndex ) const;
	private:
		void RemoveItems();
	#if USE_IMGUI
	public:
		void RemakeByCSV( const CSVLoader &loadedData );
		void SaveItems( int stageNumber, bool fromBinary );
	public:
		void ShowImGuiNode( const std::string &nodeCaption, int stageNo );
		void ShowInstanceNode( size_t instanceIndex );
	#endif // USE_IMGUI
	};
}
CEREAL_CLASS_VERSION( Item::InitializeParam,	0 )
CEREAL_CLASS_VERSION( Item::Item,				0 )
CEREAL_REGISTER_TYPE( Item::Item )
CEREAL_REGISTER_POLYMORPHIC_RELATION( Actor, Item::Item )
CEREAL_CLASS_VERSION( Item::Admin,				0 )
