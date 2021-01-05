#pragma once

#include <array>
#include <functional>
#include <memory>

#include "Donya/ModelMotion.h"
#include "Donya/ModelPose.h"
#include "Donya/Quaternion.h"
#include "Donya/Serializer.h"
#include "Donya/UseImGui.h"
#include "Donya/Vector.h"

#include "Bullet.h"
#include "CSVLoader.h"
#include "Damage.h"
#include "Effect/Effect.h"
#include "FontHelper.h"
#include "Map.h"
#include "Meter.h"
#include "ModelHelper.h"
#include "ObjectBase.h"
#include "Parameter.h"
#include "Renderer.h"
#include "Weapon.h"


class PlayerInitializer
{
private:
	Donya::Vector3	wsInitialPos;	// Store a foot position
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
	/// <summary>
	/// Returns a foot position
	/// </summary>
	Donya::Vector3	GetWorldInitialPos() const;
	bool			ShouldLookingRight() const;
public:
	void AssignParameter( const Donya::Vector3 &wsInitialFootPos, bool lookingRight = true );
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
		static void Increment();
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
		static constexpr int variationCount = 2;
	public:
		Donya::Vector2	moveVelocity;	// Unit velocity
		std::array<bool, variationCount> useJumps  = { false, false };
		std::array<bool, variationCount> useShots  = { false, false };
		std::array<bool, variationCount> useDashes = { false, false };
		std::array<int,  variationCount> shiftGuns = { false, false }; // Positive:Change to next, Negative:Change to previous
		bool			headToDestination = false;	// It priority is greater than the moveVelocity
		Donya::Vector3	wsDestination;				// World space. It is valid when the headToDestination is true
	public:
		static Input GenerateEmpty()
		{
			Input tmp;
			tmp.moveVelocity		= Donya::Vector2::Zero();
			tmp.useJumps.fill( false );
			tmp.useShots.fill( false );
			tmp.useDashes.fill( false );
			tmp.shiftGuns.fill( false );
			tmp.headToDestination	= false;
			tmp.wsDestination		= Donya::Vector3::Zero();
			return tmp;
		}
	};
	enum class MotionKind
	{
		Idle = 0,
		Run,
		Slide,
		Jump_Rise,
		Jump_Fall,
		KnockBack,
		GrabLadder,
		Shot,
		ChargedShot,
		LadderShotLeft,
		LadderChargedShotLeft,
		LadderShotRight,
		LadderChargedShotRight,
		Brace,
		Appear,
		Winning,

		MotionCount
	};
	enum class ShotLevel
	{
		Normal = 0,	// 1 damage.
		Tough,		// 2 damage, Pierce.
		Strong,		// 3 damage, Pierce.

		LevelCount	// Invalid
	};
	static constexpr bool IsFullyCharged( ShotLevel level )
	{
		constexpr int border   = scast<int>( ShotLevel::Strong );
		const     int intLevel = scast<int>( level );
		return ( border <= intLevel );
	}
