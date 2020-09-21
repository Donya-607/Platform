#pragma once

#include "Donya/Serializer.h"
#include "Donya/UseImGui.h"
#include "Donya/Vector.h"

#include "SkyMap.h"

/// <summary>
/// Draw a sky with prefer condition by SkyMap
/// </summary>
class Sky
{
public:
	struct Parameter
	{
		// Draw conditions
		Donya::Vector3	radians			{ 0.0f, 0.0f, 0.0f };
		Donya::Vector3	scale			{ 1.0f, 1.0f, 1.0f };
		// Color adjustments
		Donya::Vector3	upperLimit		{ 1.0f, 1.0f, 1.0f };
		Donya::Vector3	lowerLimit		{ 0.3f, 0.3f, 0.3f };
		Donya::Vector3	magnification	{ 3.0f, 2.0f, 1.5f };
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive
			(
				CEREAL_NVP( radians			),
				CEREAL_NVP( scale			),
				CEREAL_NVP( upperLimit		),
				CEREAL_NVP( lowerLimit		),
				CEREAL_NVP( magnification	)
			);

			if ( 1 <= version )
			{
				// archive( CEREAL_NVP( x ) );
			}
		}
	public:
	#if USE_IMGUI
		void ShowImGuiNode();
	#endif // USE_IMGUI
	};
private:
	SkyMap			sky;
	float			hour = 0.0f;
	Donya::Vector3	drawColor{ 1.0f, 1.0f, 1.0f };
public:
	bool Init();
	void Draw( const Donya::Vector3 &wsCameraPos, const Donya::Vector4x4 &matVP );
public:
	/// <summary>
	/// Accepts value is in 0.0f ~ 24.0f as hour.
	/// </summary>
	void ChangeHour( float hour );
public:
#if USE_IMGUI
	void ShowImGuiNode( const std::string &nodeCaption );
#endif // USE_IMGUI
};
