#pragma once

#include <vector>

#include "CollisionBody.h"

namespace Donya
{
	namespace Collision
	{
		class World
		{
		private:
			std::vector<Body *> bodyPtrs;		// Resolve targets
			std::vector<Body *> bodyPtrsOnce;	// Resolve targets just once
		public:
			// Register until ClearAll() a body as resolve target, and return the id for access the registered body.
			// The World accesses the body by the address.
			// The returned id will be invalid when after called ClearAll().
			size_t Register( Body *bodyAddress );
			// Register until Resolve() a body as resolve target.
			// The World accesses the body by the address.
			// If your body's address will be changed in a lot(like in std::vector), please use this instead of Register().
			void RegisterOnce( Body *bodyAddress );
			// Clear the registration of Register(), by the id returned by Register().
			// The id will be invalid.
			void Clear( size_t bodyId );
			// Clear all registrations by Register() and RegisterOnce().
			// All id of returned by Register() will be invalid.
			void ClearAll();
		public:
			// Resolve all intersections between registered bodies.
			// It clears registrations by RegisterOnce() after resolve.
			void Resolve();

			// TODO: Provide to be able to specify the iterating count of Resolve().
		};
	}
}
