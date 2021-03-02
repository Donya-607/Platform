#pragma once

#include <vector>
#include <float.h>				// Use FLT_MAX

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
#include "Map.h"
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
		SuperBall,
		Bone,
		TogeheroBody,
		ShoryukenCollision,

		KindCount
	};

	struct BusterParam;
	struct GeneralParam;
	struct SkullBusterParam;
	struct SkullShieldParam;
	struct SuperBallParam;
	struct BoneParam;
	struct TogeheroBodyParam;
	struct ShoryuColParam;
	namespace Parameter
	{
		void Load();

	#if USE_IMGUI
		void Update( const std::string &nodeCaption );
	#endif // USE_IMGUI

		const BusterParam		&GetBuster();
		const GeneralParam		&GetGeneral();
		const SkullBusterParam	&GetSkullBuster();
		const SkullShieldParam	&GetSkullShield();
		const SuperBallParam	&GetSuperBall();
		const BoneParam			&GetBone();
		const TogeheroBodyParam	&GetTogeheroBody();
		const ShoryuColParam	&GetShoryuCol();

		namespace Impl
		{
			void LoadBuster();
			void LoadGeneral();
			void LoadSkullBuster();
			void LoadSkullShield();
			void LoadSuperBall();
			void LoadBone();
			void LoadTogeheroBody();
			void LoadShoryuCol();
		#if USE_IMGUI
			void UpdateBuster( const std::string &nodeCaption );
			void UpdateGeneral( const std::string &nodeCaption );
			void UpdateSkullBuster( const std::string &nodeCaption );
			void UpdateSkullShield( const std::string &nodeCaption );
			void UpdateSuperBall( const std::string &nodeCaption );
			void UpdateBone( const std::string &nodeCaption );
			void UpdateTogeheroBody( const std::string &nodeCaption );
			void UpdateShoryuCol( const std::string &nodeCaption );
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
		// shared_ptr<> make be able to copy
		std::shared_ptr<Definition::Damage> pAdditionalDamage = nullptr; // If it is not a nullptr, the fire requestor desires add a damage.
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
				archive( CEREAL_NVP( pAdditionalDamage ) );
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
		Definition::Damage				damage;
		float							secondToRemove		= FLT_MAX;

		bool							removeIfOutScreen	= true;
		bool							wantRemove			= false;
		mutable bool					wasCollided			= false;
		mutable int						collidedCallingCount= 0; // It counts the count of the called CollidedToObject() until next update. It used for consider to play a collide SE.
		
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
		virtual void PhysicUpdate( float elapsedTime, const Map &terrain );
		virtual void Draw( RenderingHelper *pRenderer ) const;
		virtual void DrawHitBox( RenderingHelper *pRenderer, const Donya::Vector4x4 &matVP ) const;
	public:
		virtual bool Destructible() const;
		virtual bool ShouldRemove() const;
		virtual bool WasProtected() const;
		virtual bool OnOutSide( const Donya::Collision::Box3F &wsScreenHitBox ) const;
		virtual void CollidedToObject( bool otherIsBroken, bool otherIsBullet ) const;
		virtual void ProtectedBy( const Donya::Collision::Box3F		&protectObjectBody ) const;
		virtual void ProtectedBy( const Donya::Collision::Sphere3F	&protectObjectBody ) const;
	protected:
		virtual void ProtectedByImpl( float distLeft, float distRight ) const;
		virtual void ProcessOnOutSide();
	public:
		using						 Solid::GetHitBox;
		virtual Donya::Collision::Sphere3F	GetHitSphere()				const;
		virtual Donya::Collision::Box3F		GetHitBoxSubtractor()		const;
		virtual Donya::Collision::Sphere3F	GetHitSphereSubtractor()	const;
		virtual Kind						GetKind()					const = 0;
		Definition::Damage					GetDamage()					const;
	public:
		virtual Donya::Vector3 GetPosition() const override;
		virtual void SetWorldPosition( const Donya::Vector3 &wsPos );
		virtual void SetVelocity( const Donya::Vector3 &newVelocity );
		virtual void SetLifeTime( float second );
		/// <summary>
		/// Default is allow
		/// </summary>
		void AllowRemovingByOutOfScreen();
		/// <summary>
		/// Default is allow
		/// </summary>
		void DisallowRemovingByOutOfScreen();
	protected:
		virtual void GenerateCollidedEffect() const = 0;
		virtual void GenerateProtectedEffect() const;
		virtual void PlayCollidedSE() const = 0;
		virtual void PlayProtectedSE() const;
		/// <summary>
		/// It will be called when update if it was collided to some object.
		/// </summary>
		virtual void CollidedProcess();
		/// <summary>
		/// It will be called when update if it was protected. Default behavior will disable the exist of hit box.
		/// </summary>
		virtual void ProtectedProcess();
		/// <summary>
		/// Returns constant parameter of damage.
		/// </summary>
		virtual Definition::Damage GetDamageParameter() const = 0;
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
		std::vector<std::shared_ptr<Base>>	copyRequests;
	private:
		Admin() = default;
	public:
		void Update( float elapsedTime, const Donya::Collision::Box3F &wsScreenHitBox );
		void PhysicUpdate( float elasedTime, const Map &terrain );
		void Draw( RenderingHelper *pRenderer ) const;
		void DrawHitBoxes( RenderingHelper *pRenderer, const Donya::Vector4x4 &matVP ) const;
	public:
		void ClearInstances();
		void RequestFire( const FireDesc &parameter );
		void AddCopy( const std::shared_ptr<Base> &pBullet );
	public:
		size_t GetInstanceCount() const;
		bool IsOutOfRange( size_t instanceIndex ) const;
		std::shared_ptr<const Base> GetInstanceOrNullptr( size_t instanceIndex ) const;
		std::shared_ptr<Base> FindInstanceOrNullptr( const std::shared_ptr<Base> &pBullet );
		std::shared_ptr<const Base> FindInstanceOrNullptr( const std::shared_ptr<Base> &pBullet ) const;
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
