#pragma once

#include <set>

namespace Donya
{
	// Type handled by the Donya::UniqueId<T> class.
	using UniqueIdType = unsigned long long;

	// Unique identifier in type of template.
	// The template type is just specify a id territory.
	// ("<Object>::id == <Object>::id" is valid, "<Dog>::id == <Cat>::id" is meaningless)
	template<typename IdOwner>
	class UniqueId
	{
	private:
		// Ref: https://codereview.stackexchange.com/a/173624
		static UniqueIdType maxIdOfGenerated;
		static std::set<UniqueIdType> returnedIds;
	private:
		// TODO: Make it thread safe
		static UniqueIdType GenerateId()
		{
			if ( returnedIds.empty() )
			{
				return maxIdOfGenerated++;
			}
			// else

			// begin(): O(1), https://cpprefjp.github.io/reference/set/set/begin.html
			// erase( iterator ): O(1), https://cpprefjp.github.io/reference/set/set/erase.html
			// erase( number ): O(logN), https://cpprefjp.github.io/reference/set/set/erase.html
			const auto itr = returnedIds.begin();
			const UniqueIdType reuseId = *itr;
			returnedIds.erase( itr );
			return reuseId;
		}
		// TODO: Make it thread safe
		static void ReturnId( UniqueIdType usedId )
		{
			returnedIds.insert( usedId );
		}
	private:
		UniqueIdType id = 0ULL;
	public: // Allow id duplication
		UniqueId() : id( GenerateId() ) {}
		UniqueId( const UniqueId &rhs ) : id( rhs.id ) {}
		UniqueId( UniqueId &&rhs ) : id( rhs.id ) {}
		UniqueId &operator = ( const UniqueId &rhs )
		{
			if ( &rhs == this ) { return *this; }
			// else
			id = rhs.id;
			return *this;
		}
		UniqueId &operator = ( UniqueId &&rhs )
		{
			if ( &rhs == this ) { return *this; }
			// else
			id = rhs.id;
			return *this;
		}
		virtual ~UniqueId()
		{
			ReturnId( id );
		}
	public:
		UniqueIdType Get() const { return id; }
	};
	template<typename IdOwner>
	UniqueIdType UniqueId<IdOwner>::maxIdOfGenerated{};
	template<typename IdOwner>
	std::set<UniqueIdType> UniqueId<IdOwner>::returnedIds{};
}
