#pragma once

#include <functional>
#include <memory>

#include "Donya/ModelMotion.h"
#include "Donya/ModelPolygon.h"
#include "Donya/ModelPose.h"
#include "Donya/Quaternion.h"
#include "Donya/Serializer.h"
#include "Donya/UseImGui.h"
#include "Donya/Vector.h"

#include "CSVLoader.h"
#include "Damage.h"
#include "Map.h"
#include "Meter.h"
#include "ModelHelper.h"
#include "ObjectBase.h"
#include "Parameter.h"
#include "Renderer.h"


class PlayerInitializer
{
private:
	Donya::Vector3	wsInitialPos;
	bool			lookingRight = true;
private:
	friend class cereal::access;
	template<class Archive>
	void serialize( Archive &archive, std::uint32_t version )
	{
		archive
		(
			CEREAL_NVP( wsInitialPos	),
			CEREAL_NVP( lookingRight	)
		);

		if ( 1 <= version )
		{
			// archive( CEREAL_NVP( x ) );
		}
	}
	static constexpr const char *ID = "PlayerInit";
public:
	Donya::Vector3	GetWorldInitialPos() const;
	bool			ShouldLookingRight() const;
public:
	void LoadParameter( int stageNo );
private:
	void LoadBin( int stageNo );
	void LoadJson( int stageNo );
#if USE_IMGUI
public:
	void RemakeByCSV( const CSVLoader &loadedData );
	void SaveBin( int stageNo );
	void SaveJson( int stageNo );
public:
	void ShowImGuiNode( const std::string &nodeCaption, int stageNo, bool allowShowIONode = true );
#endif // USE_IMGUI
};
CEREAL_CLASS_VERSION( PlayerInitializer, 0 )


struct PlayerParam;
class  Player : public Actor
{
public:
	struct Remaining
	{
	private:
		static int  count;
	public:
		static int  Get();
		static void Set( int newCount );
		static void Decrement();
	};
private:
	static ParamOperator<PlayerParam> paramInstance;
public:
	static bool LoadResource();
	static const ParamOperator<PlayerParam> &Parameter();
#if USE_IMGUI
	static void UpdateParameter( const std::string &nodeCaption );
#endif // USE_IMGUI
public:
	struct Input
	{
		Donya::Vector2	moveVelocity;	// Unit velocity
		bool			useJump = false;
		bool			useShot = false;
	};
	enum class MotionKind
	{
		Idle = 0,
		Run,
		Slide,
		Jump,
		KnockBack,
		GrabLadder,

		MotionCount
	};
	enum class ShotLevel
	{
		Normal,	// 1 damage.
		Tough,	// 2 damage, Pierce.
		Strong,	// 3 damage, Pierce.

