#include "PointLightStorage.h"

using namespace Donya::Model::Constants::PerScene;

void PointLightStorage::Clear()
{
	plr.enableLightCount = 0;
	// The data which places to behind from plr.enableLightCount will be ignored.
	// So some reset process of lights is unnecessary.
}
bool PointLightStorage::RegisterIfThereSpace( const PointLight &light )
{
	if ( PointLight::MAX_POINT_COUNT <= plr.enableLightCount ) { return false; }
	// else

	plr.lights[plr.enableLightCount] = light;
	plr.enableLightCount++;

	return true;
}