private:
	class InputManager
	{
	private:
		Input prev;
		Input curr;
		std::array<float, Input::variationCount> keepJumpSeconds;
		std::array<bool,  Input::variationCount> wasReleasedJumps;
	public:
		void Init();
		void Update( const Player &instance, float elapsedTime, const Input &input );
	public:
		int  UseJumpIndex( bool getCurrent = true ) const; // Return -1 if not using
		int  UseShotIndex() const; // Return -1 if not using
		int  UseDashIndex() const; // Return -1 if not using
		int  ShiftGunIndex() const; // Return -1 if not using
		bool UseJump( bool getCurrent = true ) const;
		bool UseShot() const;
		bool UseDash() const;
		int  ShiftGun() const;
		bool Jumpable( int jumpInputIndex ) const;
	public:
		void Overwrite( const Input &overwrite );
		void OverwritePrevious( const Input &overwrite );
	public:
		const Input &Previous() const
		{ return prev; }
		const Input &Current() const
		{ return curr; }
		std::array<float, Input::variationCount> &KeepSecondJumpInput()
		{ return keepJumpSeconds; }
		const std::array<float, Input::variationCount> &KeepSecondJumpInput()  const
		{ return keepJumpSeconds; }
		std::array<bool,  Input::variationCount> &WasReleasedJumpInput()
		{ return wasReleasedJumps; }
		const std::array<bool,  Input::variationCount> &WasReleasedJumpInput() const
		{ return wasReleasedJumps; }
	public:
	#if USE_IMGUI
		void ShowImGuiNode( const std::string &nodeCaption );
	#endif // USE_IMGUI
	};
	class MotionManager
	{
	private:
		MotionKind prevKind = MotionKind::Jump_Fall;
		MotionKind currKind = MotionKind::Jump_Fall;
		ModelHelper::SkinningOperator model;

		Donya::Model::Pose		shotPose;
		Donya::Model::Animator	shotAnimator;
		bool					shotWasCharged = false;
		bool					shouldPoseShot = false;
	public:
		void Init();
		void Update( Player &instance, float elapsedTime, bool stopAnimation = false );
		void Draw( RenderingHelper *pRenderer, const Donya::Vector4x4 &matW, const Donya::Vector3 &blendColor, float blendAlpha ) const;
	public:
		void ResetMotionFrame();
		void QuitShotMotion();
	public:
		bool WasCurrentMotionEnded() const;
		MotionKind CurrentKind() const { return currKind; }
	private:
		void UpdateShotMotion( Player &instance, float elapsedTime );
		void ApplyPartMotion( Player &instance, float elapsedTime, MotionKind useMotion );
		void ExplorePartBone( std::vector<size_t> *pTargetBoneIndices, const std::vector<Donya::Model::Animation::Node> &exploreSkeletal, const std::string &searchBoneRootName );
	private:
		int  ToMotionIndex( MotionKind kind ) const;
		void AssignPose( MotionKind kind );
		bool ShouldEnableLoop( MotionKind kind ) const;
		MotionKind CalcNowKind( Player &instance, float elapsedTime ) const;
	};
	class ShotManager
	{
	private:
		ShotLevel		chargeLevel			= ShotLevel::Normal;
		float			prevChargeSecond	= 0.0f;
		float			currChargeSecond	= 0.0f;
		bool			nowTrigger			= false;
		Donya::Vector3	emissiveColor	{ 0.0f, 0.0f, 0.0f }; // By charge
		Donya::Vector3	destColor		{ 0.0f, 0.0f, 0.0f }; // By charge
		bool			playingChargeSE		= false;
		Effect::Handle	fxComplete;
		Effect::Handle	fxLoop;
	public:
		~ShotManager();
	public:
		void Init();
		void Uninit();
		void Update( const Player &instance, float elapsedTime );
	public:
		void ChargeFully();
		void SetFXPosition( const Donya::Vector3 &wsPosition );
	public:
		bool			IsShotRequested( const Player &instance ) const;
		float			ChargeSecond()		const { return currChargeSecond;	}
		ShotLevel		ChargeLevel()		const { return chargeLevel;			}
		Donya::Vector3	EmissiveColor()		const { return emissiveColor;		}
	private:
		bool NowTriggered( const InputManager &input ) const;
		ShotLevel		CalcChargeLevel() const;
		Donya::Vector3	CalcEmissiveColor() const;
		void AssignLoopFX( Effect::Kind kind );
		void PlayLoopSFXIfStopping();
		void StopLoopSFXIfPlaying( bool forcely = false );
	};
	class Flusher
	{
	private:
		float			workingSeconds	= 0.0f;
		float			timer			= 0.0f;
		Effect::Handle	fxHurt;
	public:
		~Flusher();
	public:
		void Start( float flushingSeconds );
		void Update( const Player &instance, float elapsedTime );
		void SetFXPosition( const Donya::Vector3 &wsPosition );
		bool Drawable() const;
		/// <summary>
		/// It means now invincible.
		/// </summary>
		bool NowWorking() const;
	};