		LevelCount // Invalid
	};
private:
	class MotionManager
	{
	private:
		MotionKind prevKind = MotionKind::Jump;
		MotionKind currKind = MotionKind::Jump;
		ModelHelper::SkinningOperator model;
	public:
		void Init();
		void Update( Player &instance, float elapsedTime );
		void Draw( RenderingHelper *pRenderer, const Donya::Vector4x4 &matW ) const;
	private:
		int  ToMotionIndex( MotionKind kind ) const;
		void AssignPose( MotionKind kind );
		bool ShouldEnableLoop( MotionKind kind ) const;
		MotionKind CalcNowKind( Player &instance, float elapsedTime ) const;
	};
	class ShotManager
	{
	private:
		ShotLevel chargeLevel  = ShotLevel::Normal;
		float prevChargeSecond = 0.0f;
		float currChargeSecond = 0.0f;
	public:
		void Init();
		void Update( float elapsedTime, Input input );
	public:
		bool		IsShotRequested() const;
		float		ChargeSecond()	const { return currChargeSecond;	}
		ShotLevel	ChargeLevel()	const { return chargeLevel;			}
	private:
		void CalcChargeLevel();
	};
	class Flusher
	{
	private:
		float workingSeconds	= 0.0f;
		float timer				= 0.0f;
	public:
		void Start( float flushingSeconds );
		void Update( float elapsedTime );
		bool Drawable() const;
		/// <summary>
		/// It means now invincible.
		/// </summary>
		bool NowWorking() const;
	};
	class MoverBase
	{
	public:
		virtual ~MoverBase() = default;
	public:
		virtual void Init( Player &instance );
		virtual void Uninit( Player &instance ) {}
		virtual void Update( Player &instance, float elapsedTime, Input input, const Map &terrain ) = 0;
		virtual void Move( Player &instance, float elapsedTime, const Map &terrain, float roomLeftBorder, float roomRightBorder ) = 0;
		virtual bool NowSliding( const Player &instance ) const { return false; }
		virtual bool NowKnockBacking( const Player &instance ) const { return false; }
		virtual bool NowGrabbingLadder( const Player &instance ) const { return false; }
		virtual bool NowMiss( const Player &instance ) const { return false; }
		virtual bool Drawable( const Player &instance ) const { return true; }
		virtual bool ShouldChangeMover( const Player &instance ) const = 0;
		virtual std::function<void()> GetChangeStateMethod( Player &instance ) const = 0;
	#if USE_IMGUI
		virtual std::string GetMoverName() const = 0;
	#endif // USE_IMGUI
	protected:
		virtual void AssignBodyParameter( Player &instance );
	protected:
		void MotionUpdate( Player &instance, float elapsedTime );
		void MoveOnlyHorizontal( Player &instance, float elapsedTime, const Map &terrain, float roomLeftBorder, float roomRightBorder );
		void MoveOnlyVertical( Player &instance, float elapsedTime, const Map &terrain );
	};
	class Normal : public MoverBase
	{
	private:
		bool gotoSlide  = false;
		bool gotoLadder = false;
	public:
		void Update( Player &instance, float elapsedTime, Input input, const Map &terrain ) override;
		void Move( Player &instance, float elapsedTime, const Map &terrain, float roomLeftBorder, float roomRightBorder ) override;
		bool ShouldChangeMover( const Player &instance ) const override;
		std::function<void()> GetChangeStateMethod( Player &instance ) const override;
	#if USE_IMGUI
		std::string GetMoverName() const override { return u8"通常"; }
	#endif // USE_IMGUI
	};
	class Slide : public MoverBase
	{
	private:
		enum class Destination
		{
			None,
			Normal,
		};
	private:
		Destination	nextStatus	= Destination::None;
		float		timer		= 0.0f;
		float		slideSign	= 1.0f;
	public:
		void Init( Player &instance ) override;
		void Uninit( Player &instance ) override;
		void Update( Player &instance, float elapsedTime, Input input, const Map &terrain ) override;
		void Move( Player &instance, float elapsedTime, const Map &terrain, float roomLeftBorder, float roomRightBorder ) override;
		bool NowSliding( const Player &instance ) const override { return true; }
		bool ShouldChangeMover( const Player &instance ) const override;
		std::function<void()> GetChangeStateMethod( Player &instance ) const override;
	#if USE_IMGUI
		std::string GetMoverName() const override { return u8"スライディング"; }
	#endif // USE_IMGUI
	private:
		void AssignBodyParameter( Player &instance ) override;
	};
	class GrabLadder : public MoverBase
	{
	private:
		enum class ReleaseWay
		{
			None,
			Climb,
			Release,
			Dismount
		};
	private:
		ReleaseWay releaseWay	= ReleaseWay::None;
		float	shotLagSecond	= 0.0f;
		float	lookingSign		= 1.0f;
		Donya::Collision::Box3F	grabArea;
	public:
		void Init( Player &instance ) override;
		void Uninit( Player &instance ) override;
		void Update( Player &instance, float elapsedTime, Input input, const Map &terrain ) override;
		void Move( Player &instance, float elapsedTime, const Map &terrain, float roomLeftBorder, float roomRightBorder ) override;
		bool NowGrabbingLadder( const Player &instance ) const override { return true; }
		bool ShouldChangeMover( const Player &instance ) const override;
		std::function<void()> GetChangeStateMethod( Player &instance ) const override;
	#if USE_IMGUI
		std::string GetMoverName() const override { return u8"はしごつかまり"; }
	#endif // USE_IMGUI
	private:
		void AssignBodyParameter( Player &instance ) override;
		void LookToFront( Player &instance );
	private:
		bool NowUnderShotLag() const;
		void ShotProcess( Player &instance, float elapsedTime, Input input );
	private:
		ReleaseWay JudgeWhetherToRelease( Player &instance, float elapsedTime, Input input, const Map &terrain ) const;
	};
	class KnockBack : public MoverBase
	{
	private:
		float timer = 0.0f;
	public:
		void Init( Player &instance ) override;
		void Uninit( Player &instance ) override;
		void Update( Player &instance, float elapsedTime, Input input, const Map &terrain ) override;
		void Move( Player &instance, float elapsedTime, const Map &terrain, float roomLeftBorder, float roomRightBorder ) override;
		bool NowKnockBacking( const Player &instance ) const override { return true; }
		bool ShouldChangeMover( const Player &instance ) const override;
		std::function<void()> GetChangeStateMethod( Player &instance ) const override;
	#if USE_IMGUI
		std::string GetMoverName() const override { return u8"のけぞり"; }
	#endif // USE_IMGUI
	};
	class Miss : public MoverBase
	{
	public:
		void Init( Player &instance ) override;
		void Update( Player &instance, float elapsedTime, Input input, const Map &terrain ) override;
		void Move( Player &instance, float elapsedTime, const Map &terrain, float roomLeftBorder, float roomRightBorder ) override;
		bool NowMiss( const Player &instance ) const override { return true; }
		bool Drawable( const Player &instance ) const override;
		bool ShouldChangeMover( const Player &instance ) const override;
		std::function<void()> GetChangeStateMethod( Player &instance ) const override;
	#if USE_IMGUI
		std::string GetMoverName() const override { return u8"ミス"; }
	#endif // USE_IMGUI
	};
private:
	using Actor::body;						// VS a terrain
	Donya::Collision::Box3F		hurtBox;	// VS an attack
	Donya::Vector3				velocity;	// Z element is almost unused.
	Donya::Quaternion			orientation;
	MotionManager				motionManager;
	ShotManager					shotManager;
	Flusher						invincibleTimer;
	Meter::Drawer				hpDrawer;
	std::unique_ptr<MoverBase>	pMover					= nullptr;
	std::weak_ptr<const Tile>	pTargetLadder{};		// It only used for initialization of Player::GrabLadder as reference
	int							currentHP				= 1;
	float						keepJumpSecond			= 0.0f;
	bool						wasReleasedJumpInput	= false;
	bool						prevSlidingStatus		= false;
	bool						onGround				= false;

