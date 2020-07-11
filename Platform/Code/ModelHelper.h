#pragma once

#include <memory>
#include <string>
#include <vector>

#include "Donya/Model.h"
#include "Donya/ModelCommon.h"
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
		using Node = Donya::Model::Animation::Node;
		Donya::Model::SkinningModel	model;
		std::vector<Node>			skeletal;	// Represents an initial pose(like a T-pose)
		Donya::Model::MotionHolder	motionHolder;
	};
	class  SkinningOperator
	{
	public:
		std::shared_ptr<ModelHelper::SkinningSet> pResource = nullptr;
		Donya::Model::Pose			pose;
		Donya::Model::Animator		animator;
	public:
		int  GetMotionCount() const;
		/// <summary>
		/// pResource != nullptr &amp;&amp; 0 &lt;= motionIndex &lt; GetMotionCount()
		/// </summary>
		bool IsAssignableIndex( int motionIndex ) const;
		/// <summary>
		/// Assign the specified motion to model.pose. The animator is not change.
		/// </summary>
		void AssignMotion( int motionIndex );
		/// <summary>
		/// Update the animator then call AssignMotion().
		/// </summary>
		void UpdateMotion( float elapsedTime, int motionIndex );
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