#pragma region Mover
	class MoverBase
	{
	public:
		virtual ~MoverBase() = default;
	public:
		virtual void Init( Player &instance );
		virtual void Uninit( Player &instance ) {}
		virtual void Update( Player &instance, float elapsedTime, const Map &terrain ) = 0;
		virtual void Move( Player &instance, float elapsedTime, const Map &terrain, float roomLeftBorder, float roomRightBorder ) = 0;
		virtual bool NowSliding( const Player &instance ) const { return false; }
		virtual bool NowKnockBacking( const Player &instance ) const { return false; }
		virtual bool NowGrabbingLadder( const Player &instance ) const { return false; }
		virtual bool NowBracing( const Player &instance ) const { return false; }
		virtual bool NowMiss( const Player &instance ) const { return false; }
		virtual bool NowAppearing( const Player &instance ) const { return false; }
		virtual bool NowWinning( const Player &instance ) const { return false; }
		virtual bool Drawable( const Player &instance ) const { return true; }
		virtual bool ShouldChangeMover( const Player &instance ) const = 0;
		virtual std::function<void()> GetChangeStateMethod( Player &instance ) const = 0;
	#if USE_IMGUI
		virtual std::string GetMoverName() const = 0;
	#endif // USE_IMGUI
	protected:
		virtual void AssignBodyParameter( Player &instance );
	protected:
		void MotionUpdate( Player &instance, float elapsedTime, bool stopAnimation = false );
		void MoveOnlyHorizontal( Player &instance, float elapsedTime, const Map &terrain, float roomLeftBorder, float roomRightBorder );
		void MoveOnlyVertical( Player &instance, float elapsedTime, const Map &terrain );
	};
	class Normal : public MoverBase
	{
	private:
		bool gotoSlide		= false;
		bool gotoLadder		= false;
		bool braceOneself	= false;
	public:
		void Update( Player &instance, float elapsedTime, const Map &terrain ) override;
		void Move( Player &instance, float elapsedTime, const Map &terrain, float roomLeftBorder, float roomRightBorder ) override;
		bool NowBracing( const Player &instance ) const override;
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
	public:
		void Init( Player &instance ) override;
		void Uninit( Player &instance ) override;
		void Update( Player &instance, float elapsedTime, const Map &terrain ) override;
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
		Donya::Collision::Box3F	grabArea;
	public:
		void Init( Player &instance ) override;
		void Uninit( Player &instance ) override;
		void Update( Player &instance, float elapsedTime, const Map &terrain ) override;
		void Move( Player &instance, float elapsedTime, const Map &terrain, float roomLeftBorder, float roomRightBorder ) override;
		bool NowGrabbingLadder( const Player &instance ) const override;
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
		void ShotProcess( Player &instance, float elapsedTime );
	private:
		ReleaseWay JudgeWhetherToRelease( Player &instance, float elapsedTime, const Map &terrain ) const;
	};
	class KnockBack : public MoverBase
	{
	private:
		float timer			= 0.0f;
		float motionSpeed	= 1.0f;
	public:
		void Init( Player &instance ) override;
		void Uninit( Player &instance ) override;
		void Update( Player &instance, float elapsedTime, const Map &terrain ) override;
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
		void Update( Player &instance, float elapsedTime, const Map &terrain ) override;
		void Move( Player &instance, float elapsedTime, const Map &terrain, float roomLeftBorder, float roomRightBorder ) override;
		bool NowMiss( const Player &instance ) const override { return true; }
		bool Drawable( const Player &instance ) const override;
		bool ShouldChangeMover( const Player &instance ) const override;
		std::function<void()> GetChangeStateMethod( Player &instance ) const override;
	#if USE_IMGUI
		std::string GetMoverName() const override { return u8"ミス"; }
	#endif // USE_IMGUI
	};
	class Appear : public MoverBase
	{
	private:
		float	timer	= 0.0f;
		bool	visible	= false;
	public:
		void Init( Player &instance ) override;
		void Update( Player &instance, float elapsedTime, const Map &terrain ) override;
		void Move( Player &instance, float elapsedTime, const Map &terrain, float roomLeftBorder, float roomRightBorder ) override;
		bool NowAppearing( const Player &instance ) const override { return true; }
		bool Drawable( const Player &instance ) const override;
		bool ShouldChangeMover( const Player &instance ) const override;
		std::function<void()> GetChangeStateMethod( Player &instance ) const override;
	#if USE_IMGUI
		std::string GetMoverName() const override { return u8"登場"; }
	#endif // USE_IMGUI
	};
	class Leave : public MoverBase
	{
	private:
		float	timer	= 0.0f;
		bool	visible	= true;
	public:
		void Init( Player &instance ) override;
		void Update( Player &instance, float elapsedTime, const Map &terrain ) override;
		void Move( Player &instance, float elapsedTime, const Map &terrain, float roomLeftBorder, float roomRightBorder ) override;
		bool Drawable( const Player &instance ) const override;
		bool ShouldChangeMover( const Player &instance ) const override;
		std::function<void()> GetChangeStateMethod( Player &instance ) const override;
	#if USE_IMGUI
		std::string GetMoverName() const override { return u8"退場"; }
	#endif // USE_IMGUI
	};
	class WinningPose : public MoverBase
	{
	public:
		void Init( Player &instance ) override;
		void Uninit( Player &instance ) override;
		void Update( Player &instance, float elapsedTime, const Map &terrain ) override;
		void Move( Player &instance, float elapsedTime, const Map &terrain, float roomLeftBorder, float roomRightBorder ) override;
		bool NowWinning( const Player &instance ) const override { return true; }
		bool ShouldChangeMover( const Player &instance ) const override;
		std::function<void()> GetChangeStateMethod( Player &instance ) const override;
	#if USE_IMGUI
		std::string GetMoverName() const override { return u8"ガッツポーズ"; }
	#endif // USE_IMGUI
	};
