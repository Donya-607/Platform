#include "CollisionWorld.h"

namespace Donya
{
	namespace Collision
	{
		size_t World::Register( Body *bodyAddress )
		{
			const size_t appendIndex = bodyPtrs.size();
			bodyPtrs.emplace_back( bodyAddress );
			return appendIndex;
		}
		void World::RegisterOnce( Body *bodyAddress )
		{
			bodyPtrsOnce.emplace_back( bodyAddress );
		}

		void World::ClearAll()
		{
			bodyPtrs.clear();
			bodyPtrsOnce.clear();
		}
		void World::Clear( size_t id )
		{
			// Validation
			if ( bodyPtrs.size() <= id ) { return; }
			// else


			// Clear
			// For keep other ids, assign nullptr instead of erase the one.
			bodyPtrs[id] = nullptr;
		}

		void World::Resolve()
		{
			size_t bodyCount = bodyPtrs.size() + bodyPtrsOnce.size();
			
			// Prepare all valid pointers into one array
			std::vector<Body *> allBodyPtrs;
			allBodyPtrs.reserve( bodyCount );
			for ( auto &p : bodyPtrs )
			{
				if ( !p )
				{
					bodyCount--;
					continue;
				}
				// else
				
				allBodyPtrs.emplace_back( p );
			}
			for ( auto &p : bodyPtrsOnce )
			{
				if ( !p )
				{
					bodyCount--;
					continue;
				}
				// else
				
				allBodyPtrs.emplace_back( p );
			}


			// Resolve
			Body *pI = nullptr;
			Body *pJ = nullptr;
			for ( size_t i = 0; i < bodyCount; ++i )
			{
				pI = allBodyPtrs[i];
				for ( size_t j = i + 1; j < bodyCount; ++j )
				{
					pJ = allBodyPtrs[j];

					pI->ResolveIntersectionWith( pJ );
				}
			}


			// Clear the once pointers
			bodyPtrsOnce.clear();
		}
	}
}