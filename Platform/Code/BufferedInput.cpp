#include "BufferedInput.h"

namespace Input
{
	void BufferedInput::Reset()
	{
		buffer.clear();
		lastAddedStatus = false;
	}
	void BufferedInput::Init( float maxRecordSecond )
	{
		Reset();

		// If set zero, it class does not work because it can not record anything
		lifeSpanSecond = std::max( 0.001f, maxRecordSecond );
	}
	void BufferedInput::Update( float elapsedTime, bool pressed )
	{
		for ( auto &it : buffer )
		{
			it.elapsedSecond += elapsedTime;
		}
		DiscardByLifeSpan();


		AppendIfRecordable( pressed );


		publicBuffer = buffer;
	}
	
	float BufferedInput::PressingSecond( float allowSecond, bool discardFoundInstance ) const
	{
		constexpr float notFound = -1.0f;

		const int count = scast<int>( publicBuffer.size() );
		if ( !count ) { return notFound; }
		// else


		// There is no release record == Still pressing now
		const Part &latest = publicBuffer[count - 1];
		if ( latest.pressed && latest.elapsedSecond <= allowSecond )
		{
			if ( discardFoundInstance )
			{
				buffer.back().pickedUp = true;
			}

			return latest.elapsedSecond;
		}
		// else


		// Search long-pressed record pair

		const Part *pPart = nullptr;
		for ( int i = count - 1; 0 <= i; --i )
		{
			pPart = &publicBuffer[i];

			// There is not possibility anymore
			if ( allowSecond < pPart->elapsedSecond ) { return notFound; }
			// else

			if ( !pPart->pressed ) { continue; }
			if ( pPart->pickedUp ) { continue; }
			// else

			// It records the press and the release alternately, so [i + 1] is release record.
			const Part *pReleased   = &publicBuffer[i + 1];
			const float pressSecond = pReleased->elapsedSecond - pPart->elapsedSecond;

			if ( discardFoundInstance )
			{
				buffer[i].pickedUp = true;
			}

			return pressSecond;
		}

		return notFound;
	}
	
	// HACK: IsReleased() and IsTriggered() are very similar code, we can write these more smart

	bool BufferedInput::IsReleased( float allowSecond, bool discardFoundInstance ) const
	{
		const Part *pPart = nullptr;

		const int count = scast<int>( publicBuffer.size() );
		for ( int i = count - 1; 0 <= i; --i )
		{
			pPart = &publicBuffer[i];

			// There is not possibility anymore
			if ( allowSecond < pPart->elapsedSecond ) { return false; }
			// else

			if ( pPart->pressed  ) { continue; }
			if ( pPart->pickedUp ) { continue; }
			// else

			if ( discardFoundInstance )
			{
				buffer[i].pickedUp = true;
			}

			return true;
		}

		return false;
	}
	bool BufferedInput::IsTriggered( float allowSecond, bool discardFoundInstance ) const
	{
		const Part *pPart = nullptr;

		const int count = scast<int>( publicBuffer.size() );
		for ( int i = count - 1; 0 <= i; --i )
		{
			pPart = &publicBuffer[i];

			// There is not possibility anymore
			if ( allowSecond < pPart->elapsedSecond ) { return false; }
			// else

			if ( !pPart->pressed ) { continue; }
			if ( pPart->pickedUp ) { continue; }
			// else

			if ( discardFoundInstance )
			{
				buffer[i].pickedUp = true;
			}

			return true;
		}

		return false;
	}
	
	void BufferedInput::DiscardByLifeSpan()
	{
		// Keep long press info
		Part lastElement = ( buffer.empty() ) ? Part{} : buffer.back();

		auto result = std::remove_if
		(
			buffer.begin(), buffer.end(),
			[&]( const Part &element )
			{
				return ( lifeSpanSecond <= element.elapsedSecond );
			}
		);

		buffer.erase( result, buffer.end() );

		if ( lastAddedStatus == true && buffer.empty() )
		{
			buffer.emplace_back( lastElement );
		}
	}
	void BufferedInput::AppendIfRecordable( bool pressed )
	{
		// Records only toggle timing(trigger/release)
		if ( pressed == lastAddedStatus ) { return; }
		// else

		lastAddedStatus = pressed;

		Part tmp{};
		tmp.pressed = pressed;
		buffer.emplace_back( tmp );
	}

#if USE_IMGUI
	void BufferedInput::ShowImGuiNode( const char *nodeCaption )
	{
		if ( nodeCaption && !ImGui::TreeNode( nodeCaption ) ) { return; }
		// else

		ImGui::DragFloat( u8"Žõ–½•b”", &lifeSpanSecond, 0.01f );

		auto ShowPart = []( const char *caption, Part *p )
		{
			ImGui::Text( caption );
			ImGui::SameLine();
			ImGui::Checkbox( u8"‰Ÿ‰º", &p->pressed );
			ImGui::SameLine();
			ImGui::Text( u8":[%5.2f]Œo‰ß:", p->elapsedSecond );
			ImGui::SameLine();
			ImGui::Checkbox( u8"’†ŒÃ", &p->pickedUp );
		};

		std::string caption;
		const size_t count = buffer.size();
		for ( size_t i = 0; i < count; ++i )
		{
			// Align two digits
			if ( i < 10 )
			{
				caption = "_" + Donya::MakeArraySuffix( i );
			}
			else
			{
				caption = Donya::MakeArraySuffix( i );
			}

			ShowPart( caption.c_str(), &buffer[i] );
		}

		if ( nodeCaption ) { ImGui::TreePop(); }
	}
#endif // USE_IMGUI
}
