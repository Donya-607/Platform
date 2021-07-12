#pragma once

#include <iterator> // std::next()
#include <set>

namespace Donya
{
	// Type handled by the Donya::UniqueId<T> class.
	using UniqueIdType = unsigned long long;

	// Unique identifier in type of template.
	// The template type is just specify a id territory.
	// ("<Object>::id == <Object>::id" is valid, "<Dog>::id == <Cat>::id" is meaningless)
	template<typename IdOwner>
	class UniqueId final
	{
	private:
		// Ref: https://codereview.stackexchange.com/a/173624
		static UniqueIdType maxIdOfGenerated;
		static std::unordered_set<UniqueIdType> returnedIds;
		// It used like reference counting. It needed because UniqueId<> allowing the duplication.
		// It prevents to do not add the cloned id into "returnedId".
		// If allow to add the cloned id, GenerateId() may generate the same as cloned id.
		// For uniqueness, we must prevent it.
		static std::unordered_multiset<UniqueIdType> usingIds;
	private:
		// TODO: Make it thread safe
		static UniqueIdType GenerateId()
		{
			UniqueIdType generatedId{};

			if ( returnedIds.empty() )
			{
				generatedId = maxIdOfGenerated;
				++maxIdOfGenerated;
			}
			else
			{
				// begin(): O(1), https://cpprefjp.github.io/reference/set/set/begin.html
				// erase( iterator ): O(1), https://cpprefjp.github.io/reference/set/set/erase.html
				// erase( number ): O(logN), https://cpprefjp.github.io/reference/set/set/erase.html
				const auto itr = returnedIds.begin();
				generatedId = *itr;
				returnedIds.erase( itr );
			}
			
			usingIds.insert( generatedId );

			return generatedId;
		}
		// TODO: Make it thread safe
		static UniqueIdType CloneId( UniqueIdType cloningId )
		{
			usingIds.insert( cloningId );
			return cloningId;
		}
		// TODO: Make it thread safe
		static void ReturnId( UniqueIdType usedId )
		{
			auto itr = usingIds.find( usedId );

			// "Not found" may not occur, but fail safe it
			if ( itr == usingIds.end() ) { return; }
			// else


			// If next is not existed, reference count is 1.
			if ( std::next( itr ) == usingIds.end() )
			{
				// This id is completely returned
				returnedIds.insert( usedId );

				// Reference count to be zero
				// multiset::erase( iterator ) is O(1): https://cpprefjp.github.io/reference/set/multiset/erase.html
				usingIds.erase( itr );

				return;
			}
			// else


			// Decrement the reference count one
			usingIds.erase( itr );
		}
		// TODO: Make it thread safe
		static void OverwriteIdBy( UniqueIdType *pOverwritee, const UniqueIdType &overwriter )
		{
			// We can not think the case(overwrite by returned id),
			// but fail safe it
			auto itr = returnedIds.find( overwriter );
			if ( itr != returnedIds.end() )
			{
				returnedIds.erase( itr );
			}

			// We must add it firstly, then erase it.
			// If arguments are same, id will be added and erased, finally is not change.
			// If arguments are not same, it overwrites as expectedly.

			usingIds.insert( overwriter );

			ReturnId( *pOverwritee );

			*pOverwritee = overwriter;
		}
	private:
		UniqueIdType id = 0ULL;
	public: // Allow id duplication
		UniqueId() : id( GenerateId() ) {}
		UniqueId( UniqueIdType clone )   : id( CloneId( clone  ) ) {}
		UniqueId( const UniqueId &rhs )  : id( CloneId( rhs.id ) ) {}
		UniqueId(       UniqueId &&rhs ) : id( CloneId( rhs.id ) ) {}
		UniqueId &operator = ( const UniqueId &rhs )
		{
			if ( &rhs == this ) { return *this; }
			// else
			OverwriteBy( rhs.id );
			return *this;
		}
		UniqueId &operator = (       UniqueId &&rhs )
		{
			if ( &rhs == this ) { return *this; }
			// else
			OverwriteBy( rhs.id );
			return *this;
		}
		~UniqueId()
		{
			// Call it even if either the id is cloned or is not cloned.
			// ReturnId() will handles the cloned id case.
			ReturnId( id );
		}
	public:
		UniqueIdType Get() const { return id; }
		void OverwriteBy( const UniqueIdType &overwriter )
		{
			OverwriteIdBy( &id, overwriter );
		}
	};
	template<typename IdOwner>
	UniqueIdType UniqueId<IdOwner>::maxIdOfGenerated{};
	template<typename IdOwner>
	std::unordered_set<UniqueIdType> UniqueId<IdOwner>::returnedIds{};
	template<typename IdOwner>
	std::unordered_multiset<UniqueIdType> UniqueId<IdOwner>::usingIds{};

	template<typename IdOwner>
	bool operator == ( const UniqueId<IdOwner> &lhs, const UniqueId<IdOwner> &rhs )
	{
		return ( lhs.Get() == rhs.Get() );
	}
	template<typename IdOwner>
	bool operator != ( const UniqueId<IdOwner> &lhs, const UniqueId<IdOwner> &rhs )
	{
		return !( lhs == rhs );
	}
}