// Mover
#pragma endregion
#pragma region Gun
	class GunBase
	{
	private:
		Definition::WeaponKind kind = Definition::WeaponKind::Buster;
	public:
		virtual ~GunBase() = default;
	public:
		virtual void Init( Player &instance );
		virtual void Uninit( Player &instance );
		virtual void Update( Player &instance, float elapsedTime );
	public:
		virtual bool Chargeable() const = 0;
		virtual bool AllowFireByRelease( ShotLevel nowChargeLevel ) const = 0;
		virtual void Fire( Player &instance, const InputManager &input ) = 0;
	public:
		virtual Definition::WeaponKind GetKind() const = 0;
		virtual Donya::Vector3 GetThemeColor() const;
	#if USE_IMGUI
		virtual std::string GetGunName() const = 0;
	#endif // USE_IMGUI
	};
	class BusterGun : public GunBase
	{
	public:
		bool Chargeable() const override;
		bool AllowFireByRelease( ShotLevel nowChargeLevel ) const override;
		void Fire( Player &instance, const InputManager &input ) override;
	public:
		Definition::WeaponKind GetKind() const override
		{ return Definition::WeaponKind::Buster; }
	#if USE_IMGUI
		std::string GetGunName() const override
		{
			return "BusterGun";
		}
	#endif // USE_IMGUI
	};
	class ShieldGun : public GunBase
	{
	private:
		bool takeShield = false;
	public:
		void Init( Player &instance ) override;
		void Uninit( Player &instance ) override;
		void Update( Player &instance, float elapsedTime ) override;
	public:
		bool Chargeable() const override;
		bool AllowFireByRelease( ShotLevel nowChargeLevel ) const override;
		void Fire( Player &instance, const InputManager &input ) override;
	public:
		Definition::WeaponKind GetKind() const override
		{ return Definition::WeaponKind::SkullShield; }
	#if USE_IMGUI
		std::string GetGunName() const override
		{
			return "ShieldGun";
		}
	#endif // USE_IMGUI
	private:
		void ReleaseShieldHandle( Player &instance );
		Donya::Vector3 CalcThrowDirection( const Player &instance, const InputManager &input ) const;
		Donya::Vector3 CalcShieldPosition( const Player &instance ) const;
		void ExpandShield( Player &instance, const InputManager &input );
		void ThrowShield( Player &instance, const InputManager &input );
	};
