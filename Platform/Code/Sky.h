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
	static constexpr float defaultHour = 12.0f;
private:
	SkyMap			sky;
	float			lerpTimer		= -1.0f;	// Second. The amount of remaining second.
	float			lerpSecond		= 0.0f;		// Second. Store the specified second
	float			startHour		= defaultHour;
	float			destinationHour	= defaultHour;
	Donya::Vector3	drawColor{ 1.0f, 1.0f, 1.0f };
public:
	bool Init();
	void Update( float elapsedTime );
	void Draw( const Donya::Vector3 &wsCameraPos, const Donya::Vector4x4 &matVP );
public:
	/// <summary>
	/// Accepts hour is in 0.0f ~ 24.0f.
	/// </summary>
	void AdvanceHourTo( float destinationHour, float takeSecond );
private:
	Donya::Vector3 CalcCurrentColor( float hour ) const;
public:
#if USE_IMGUI
	void ShowImGuiNode( const std::string &nodeCaption );
#endif // USE_IMGUI
};
