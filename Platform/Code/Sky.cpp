#include "Sky.h"

#include "Donya/Template.h"	// Use Donya::Clamp()
#include "Donya/Useful.h"

#include "FilePath.h"
#include "Parameter.h"

namespace
{
	static ParamOperator<Sky::Parameter> parameter{ "Sky" };
	const Sky::Parameter &GetParameter()
	{
		return parameter.Get();
	}
}

#if USE_IMGUI
void Sky::Parameter::ShowImGuiNode()
{
	Donya::Vector3 degrees
	{
		ToDegree( radians.x ),
		ToDegree( radians.y ),
		ToDegree( radians.z ),
	};
	ImGui::SliderFloat3	( u8"回転角度s (degree)", &degrees.x, -180.0f, 180.0f );
	radians.x = ToRadian( degrees.x );
	radians.y = ToRadian( degrees.y );
	radians.z = ToRadian( degrees.z );

	ImGui::DragFloat3	( u8"スケール", &scale.x, 0.01f );

	ImGui::SliderFloat3	( u8"色：上限値", &upperLimit.x, 0.0f, 1.0f );
	ImGui::SliderFloat3	( u8"色：下限値", &lowerLimit.x, 0.0f, 1.0f );
	ImGui::DragFloat3	( u8"色：増幅値", &magnification.x, 0.01f );
}
#endif // USE_IMGUI

bool Sky::Init()
{
	parameter.LoadParameter();

	hour		= 0.0f;
	drawColor	= { 1.0f, 1.0f, 1.0f };
	return sky.Init();
}

void Sky::Draw( const Donya::Vector3 &cameraPos, const Donya::Vector4x4 &VP )
{
	const auto &data = GetParameter();

	const Donya::Quaternion rotation = Donya::Quaternion::Make( data.radians.x, data.radians.y, data.radians.z );
	const Donya::Vector4x4 W = Donya::Vector4x4::MakeTransformation( data.scale, rotation, cameraPos );
	
	sky.UpdateConstant( W * VP, Donya::Vector4{ drawColor, 1.0f } );
	sky.Draw();
}

void Sky::ChangeHour( float argHour )
{
	while ( 24.0f < argHour ) { argHour -= 24.0f; }
	while ( argHour <  0.0f ) { argHour += 24.0f; }

	hour = argHour;

	constexpr float timeToRad = 1.0f / 24.0f * ToRadian( 360.0f );
	const float timeRadian = hour * timeToRad;
	const float cos = cosf( timeRadian );

	const auto &data = GetParameter();
	for ( int i = 0; i < 3; ++i )
	{
		drawColor[i] = Donya::Clamp( data.magnification[i] * -cos, data.lowerLimit[i], data.upperLimit[i] );
	}
}

#if USE_IMGUI
void Sky::ShowImGuiNode( const std::string &nodeCaption )
{
	if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
	// else

	ImGui::SliderFloat( u8"現在の時間",	&hour, 0.0f, 24.0f );
	ImGui::ColorEdit3 ( u8"現在の色",	&drawColor.x );

	parameter.ShowImGuiNode( u8"パラメータ調整" );

	ImGui::TreePop();
}
#endif // USE_IMGUI
