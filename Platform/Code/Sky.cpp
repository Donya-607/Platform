#include "Sky.h"

#include "Donya/Quaternion.h"
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
	ImGui::SliderFloat3	( u8"��]�p�xs (degree)", &degrees.x, -180.0f, 180.0f );
	radians.x = ToRadian( degrees.x );
	radians.y = ToRadian( degrees.y );
	radians.z = ToRadian( degrees.z );

	ImGui::DragFloat3	( u8"�X�P�[��", &scale.x, 0.01f );

	ImGui::SliderFloat3	( u8"�F�F����l", &upperLimit.x, 0.0f, 1.0f );
	ImGui::SliderFloat3	( u8"�F�F�����l", &lowerLimit.x, 0.0f, 1.0f );
	ImGui::DragFloat3	( u8"�F�F�����l", &magnification.x, 0.01f );
}
#endif // USE_IMGUI

bool Sky::Init()
{
	parameter.LoadParameter();

	lerpTimer		= -1.0f;
	lerpSecond		= 0.0f;
	startHour		= defaultHour;
	destinationHour	= defaultHour;
	drawColor		= CalcCurrentColor( defaultHour );
	return sky.Init();
}
void Sky::Update( float elapsedTime )
{
	if ( lerpTimer <= 0.0f ) { return; }
	// else

	lerpTimer -= elapsedTime;
	if ( lerpTimer <= 0.0f )
	{
		lerpTimer = -1.0f;
		startHour = destinationHour;
		drawColor = CalcCurrentColor( destinationHour );
		return;
	}
	// else

	const float percent		= 1.0f - ( lerpTimer / lerpSecond );
	const float currentHour = Donya::Lerp( startHour, destinationHour, percent );
	drawColor = CalcCurrentColor( currentHour );
}
void Sky::Draw( const Donya::Vector3 &cameraPos, const Donya::Vector4x4 &VP )
{
	const auto &data = GetParameter();

	const Donya::Quaternion rotation = Donya::Quaternion::Make( data.radians.x, data.radians.y, data.radians.z );
	const Donya::Vector4x4 W = Donya::Vector4x4::MakeTransformation( data.scale, rotation, cameraPos );
	
	sky.UpdateConstant( W * VP, Donya::Vector4{ drawColor, 1.0f } );
	sky.Draw();
}

void Sky::AdvanceHourTo( float dest, float second )
{
	second = std::max( 0.0f, second );

	while ( 24.0f < dest ) { dest -= 24.0f; }
	while ( dest <  0.0f ) { dest += 24.0f; }

	if ( IsZero( second ) )
	{
		lerpTimer		= -1.0f;
		lerpSecond		= 0.0f;
		startHour		= dest;
		destinationHour	= dest;
		drawColor		= CalcCurrentColor( destinationHour );
		return;
	}
	// else

	lerpTimer		= second;
	lerpSecond		= second;

	startHour		= destinationHour;
	destinationHour	= dest;
}

Donya::Vector3 Sky::CalcCurrentColor( float hour ) const
{
	constexpr float timeToRad = 1.0f / 24.0f * ToRadian( 360.0f );
	const float timeRadian = hour * timeToRad;
	const float cos = cosf( timeRadian );

	const auto &data = GetParameter();

	Donya::Vector3 color;
	for ( int i = 0; i < 3; ++i )
	{
		color[i] = Donya::Clamp( data.magnification[i] * -cos, data.lowerLimit[i], data.upperLimit[i] );
	}
	return color;
}

#if USE_IMGUI
void Sky::ShowImGuiNode( const std::string &nodeCaption )
{
	if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
	// else

	ImGui::ColorEdit3	( u8"���݂̐F",		&drawColor.x );

	ImGui::SliderFloat	( u8"�n�_����",		&startHour,			0.0f, 24.0f );
	ImGui::SliderFloat	( u8"�I�_����",		&destinationHour,	0.0f, 24.0f );
	ImGui::DragFloat	( u8"��ԕb��",		&lerpSecond,		0.01f );
	ImGui::DragFloat	( u8"��Ԏc��b��",	&lerpTimer,			0.01f );

	static bool alwaysAdvance = true;
	ImGui::Checkbox( u8"��ɏI�_���Ԃ֕ύX����", &alwaysAdvance );
	if ( alwaysAdvance || ImGui::Button( u8"��ԊJ�n" ) )
	{
		AdvanceHourTo( destinationHour, 0.0f );
	}

	parameter.ShowImGuiNode( u8"�p�����[�^����" );

	ImGui::TreePop();
}
#endif // USE_IMGUI
