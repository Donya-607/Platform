#pragma once

#include <vector>

#undef max
#undef min
#include <cereal/types/memory.hpp>

#include "Donya/Collision.h"
#include "Donya/Quaternion.h"
#include "Donya/Serializer.h"
#include "Donya/Template.h"		// Use Donya::Singleton<>
#include "Donya/UseImGui.h"
#include "Donya/Vector.h"

#include "Damage.h"
#include "ModelHelper.h"
#include "ObjectBase.h"
#include "Renderer.h"

namespace Bullet
{
	enum class Kind
	{
		Buster,
		SkullBuster,
		SkullShield,

		KindCount
	};

	struct BusterParam;
	struct GeneralParam;
	struct SkullBusterParam;
	struct SkullShieldParam;
	namespace Parameter
	{
		void Load();

		const BusterParam		&GetBuster();
		const GeneralParam		&GetGeneral();
		const SkullBusterParam	&GetSkullBuster();
		const SkullShieldParam	&GetSkullShield();

	#if USE_IMGUI
		void Update( const std::string &nodeCaption );
	#endif // USE_IMGUI
		namespace Impl
		{
			void LoadBuster();
			void LoadGeneral();
			void LoadSkullBuster();
			void LoadSkullShield();
		#if USE_IMGUI
			void UpdateBuster( const std::string &nodeCaption );
			void UpdateGeneral( const std::string &nodeCaption );
			void UpdateSkullBuster( const std::string &nodeCaption );
			void UpdateSkullShield( const std::string &nodeCaption );
		#endif // USE_IMGUI
		}
	}

	bool LoadResource();

	/// <summary>
	/// Generate parameters. Descriptor.
	/// </summary>
	struct FireDesc
	{
	public: // Serialize members
		Kind			kind			= Kind::Buster;
		float			initialSpeed	= 1.0f;			// [m/s]
		Donya::Vector3	direction{ 1.0f, 0.0f, 0.0f };	// Unit vector
		Donya::Vector3	position;
		std::shared_ptr<Definition::Damage> pOverrideDamage = nullptr; // If it is not a nullptr, the fire requestor desires override damage.
	public:
		Donya::Collision::IDType owner = Donya::Collision::invalidID;
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive
			(
				CEREAL_NVP( initialSpeed	),
				CEREAL_NVP( direction		),
				CEREAL_NVP( position		)
			);
			if ( 1 <= version )
			{
				archive( CEREAL_NVP( kind ) );
			}
			if ( 2 <= version )
			{
				archive( CEREAL_NVP( pOverrideDamage ) );
			}
			if ( 3 <= version )
			{
				// archive( CEREAL_NVP() );
			}
		}
	public:
	#if USE_IMGUI
		void ShowImGuiNode( const std::string &nodeCaption, bool positionIsRelative = true );
	#endif // USE_IMGUI
	};


	/// <summary>
	/// You must call Init() when generate and Uninit() before remove. Because these method manages instance count.
	/// </summary>
	class Base : public Solid
	{
	protected:
		ModelHelper::SkinningOperator	model;
		// Please make unused hit box has zero sizes(or radius), and false exist flag.
		using					 Solid::body;		// Hit box as AABB
		Donya::Collision::Sphere3F		hitSphere;	// Hit box as Sphere
		Donya::Vector3					velocity;	// [m/s]
		Donya::Quaternion				orientation;
		bool							wantRemove		= false;
		mutable bool					wasCollided		= false;
		
		// It means: "(was protected) from XXX side".
		enum class ProtectedInfo
		{
			None, ByRightSide, ByLeftSide, Processed
		};
		mutable ProtectedInfo			wasProtected	= ProtectedInfo::None;
	public:
		Base() = default;
		Base( const Base &  ) = default;
		Base(       Base && ) = default;
		Base &operator = ( const Base &  ) = default;
		Base &operator = (       Base && ) = default;
		virtual ~Base() = default;
	public:
		virtual void Init( const FireDesc &parameter );
		virtual void Uninit() = 0;
		virtual void Update( float elapsedTime, const Donya::Collision::Box3F &wsScreenHitBox );
		virtual void PhysicUpdate( float elapsedTime );
		virtual void Draw( RenderingHelper *pRenderer ) const;
		virtual void DrawHitBox( RenderingHelper *pRenderer, const Donya::Vector4x4 &matVP ) const;
	public:
		virtual bool ShouldRemove() const;
		virtual bool OnOutSide( const Donya::Collision::Box3F &wsScreenHitBox ) const;
		virtual void CollidedToObject() const;
		virtual void ProtectedBy( const Donya::Collision::Box3F		&protectObjectBody ) const;
		virtual void ProtectedBy( const Donya::Collision::Sphere3F	&protectObjectBody ) const;
	protected:
		virtual void ProtectedByImpl( float distLeft, float distRight ) const;
	public:
		using						 Solid::GetHitBox;
		virtual Donya::Collision::Sphere3F	GetHitSphere()	const;
		virtual Kind						GetKind()		const = 0;
		virtual Definition::Damage			GetDamage()		const = 0;
	public:
		virtual void SetWorldPosition( const Donya::Vector3 &wsPos );
		virtual void SetVelocity( const Donya::Vector3 &newVelocity );
	protected:
		/// <summary>
		/// It will be called when update if it was collided to some object.
		/// </summary>
		virtual void CollidedProcess();
		/// <summary>
		/// It will be called when update if it was protected. Default behavior will disable the exist of hit box.
		/// </summary>
		virtual void ProtectedProcess();
		virtual void AssignBodyParameter( const Donya::Vector3 &wsPos ) = 0;
		virtual void InitBody( const FireDesc &parameter );
		virtual void UpdateOrientation( const Donya::Vector3 &direction );
		void UpdateMotionIfCan( float elapsedTime, int motionIndex );
		Donya::Vector4x4 MakeWorldMatrix( const Donya::Vector3 &scale, bool enableRotation, const Donya::Vector3 &translation ) const;
	public:
	#if USE_IMGUI
		virtual void ShowImGuiNode( const std::string &nodeCaption );
	#endif // USE_IMGUI
	};


	/// <summary>
	/// Container of all bullets.
	/// </summary>
	class Admin : public Donya::Singleton<Admin>
	{
		friend Donya::Singleton<Admin>;
	private: // shared_ptr<> make be able to copy
		std::vector<std::shared_ptr<Base>>	bulletPtrs;
		std::vector<FireDesc>				generateRequests;
	private:
		Admin() = default;
	public:
		void Update( float elapsedTime, const Donya::Collision::Box3F &wsScreenHitBox );
		void PhysicUpdate( float elasedTime );
		void Draw( RenderingHelper *pRenderer ) const;
		void DrawHitBoxes( RenderingHelper *pRenderer, const Donya::Vector4x4 &matVP ) const;
	public:
		void ClearInstances();
		void RequestFire( const FireDesc &parameter );
	public:
		size_t GetInstanceCount() const;
		bool IsOutOfRange( size_t instanceIndex ) const;
		std::shared_ptr<const Base> GetInstanceOrNullptr( size_t instanceIndex ) const;
	private:
		void GenerateRequestedFires();
		void RemoveInstancesIfNeeds();
	public:
	#if USE_IMGUI
		void ShowImGuiNode( const std::string &nodeCaption );
	#endif // USE_IMGUI
	};
}
CEREAL_CLASS_VERSION( Bullet::FireDesc, 2 )
