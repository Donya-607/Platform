#ifndef INCLUDED_LEX_CAMERA_H_
#define INCLUDED_LEX_CAMERA_H_

#include <memory>

#include "Constant.h"	// Use DEBUG_MODE macro.
#include "Quaternion.h"
#include "UseImGui.h"
#include "Vector.h"

namespace Donya
{
	/// <summary>
	/// The interface of camera.
	/// </summary>
	class ICamera
	{
	public:
		/// <summary>
		/// Specify the camera's movement.
		/// </summary>
		enum class Mode
		{
			Free,		// The rotate origin is myself. The focus point will be invalid.
			Look,		// Keep looking the focus point. The rotation is invalid.
			Satellite,	// The movement is like satellite. Keep looking the focus point. The position and focus point will move by moveVelocity. The orientation will rotate by roll, pitch and yaw.
		};
		/// <summary>
		/// Store information of drive the camera.<para></para>
		/// The rotation order is roll->pitch->yaw.
		/// </summary>
		struct Controller
		{
			Donya::Vector3	moveVelocity{};				// Set move vector(contain speed).
			float			roll{};						// Radian. Rotate with Z axis, Local space.
			float			pitch{};					// Radian. Rotate with X axis, Local space.
			float			yaw{};						// Radian. Rotate with Y axis, World space.
			float			slerpPercent{ 1.0f };		// Set percentage of interpolation(0.0f ~ 1.0f). This affects the movement and the rotation.
			bool			moveInLocalSpace{ true };	// Specify the space of movement(world-space or camera-space). If the Satellite mode, moving space is fixed to camera-space.
		public:
			// This condition is same as default constructed condition.
			void SetNoOperation()
			{
				moveVelocity = Donya::Vector3::Zero();
				roll = pitch = yaw = 0.0f;
				slerpPercent = 0.0f;
			}
		};
	// private:
	public: // I want to hide the "Configuration" struct, but if hide it, a derived from "BaseCamera" class can not access. :(
		/// <summary>
		/// Use when change the mode. Store a user specified parameter, then change the mode and set the parameter.
		/// </summary>
		struct Configuration
		{
			float				FOV{};
			float				zNear{};
			float				zFar{};
			Donya::Vector2		screenSize{};
			Donya::Vector3		pos{};
			Donya::Vector3		focus{};
			Donya::Quaternion	orientation{};
		};
	public:
		class BaseCamera;
	private:
		std::unique_ptr<BaseCamera> pCamera;
		Mode currentMode;
	public:
		ICamera();
		~ICamera();
	public:
		void Init( Mode initialMode );
		void Uninit();

		void Update( Controller controller );
	public:
		void ChangeMode( Mode nextMode );

		/// <summary>
		/// This set only z-range(near, far), so you should call SetProjectionXXX() after this.
		/// </summary>
		void SetZRange					( float zNear, float zFar );
		/// <summary>
		/// This set only Field-Of-View, so you should call SetProjectionXXX() after this.
		/// </summary>
		void SetFOV						( float FOV );
		/// <summary>
		/// Please don't set zero to the height of screenSize. Because will be divided the width by height.<para></para>
		/// This set only screen size, so you should call SetProjectionXXX() after this.
		/// </summary>
		void SetScreenSize				( const Donya::Vector2 &screenSize );
		/// <summary>
		/// The orientation will be also setting.
		/// </summary>
		void SetPosition				( const Donya::Vector3 &point );
		/// <summary>
		/// The orientation will be also setting.
		/// </summary>
		void SetFocusPoint				( const Donya::Vector3 &point );
		/// <summary>
		/// This method is valid when the camera's orientation is valid.
		/// </summary>
		void SetFocusToFront			( float distance );
		void SetOrientation				( const Donya::Quaternion &orientation );
		/// <summary>
		/// If set { 0, 0 } to the "viewSize", use registered screen size.
		/// </summary>
		void SetProjectionOrthographic	( const Donya::Vector2 &viewSize = { 0.0f, 0.0f } );
		/// <summary>
		/// If set 0.0f to the "aspectRatio", calculate by registered screen size.
		/// </summary>
		void SetProjectionPerspective	( float aspectRatio = 0.0f );

		float				GetFOV()				const;
		float				GetZNear()				const;
		float				GetZFar()				const;
		Donya::Vector2		GetScreenSize()			const;
		Donya::Vector3		GetPosition()			const;
		Donya::Vector3		GetFocusPoint()			const;
		Donya::Quaternion	GetOrientation()		const;
		Donya::Vector4x4	CalcViewMatrix()		const;
		Donya::Vector4x4	GetProjectionMatrix()	const;

	#if USE_IMGUI

		void ShowImGuiNode();

	#endif // USE_IMGUI
	private:
		void AssertIfNullptr() const;

		Configuration BuildCurrentConfiguration() const;
	};
}

#endif // INCLUDED_LEX_CAMERA_H_