// Gun
#pragma endregion
private:
	using						 Actor::body;						// VS a terrain
	Donya::Collision::Box3F				hurtBox;					// VS an attack
	using						 Actor::orientation;
	Donya::Vector3						velocity;					// Z element is almost unused.
	InputManager						inputManager;
	MotionManager						motionManager;
	ShotManager							shotManager;
	Flusher								invincibleTimer;
	std::unique_ptr<MoverBase>			pMover				= nullptr;
	std::unique_ptr<GunBase>			pGun				= nullptr;
	std::shared_ptr<Bullet::Base>		pBullet				= nullptr;
	std::weak_ptr<const Tile>			pTargetLadder{};			// It only used for initialization of Player::GrabLadder as reference
	Definition::WeaponAvailableStatus	availableWeapon;
	float								nowGravity			= 0.0f;
	int									currentHP			= 1;
	float								lookingSign			= 1.0f;	// Current looking direction in world space. 0.0f:Left - 1.0f:Right
	bool								onGround			= false;
	bool								wasJumpedWhileSlide	= false;
	// TODO: These status variables can be combine by replace to MotionKind value
	bool								prevSlidingStatus	= false;
	bool								prevBracingStatus	= false;

	struct DamageDesc
	{
		bool knockedFromRight = true;
		Definition::Damage damage;
	};
	mutable std::unique_ptr<DamageDesc> pReceivedDamage	= nullptr; // Will be made at GiveDamage()
public:
	void Init( const PlayerInitializer &initializer, const Map &terrain, bool withAppearPerformance = true );
	void Uninit();

	void Update( float elapsedTime, const Input &input, const Map &terrain );
	void PhysicUpdate( float elapsedTime, const Map &terrain, float roomLeftBorder, float roomRightBorder );

	void Draw( RenderingHelper *pRenderer ) const;
	void DrawHitBox( RenderingHelper *pRenderer, const Donya::Vector4x4 &matVP, const Donya::Vector4 &unused = { 0.0f, 0.0f, 0.0f, 0.0f } ) const override;
public:
	void RecoverHP( int recovery );
	void ChargeFully();
	/// <summary>
	/// It overwrites to the current status
	/// </summary>
	void ApplyAvailableWeapon( const Definition::WeaponAvailableStatus &newStatus );
	/// <summary>
	/// It adds to the current status
	/// </summary>
	void ApplyAvailableWeapon( const Definition::WeaponKind &unlockKind );
public:
	bool NowMiss() const;
	bool NowGrabbingLadder() const;
	bool NowWinningPose() const;
	int  GetCurrentHP() const;
	Donya::Vector3			GetVelocity()		const;
	Donya::Collision::Box3F	GetHurtBox()		const;
	Donya::Quaternion		GetOrientation()	const;
	Donya::Vector3			GetThemeColor()		const;
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
	void PerformWinning();
	void PerformLeaving();
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
	template<class Gun>
	void AssignGun()
	{
		if ( pGun )
		{
			pGun->Uninit( *this );
			pGun.reset();
		}

		pGun = std::make_unique<Gun>();
		pGun->Init( *this );
	}
	void AssignGunByKind( Definition::WeaponKind kind );
private:
	void AssignCurrentBodyInfo( Donya::Collision::Box3F *pTarget, bool useHurtBoxInfo ) const;
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
	void MoveHorizontal( float elapsedTime );
	void MoveVertical  ( float elapsedTime );
	bool NowShotable( float elapsedTime ) const;
	void ShotIfRequested( float elapsedTime );
	void UpdateOrientation( bool lookingRight );
	void Jump( int inputIndex );
	bool Jumpable( int inputIndex ) const;
	bool WillUseJump() const;
	void Fall( float elapsedTime );
	void Landing();
	void ShiftGunIfNeeded( float elapsedTime );
	void GenerateSlideEffects() const;
private:
	Donya::Vector4x4 MakeWorldMatrix( const Donya::Vector3 &scale, bool enableRotation, const Donya::Vector3 &translation ) const;
public:
#if USE_IMGUI
	void ShowImGuiNode( const std::string &nodeCaption );
#endif // USE_IMGUI
};