	struct DamageDesc
	{
		bool knockedFromRight = true;
		Definition::Damage damage;
	};
	mutable std::unique_ptr<DamageDesc> pReceivedDamage	= nullptr; // Will be made at GiveDamage()
public:
	void Init( const PlayerInitializer &initializer );
	void Uninit();

	void Update( float elapsedTime, Input input, const Map &terrain );
	void PhysicUpdate( float elapsedTime, const Map &terrain, float roomLeftBorder, float roomRightBorder );

	void Draw( RenderingHelper *pRenderer ) const;
	void DrawHitBox( RenderingHelper *pRenderer, const Donya::Vector4x4 &matVP, const Donya::Vector4 &unused = { 0.0f, 0.0f, 0.0f, 0.0f } ) const override;
	void DrawMeter( float drawDepth = 0.0f ) const;
public:
	bool NowMiss() const;
	bool NowGrabbingLadder() const;
	using				 Actor::GetHitBox;
	Donya::Collision::Box3F		GetHurtBox()		const;
	Donya::Quaternion			GetOrientation()	const;
	void GiveDamage( const Definition::Damage &damage, const Donya::Collision::Box3F	&collidingHitBox ) const;
	void GiveDamage( const Definition::Damage &damage, const Donya::Collision::Sphere3F	&collidingHitBox ) const;
	/// <summary>
	/// GiveDamage() is not apply damage as immediately, so if you wanna know to will dead by GiveDamage(), you should use this instead of NowDead().
	/// It may return false even when NowDead() is true.
	/// </summary>
	virtual bool				WillDie()			const;
public:
	void KillMe();
	void KillMeIfCollideToKillAreas( float elapsedTime, const Map &terrain );
private:
	void GiveDamageImpl( const Definition::Damage &damage, float distLeft, float distRight ) const;
	/// <summary>
	/// After this, the "pReceivedDamage" will be reset.
	/// </summary>
	void ApplyReceivedDamageIfHas( float elapsedTime, const Map &terrain );
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
	void AssignBodyInfo( Donya::Collision::Box3F *pTarget, bool useHurtBoxInfo ) const;
	Donya::Collision::Box3F GetNormalBody ( bool ofHurtBox ) const;
	Donya::Collision::Box3F GetSlidingBody( bool ofHurtBox ) const;
	Donya::Collision::Box3F GetLadderGrabArea() const;
	std::vector<Donya::Collision::Box3F> FetchAroundSolids( const Donya::Collision::Box3F &searchingBody, const Donya::Vector3 &movement, const Map &terrain ) const;
	std::vector<Donya::Collision::Box3F> FetchAroundKillAreas( const Donya::Collision::Box3F &searchingBody, const Donya::Vector3 &movement, const Map &terrain ) const;
	bool WillCollideToAroundTiles( const Donya::Collision::Box3F &verifyBody, const Donya::Vector3 &movement, const Map &terrain ) const;
	using Actor::MoveX;
	using Actor::MoveY;
	using Actor::MoveZ;
	using Actor::DrawHitBox;
	void MoveHorizontal( float elapsedTime, Input input );
	void MoveVertical  ( float elapsedTime, Input input );
	bool NowShotable() const;
	void ShotIfRequested( float elapsedTime, Input input );
	void UpdateOrientation( bool lookingRight );
	void Jump();
	bool Jumpable() const;
	void Fall( float elapsedTime, Input input );
	void Landing();
private:
	Donya::Vector4x4 MakeWorldMatrix( const Donya::Vector3 &scale, bool enableRotation, const Donya::Vector3 &translation ) const;
public:
#if USE_IMGUI
	void ShowImGuiNode( const std::string &nodeCaption );
#endif // USE_IMGUI
};
