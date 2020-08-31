#pragma once

#include "Donya/Template.h"	// Use Donya::Singleton<>
#include "Donya/ModelCommon.h"
#include "Donya/Vector.h"

class PointLightStorage : public Donya::Singleton<PointLightStorage>
{
	friend Donya::Singleton<PointLightStorage>;
private:
	Donya::Model::Constants::PerScene::PointLightRoom plr;
private:
	PointLightStorage() = default;
public:
	/// <summary>
	/// Clear the registered point lights.
	/// </summary>
	void Clear();
	/// <summary>
	/// Returns false if a space of point light is not there.
	/// </summary>
	bool RegisterIfThereSpace( const Donya::Model::Constants::PerScene::PointLight &light );
public:
	/// <summary>
	/// Returns registered point lights with enable count of lights.
	/// </summary>
	const Donya::Model::Constants::PerScene::PointLightRoom &GetStorage() const { return plr; }
};
