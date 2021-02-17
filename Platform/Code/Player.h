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

#include "BufferedInput.h"
#include "Bullet.h"
#include "Command.h"
#include "CSVLoader.h"
#include "Damage.h"
#include "Effect/Effect.h"
#include "FontHelper.h"
#include "Map.h"
#include "Meter.h"
#include "ModelHelper.h"
#include "NumPad.h"
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
		Donya::Vector2	moveVelocity;	// -1.0f ~ +1.0f
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
		Shoryuken_Fire,
		Shoryuken_Lag,
		Shoryuken_Landing,

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
		std::array<::Input::BufferedInput,	Input::variationCount> jumps;
		std::array<bool,					Input::variationCount> jumpWasReleased_es; // On when the "jumps" records the press, Off when the "jumps" records the release.
		std::array<::Input::BufferedInput,	Input::variationCount> shots;
		std::array<::Input::BufferedInput,	Input::variationCount> dashes;
		std::array<std::pair<int, int>,		Input::variationCount> shiftGuns; // first:curent, second:previous

		Donya::Vector2	moveVelocity;				// -1.0f ~ +1.0f
		bool			headToDestination = false;	// It priority is greater than the moveVelocity
		Donya::Vector3	wsDestination;				// World space. It is valid when the headToDestination is true
	public:
		void Init();
		void Update( const Player &instance, float elapsedTime, const Input &input );
	public:
		bool NowJumpable( bool useSlideParam = false )	const;
		bool NowUseJump( bool useSlideParam = false )	const;	// Returns just is there a jump input
		bool NowReleaseJump()	const;
		bool NowUseShot()		const;
		bool NowTriggerShot()	const;
		bool NowUseDash()		const;
		int  NowShiftGun()		const;
	public:
		void DetainNowJumpInput();
	public:
		/// <summary>
		/// Each element is in range of [-1.0f ~ +1.0f]
		/// </summary>
		const Donya::Vector2 &CurrentMoveDirection() const;
		bool NowHeading() const;
		/// <summary>
		/// It returns the destination of now heading, or origin(0,0,0) if NowHeading() is false.
		/// </summary>
		Donya::Vector3 HeadingDestinationOrOrigin() const;
	private:
		void RegisterCurrentInputs( float elapsedTime, const Input &input );
		int  IndexOfUsingJump( bool useSlideParam = false ) const; // Return -1 if not using
		int  IndexOfReleasingJump()		const; // Return -1 if not using
		int  IndexOfUsingShot()			const; // Return -1 if not using
		int  IndexOfTriggeringShot()	const; // Return -1 if not using
		int  IndexOfUsingDash()			const; // Return -1 if not using
		int  IndexOfShiftingGun()		const; // Return -1 if not using
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
		bool shotWasCharged = false;
		bool shouldPoseShot = false;
	public:
		void Init();
		void Update( Player &instance, float elapsedTime, bool stopAnimation = false );
		void Draw( RenderingHelper *pRenderer, const Donya::Vector4x4 &matW, const Donya::Vector3 &blendColor, float blendAlpha ) const;
	public:
		void ResetMotionFrame();
		void QuitShotMotion();
		void OverwriteLerpSecond( float newSecond );
	public:
		bool WasCurrentMotionEnded() const;
		MotionKind CurrentKind() const { return currKind; }
		bool NowShotPoses() const;
		const Donya::Model::Pose &GetCurrentPose() const;
		const Donya::Model::SkinningModel *GetModelOrNullptr() const;
	private:
		void UpdateShotMotion( Player &instance, float elapsedTime );
		void ApplyPartMotion( Player &instance, float elapsedTime, MotionKind useMotion );
		void ExplorePartBone( std::vector<size_t> *pTargetBoneIndices, const std::vector<Donya::Model::Animation::Node> &exploreSkeletal, const std::string &searchBoneRootName );
	private:
		int  ToMotionIndex( MotionKind kind ) const;
		void AssignPose( MotionKind kind );
		bool ShouldEnableLoop( MotionKind kind ) const;
		MotionKind GetNowKind( Player &instance, float elapsedTime ) const;
	};
	class ShotManager
	{
	private:
		ShotLevel		chargeLevel		= ShotLevel::Normal;
		float			chargeSecond	= 0.0f;
		bool			currUseShot		= false;
		bool			prevUseShot		= false;
		Donya::Vector3	emissiveColor	{ 0.0f, 0.0f, 0.0f }; // By charge
		Donya::Vector3	destColor		{ 0.0f, 0.0f, 0.0f }; // By charge
		bool			playingChargeSE	= false;
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
		float			ChargeSecond()		const { return chargeSecond;	}
		ShotLevel		ChargeLevel()		const { return chargeLevel;		}
		Donya::Vector3	EmissiveColor()		const { return emissiveColor;	}
	private:
		bool			NowTriggered		( const Player &instance ) const;
		ShotLevel		CalcChargeLevel		( float chargingSecond ) const;
		Donya::Vector3	CalcEmissiveColor	( float chargingSecond ) const;
		void AssignLoopFX( Effect::Kind kind );
	private:
		void ChargeUpdate( const Player &instance , float elapsedTime );
		void PlayLoopSFXIfStopping();
		void StopLoopSFXIfPlaying( bool forcely = false );
	};
	class CommandManager
	{
	private:
		using SticksType = std::array<::Input::BufferedInput, NumPad::keyCount>;
	private:
		class Processor
		{
		private:
			int		arraySize			= 0;
			int		progressIndex		= 0;	// It indicates the you should achieve of next
			int		backGroundProgress	= 0;	// It always monitors achievements for update the succeed time
			float	lastElapsedSecond	= 0.0f;	// The elapsed second since last achievement
			float	BGElapsedSecond		= 0.0f;	// The elapsed second of back-ground's
			Command::Part cmd;
		public:
			void Init( const Command::Part &chargeCommand );
			void Update( const SticksType &inputs, float elapsedTime );
			bool Accepted() const;
		private:
			void AdvanceProgressIfPressed( const SticksType &inputs );
		public:
		#if USE_IMGUI
			bool EqualTo( const Command::Part &verify ) const;
			void ShowImGuiNode( const char *nodeCaption );
		#endif // USE_IMGUI
		};
	private:
		bool					currPress	= false;
		bool					prevPress	= false;
		bool					wantFire	= false;
		SticksType				sticks;
		std::vector<Processor>	processors;
	public:
		void Init();
		void Update( Player &instance, float elapsedTime );
		bool WantFire() const { return wantFire; }
	public:
	#if USE_IMGUI
		bool ParametersAreUpdated() const;
		void ShowImGuiNode( const char *nodeCaption );
	#endif // USE_IMGUI
	};
	class Flusher
	{
	private:
		float			workingSecond	= 0.0f;
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
	class LagVision
	{
	private:
		struct Vision
		{
			float				alpha = 1.0f;
			Donya::Model::Pose	pose;
			Donya::Vector4x4	matWorld;
		};
	private:
		std::vector<Vision> visions;
	public:
		void Init();
		void Update( float elapsedTime );
		void Draw( RenderingHelper *pRenderer, const Donya::Model::SkinningModel &model ) const;
	public:
		void Add( const Donya::Model::Pose &pose, const Donya::Vector3 &wsPos, const Donya::Quaternion &orientation );
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
	public:
		virtual MotionKind GetNowMotionKind( const Player &instance ) const = 0;
		virtual bool NowMiss( const Player &instance ) const { return false; }
		virtual bool Drawable( const Player &instance ) const { return true; }
		virtual bool CanShoryuken( const Player &instance ) const { return true; }
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
	public:
		MotionKind GetNowMotionKind( const Player &instance ) const override;
		bool ShouldChangeMover( const Player &instance ) const override;
		std::function<void()> GetChangeStateMethod( Player &instance ) const override;
	private:
		void UpdateVertical( Player &instance, float elapsedTime, const Map &terrain );
	public:
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
			Ladder,
		};
	private:
		Destination	nextStatus		= Destination::None;
		float		timer			= 0.0f;
		bool		takeOverInput	= false; // Take-over the down+jump input to next state
		bool		finishByJump	= false;
	public:
		void Init( Player &instance ) override;
		void Uninit( Player &instance ) override;
		void Update( Player &instance, float elapsedTime, const Map &terrain ) override;
		void Move( Player &instance, float elapsedTime, const Map &terrain, float roomLeftBorder, float roomRightBorder ) override;
	public:
		MotionKind GetNowMotionKind( const Player &instance ) const override;
		bool ShouldChangeMover( const Player &instance ) const override;
		std::function<void()> GetChangeStateMethod( Player &instance ) const override;
	private:
		void UpdateStatus( Player &instance, float elapsedTime, const Map &terrain );
		void UpdateVertical( Player &instance, float elapsedTime );
		void UpdateTakeOverInput( bool pressJump, bool pressDown );
	public:
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
	public:
		MotionKind GetNowMotionKind( const Player &instance ) const override;
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
	public:
		MotionKind GetNowMotionKind( const Player &instance ) const override;
		bool CanShoryuken( const Player &instance ) const override { return false; }
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
	public:
		MotionKind GetNowMotionKind( const Player &instance ) const override;
		bool NowMiss( const Player &instance ) const override { return true; }
		bool Drawable( const Player &instance ) const override;
		bool CanShoryuken( const Player &instance ) const override { return false; }
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
	public:
		MotionKind GetNowMotionKind( const Player &instance ) const override;
		bool Drawable( const Player &instance ) const override;
		bool CanShoryuken( const Player &instance ) const override { return false; }
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
	public:
		MotionKind GetNowMotionKind( const Player &instance ) const override;
		bool Drawable( const Player &instance ) const override;
		bool CanShoryuken( const Player &instance ) const override { return false; }
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
	public:
		MotionKind GetNowMotionKind( const Player &instance ) const override;
		bool CanShoryuken( const Player &instance ) const override { return false; }
		bool ShouldChangeMover( const Player &instance ) const override;
		std::function<void()> GetChangeStateMethod( Player &instance ) const override;
	#if USE_IMGUI
		std::string GetMoverName() const override { return u8"ガッツポーズ"; }
	#endif // USE_IMGUI
	};
	class Shoryuken : public MoverBase
	{
	private:
		float	timer			= 0.0f;
		float	visionInterval	= 0.0f;
		float	riseHSpeedAdjust= 0.0f;
		bool	nowRising		= true;
		bool	wasLanded		= false; // After ended the attack
	public:
		void Init( Player &instance ) override;
		void Uninit( Player &instance ) override;
		void Update( Player &instance, float elapsedTime, const Map &terrain ) override;
		void Move( Player &instance, float elapsedTime, const Map &terrain, float roomLeftBorder, float roomRightBorder ) override;
	public:
		MotionKind GetNowMotionKind( const Player &instance ) const override;
		bool CanShoryuken( const Player &instance ) const override { return false; }
		bool ShouldChangeMover( const Player &instance ) const override;
		std::function<void()> GetChangeStateMethod( Player &instance ) const override;
	private:
		void UpdateHSpeed( Player &instance, float elapsedTime );
		void UpdateVSpeed( Player &instance, float elapsedTime );
	private:
		void GenerateCollision( Player &instance );
		std::shared_ptr<Bullet::Base> FindAliveCollisionOrNullptr( Player &inst );
		void UpdateCollision( Player &instance );
		void RemoveCollision( Player &instance );
	private:
		void GenerateVisionIfNeeded( Player &instance, float elapsedTime );
	public:
	#if USE_IMGUI
		std::string GetMoverName() const override { return u8"昇龍拳"; }
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
	CommandManager						commandManager;
	Flusher								invincibleTimer;
	LagVision							lagVision;
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
	bool								pressJumpSinceSlide	= false;// Information from: Slide state, to: Normal state
	MotionKind							currMotionKind		= MotionKind::Idle;

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
	void DrawVision( RenderingHelper *pRenderer ) const;
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
	bool OnGround() const;
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

		// The Init() may refers now motion kind
		currMotionKind = pMover->GetNowMotionKind( *this );

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
	void UpdateInvincible( float elapsedTime, const Map &terrain );
	void UpdateMover( float elapsedTime, const Map &terrain );
private:
	bool NowShoryuken() const;
	void AssignCurrentBodyInfo( Donya::Collision::Box3F *pTarget, bool useHurtBoxInfo ) const;
	Donya::Collision::Box3F GetNormalBody ( bool ofHurtBox ) const;
	Donya::Collision::Box3F GetSlidingBody( bool ofHurtBox ) const;
	Donya::Collision::Box3F GetLadderGrabArea() const;
	std::shared_ptr<const Tile> FindGrabbingLadderOrNullptr( float verticalInput, const Map &terrain ) const;
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
	void Jump();
	bool Jumpable() const;
	bool WillUseJump() const; // Returns comprehensively judge of the player will jumps
	void Fall( float elapsedTime );
	void Landing();
	void ShiftGunIfNeeded( float elapsedTime );
private:
	void GenerateSlideEffects() const;
	void AddLagVision();
private:
	Donya::Vector4x4 MakeWorldMatrix( const Donya::Vector3 &scale, bool enableRotation, const Donya::Vector3 &translation ) const;
public:
#if USE_IMGUI
	void ShowImGuiNode( const std::string &nodeCaption );
#endif // USE_IMGUI
};
