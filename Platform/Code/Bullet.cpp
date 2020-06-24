#include "Bullet.h"

#include <algorithm>	// Use std::remove_if

namespace Bullet
{
	void Buster::Init( const FireDesc &parameter )
	{

	}
	void Buster::Uninit()
	{

	}
	void Buster::Update( float elapsedTime )
	{

	}
	void Buster::Draw( RenderingHelper *pRenderer ) const
	{

	}
	void Buster::DrawHitBox( RenderingHelper *pRenderer, const Donya::Vector4x4 &VP ) const
	{

	}
	bool Buster::ShouldRemove() const
	{
		return wantRemove;
	}
#if USE_IMGUI
	void Buster::ShowImGuiNode( const std::string &nodeCaption )
	{
		if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
		// else

		ImGui::TreePop();
	}
#endif // USE_IMGUI


	void Admin::Update( float elapsedTime )
	{
		GenerateRequestedFires();
		generateRequests.clear();

		for ( auto &it : bullets )
		{
			it.Update( elapsedTime );

			if ( it.ShouldRemove() )
			{
				it.Uninit();
			}
		}

		RemoveInstancesIfNeeds();
	}
	void Admin::Draw( RenderingHelper *pRenderer ) const
	{
		if ( !pRenderer ) { return; }
		// else
		for ( const auto &it : bullets )
		{
			it.Draw( pRenderer );
		}
	}
	void Admin::DrawHitBoxes( RenderingHelper *pRenderer, const Donya::Vector4x4 &VP ) const
	{
		if ( !pRenderer ) { return; }
		// else
		for ( const auto &it : bullets )
		{
			it.DrawHitBox( pRenderer, VP );
		}
	}
	void Admin::ClearInstances()
	{
		for ( auto &it : bullets )
		{
			it.Uninit();
		}
		bullets.clear();
	}
	void Admin::RequestFire( const FireDesc &parameter )
	{
		generateRequests.emplace_back( parameter );
	}
	void Admin::GenerateRequestedFires()
	{
		for ( const auto &it : generateRequests )
		{
			Buster tmp{};
			tmp.Init( it );
			bullets.emplace_back( std::move( tmp ) );
		}
	}
	void Admin::RemoveInstancesIfNeeds()
	{
		auto result = std::remove_if
		(
			bullets.begin(), bullets.end(),
			[]( Buster &element )
			{
				return element.ShouldRemove();
			}
		);
		bullets.erase( result, bullets.end() );
	}
#if USE_IMGUI
	void Admin::ShowImGuiNode( const std::string &nodeCaption )
	{
		if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
		// else

		ImGui::TreePop();
	}
#endif // USE_IMGUI
}
