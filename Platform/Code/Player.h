#pragma once

#include <memory>

#include "Donya/ModelMotion.h"
#include "Donya/ModelPolygon.h"
#include "Donya/ModelPose.h"
#include "Donya/Quaternion.h"
#include "Donya/Serializer.h"
#include "Donya/UseImGui.h"
#include "Donya/Vector.h"

#include "CSVLoader.h"
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
	void RemakeByCSV( const CSVLoader &loadedData );
private:
	void LoadBin( int stageNo );
	void LoadJson( int stageNo );
#if USE_IMGUI
public:
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
		Jump,

		MotionCount
	};
private:
	class MotionManager
	{
	private:
		MotionKind prevKind = MotionKind::Jump;
		MotionKind currKind = MotionKind::Jump;
		Donya::Model::Animator	animator;
		Donya::Model::Pose		pose;
	public:
		void Init();
		void Update( Player &instance, float elapsedTime );
	public:
		const Donya::Model::Pose &GetPose() const;
	private:
		int  ToMotionIndex( MotionKind kind ) const;
		bool ShouldEnableLoop( MotionKind kind ) const;
		void AssignPose( MotionKind kind );
		MotionKind CalcNowKind( Player &instance ) const;
	};
private:
	using Actor::body;					// VS a terrain
	Donya::Collision::Box3F	hurtBox;	// VS an attack
	Donya::Vector3			velocity;	// Z element is almost unused.
	Donya::Quaternion		orientation;
	MotionManager			motionManager;
	float					keepJumpSecond			= 0.0f;
	float					keepShotSecond			= 0.0f;
	bool					wasReleasedJumpInput	= false;
	bool					onGround				= false;
public:
	void Init( const PlayerInitializer &initializer );
	void Uninit();

	void Update( float elapsedTime, Input input );
	void PhysicUpdate( float elapsedTime, const std::vector<Donya::Collision::Box3F> &solids );

	void Draw( RenderingHelper *pRenderer );
	void DrawHitBox( RenderingHelper *pRenderer, const Donya::Vector4x4 &matVP );
public:
	using Actor::GetHitBox;
	Donya::Collision::Box3F	GetHurtBox()		const;
	Donya::Quaternion		GetOrientation()	const;
private:
	using Actor::MoveX;
	using Actor::MoveY;
	using Actor::MoveZ;
	using Actor::DrawHitBox;
	void MoveHorizontal( float elapsedTime, Input input );
	void MoveVertical  ( float elapsedTime, Input input );
	void Shot( float elapsedTime, Input input );
private:
	Donya::Vector4x4 MakeWorldMatrix( const Donya::Vector3 &scale, bool enableRotation, const Donya::Vector3 &translation ) const;
public:
#if USE_IMGUI
	void ShowImGuiNode( const std::string &nodeCaption );
#endif // USE_IMGUI
};
