#pragma once

#include <string>

#include "Donya/Model.h"
#include "Donya/ModelMotion.h"
#include "Donya/ModelPose.h"

namespace ModelHelper
{
	struct StaticSet
	{
		Donya::Model::StaticModel	model;
		Donya::Model::Pose			pose;
	};
	struct SkinningSet
	{
		Donya::Model::SkinningModel	model;
		Donya::Model::MotionHolder	motionHolder;
	};

	/// <summary>
	/// Returns true if the load process was succeed.
	/// </summary>
	bool Load( const std::string &filePath, StaticSet *pOut );
	/// <summary>
	/// Returns true if the load process was succeed.
	/// </summary>
	bool Load( const std::string &filePath, SkinningSet *pOut );
}
