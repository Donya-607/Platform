#pragma once

#include <memory>
#include <string>
#include <vector>

#undef max
#undef min
#include <cereal/types/string.hpp>
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
		struct Interpolation
		{
			static constexpr float defaultTransitionSecond = 1.0f / 4.0f;
		public:
			int					currMotionIndex	= 0;
			float				transPercent	= 1.0f; // 0.0f:Previous ~ 1.0f:Current
			float				transSecond		= defaultTransitionSecond;
			Donya::Model::Pose	prevPose;
			Donya::Model::Pose	lerpedPose;
		};
	public:
		std::shared_ptr<ModelHelper::SkinningSet> pResource = nullptr;
		Donya::Model::Pose			pose;
		Donya::Model::Animator		animator;
		Interpolation				interpolation;
	public:
		void Initialize( const std::shared_ptr<ModelHelper::SkinningSet> &pAssignResource );
	public:
		void SetInterpolationSecond( float takingSecond );
		Donya::Model::Pose &GetCurrentPose();
		const Donya::Model::Pose &GetCurrentPose() const;
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
		/// And advance motion interpolation if now transition-ing.
		/// </summary>
		void UpdateMotion( float elapsedTime, int motionIndex );
	public:
		/// <summary>
		/// Only update the interpolate process.
		/// The UpdateMotion() is also calls this, so you have not need to call it if you call UpdateMotion().
		/// </summary>
		void AdvanceInterpolation( float elapsedTime );
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


namespace Donya
{
namespace Model
{
namespace Constants
{
namespace PerScene
{
	template<class Archive>
	void serialize( Archive &archive, Light &var, std::uint32_t version )
	{
		archive
		(
			cereal::make_nvp( "diffuseColor",	var.diffuseColor	),
			cereal::make_nvp( "specularColor",	var.specularColor	)
		);

		if ( 1 <= version )
		{
			archive( cereal::make_nvp( "ambientColor", var.ambientColor ) );
		}
		if ( 2 <= version )
		{
			// archive( CEREAL_NVP( x ) );
		}
	}
	template<class Archive>
	void serialize( Archive &archive, DirectionalLight &var, std::uint32_t version )
	{
		archive
		(
			cereal::make_nvp( "light",		var.light		),
			cereal::make_nvp( "direction",	var.direction	)
		);

		if ( 1 <= version )
		{
			// archive( CEREAL_NVP( x ) );
		}
	}
	template<class Archive>
	void serialize( Archive &archive, PointLight &var, std::uint32_t version )
	{
		archive
		(
			cereal::make_nvp( "light",			var.light		),
			cereal::make_nvp( "wsPos",			var.wsPos		),
			cereal::make_nvp( "attenuation",	var.attenuation	),
			cereal::make_nvp( "range",			var.range		)
		);

		if ( 1 <= version )
		{
			// archive( CEREAL_NVP( x ) );
		}
	}
}
}
}
}
CEREAL_CLASS_VERSION( Donya::Model::Constants::PerScene::Light,				1 )
CEREAL_CLASS_VERSION( Donya::Model::Constants::PerScene::DirectionalLight,	0 )
CEREAL_CLASS_VERSION( Donya::Model::Constants::PerScene::PointLight,		0 )
