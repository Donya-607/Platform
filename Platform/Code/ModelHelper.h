#pragma once

#include <memory>
#include <string>
#include <vector>

#undef max
#undef min
#include <cereal/types/vector.hpp>

#include "Donya/Model.h"
#include "Donya/ModelCommon.h"
#include "Donya/ModelMotion.h"
#include "Donya/ModelPose.h"
#include "Donya/Serializer.h"
#include "Donya/UseImGui.h"		// Use USE_IMGUI macro

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
		void Initialize( const std::shared_ptr<ModelHelper::SkinningSet> &pAssignResource );
	public:
		int  GetMotionCount() const;
		/// <summary>
		/// pResource != nullptr &amp;&amp; 0 &lt;= motionIndex &lt; GetMotionCount()
		/// </summary>
		bool IsAssignableIndex( int motionIndex ) const;
		/// <summary>
		/// Assign the specified motion to model.pose.
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

	struct PartApply
	{
	public:
		std::string				motionName;
		std::vector<std::string>applyRootBoneNames;
		Donya::Vector3			rootTranslationBlendPercent; // [0.0f:Current Motion][1.0f:Destination Motion]
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive
			(
				CEREAL_NVP( motionName					),
				CEREAL_NVP( applyRootBoneNames			),
				CEREAL_NVP( rootTranslationBlendPercent	)
			);

			if ( 1 <= version )
			{
				// archive( CEREAL_NVP( x ) );
			}
		}
	public:
	#if USE_IMGUI
		void ShowImGuiNode( const std::string &nodeCaption );
	#endif // USE_IMGUI
	};
}
CEREAL_CLASS_VERSION( ModelHelper::PartApply, 0 )

