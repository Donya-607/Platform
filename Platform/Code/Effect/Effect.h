#pragma once

#include <string>

#include "Effekseer.h"

#include "../Donya/Quaternion.h"
#include "../Donya/Vector.h"

#include "EffectKind.h"

namespace Effect
{
	/// <summary>
	/// It can be copy, but the old handle will be discarded, so you must careful the instance's life-time if that effect playing as looping.
	/// </summary>
	class Handle
	{
	public:
		/// <summary>
		/// Returns invalid object if specified effect was not loaded.
		/// </summary>
		static Handle Generate( Kind effectAttribute, const Donya::Vector3 &position, int32_t startFrame = 0 );
	private:
		static constexpr Effekseer::Handle invalidID = -1;
		Effekseer::Handle handle = invalidID;
	public:
		/// <summary>
		/// The old handle will be discarded, so you must careful the instance's life-time if that effect playing as looping.
		/// </summary>
		void Assign( const Effekseer::Handle &handle );
		/// <summary>
		/// disconnect my handle from effect system's instance.
		/// The instance of effect is still alive, so you must careful the instance's life-time if that effect playing as looping.
		/// </summary>
		void Disable();
	public:
		void SetScale( float scale );
		void SetScale( float scaleX, float scaleY, float scaleZ );
		void SetScale( const Donya::Vector3 &scale );
		void SetRotation( float pitch, float yaw, float roll );
		void SetRotation( const Donya::Vector3 &axis, float angleRadian );
		void SetRotation( const Donya::Quaternion &unitOrientation );
		void SetPosition( const Donya::Vector3 &position );
		void Move( const Donya::Vector3 &velocity );
		void Stop();
	public:
		/// <summary>
		/// Returns false if my handle is invalid, or Effect System did not initialized.
		/// </summary>
		bool IsExists() const;
	};
}
