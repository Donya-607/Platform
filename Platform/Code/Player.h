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
#include "FontHelper.h"
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
		Donya::Vector2 moveVelocity; // Unit velocity
		std::array<bool, variationCount> useJumps  = { false, false };
		std::array<bool, variationCount> useShots  = { false, false };
		std::array<bool, variationCount> useDashes = { false, false };
		std::array<int,  variationCount> shiftGuns = { false, false }; // Positive:Change to next, Negative:Change to previous
	public:
		static Input GenerateEmpty()
		{
			Input tmp;
			tmp.moveVelocity = Donya::Vector2::Zero();
			tmp.useJumps.fill( false );
			tmp.useShots.fill( false );
			tmp.useDashes.fill( false );
			tmp.shiftGuns.fill( false );
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
		LadderShotLeft,
		LadderShotRight,
		Brace,

		MotionCount
	};
	enum class ShotLevel
	{
		Normal = 0,	// 1 damage.
		Tough,		// 2 damage, Pierce.
		Strong,		// 3 damage, Pierce.

		LevelCount	// Invalid
	};
	enum class GunKind
	{
		Buster,
		Shield,

		GunCount,
	};
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
		void Update( float elapsedTime, const Input input );
	public:
		int  UseJumpIndex() const; // Return -1 if not using
		int  UseShotIndex() const; // Return -1 if not using
		int  UseDashIndex() const; // Return -1 if not using
		int  ShiftGunIndex() const; // Return -1 if not using
		bool UseJump() const;
		bool UseShot() const;
		bool UseDash() const;
		int  ShiftGun() const;
		bool Jumpable( int jumpInputIndex ) const;
	public:
		void Overwrite( const Input overwrite );
		void OverwritePrevious( const Input overwrite );
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
		bool					shouldPoseShot = false;
	public:
		void Init();
		void Update( Player &instance, float elapsedTime, bool stopAnimation = false );
		void Draw( RenderingHelper *pRenderer, const Donya::Vector4x4 &matW, const Donya::Vector3 &blendColor, float blendAlpha ) const;
	public:
		void QuitShotMotion();
	public:
		MotionKind CurrentKind() const { return currKind; }
	private:
		void UpdateShotMotion( Player &instance, float elapsedTime );
		void ApplyPartMotion( Player &instance, float elapsedTime, MotionKind useMotion, const ModelHelper::PartApply &partData );
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
	public:
		void Init();
		void Update( const Player &instance, float elapsedTime, const InputManager &input );
	public:
		bool			IsShotRequested( const Player &instance ) const;
		float			ChargeSecond()		const { return currChargeSecond;	}
		ShotLevel		ChargeLevel()		const { return chargeLevel;			}
		Donya::Vector3	EmissiveColor()		const { return emissiveColor;		}
	private:
		bool NowTriggered( const InputManager &input ) const;
		ShotLevel		CalcChargeLevel();
		Donya::Vector3	CalcEmissiveColor();
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
		float		slideSign	= 1.0f;
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
// Mover
#pragma endregion
#pragma region Gun
	class GunBase
	{
	private:
		GunKind kind = GunKind::Buster;
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
		virtual GunKind GetKind() const = 0;
	};
	class BusterGun : public GunBase
	{
	public:
		bool Chargeable() const override;
		bool AllowFireByRelease( ShotLevel nowChargeLevel ) const override;
		void Fire( Player &instance, const InputManager &input ) override;
	public:
		GunKind GetKind() const override
		{ return GunKind::Buster; }
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
		GunKind GetKind() const override
		{ return GunKind::Shield; }
	private:
		Donya::Vector3 CalcThrowDirection( const Player &instance, const InputManager &input ) const;
		Donya::Vector3 CalcShieldPosition( const Player &instance ) const;
		void ExpandShield( Player &instance, const InputManager &input );
		void ThrowShield( Player &instance, const InputManager &input );
	};
// Gun
#pragma endregion
private:
	using					 Actor::body;		// VS a terrain
	Donya::Collision::Box3F			hurtBox;	// VS an attack
	using					 Actor::orientation;
	Donya::Vector3					velocity;	// Z element is almost unused.
	InputManager					inputManager;
	MotionManager					motionManager;
	ShotManager						shotManager;
	Flusher							invincibleTimer;
	std::unique_ptr<MoverBase>		pMover					= nullptr;
	std::unique_ptr<GunBase>		pGun					= nullptr;
	std::shared_ptr<Bullet::Base>	pBullet					= nullptr;
	std::weak_ptr<const Tile>		pTargetLadder{};				// It only used for initialization of Player::GrabLadder as reference
	int								currentHP				= 1;
	float							lookingSign				= 1.0f;	// Current looking direction in world space. 0.0f:Left - 1.0f:Right
	bool							onGround				= false;
	// TODO: These status variables can be combine by replace to MotionKind value
	bool							prevSlidingStatus		= false;
	bool							prevBracingStatus		= false;

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
public:
	void RecoverHP( int recovery );
public:
	bool NowMiss() const;
	bool NowGrabbingLadder() const;
	int  GetCurrentHP() const;
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
	void AssignGunByKind( GunKind kind );
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
	void ShiftGunIfNeeded();
private:
	Donya::Vector4x4 MakeWorldMatrix( const Donya::Vector3 &scale, bool enableRotation, const Donya::Vector3 &translation ) const;
public:
#if USE_IMGUI
	void ShowImGuiNode( const std::string &nodeCaption );
#endif // USE_IMGUI
};
