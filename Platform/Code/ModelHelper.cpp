#include "ModelHelper.h"

#include "Donya/Loader.h"

namespace ModelHelper
{
	void SkinningOperator::Initialize( const std::shared_ptr<ModelHelper::SkinningSet> &pAssignResource )
	{
		pResource = pAssignResource;
		if ( pResource )
		{
			pose.AssignSkeletal( pResource->skeletal );
			animator.ResetTimer();
		}

		interpolation.currMotionIndex	= 0;
		interpolation.transPercent		= 1.0f;
		interpolation.transSecond		= Interpolation::defaultTransitionSecond;
		interpolation.prevPose			= pose;
		interpolation.lerpedPose		= pose;
	}
	void SkinningOperator::SetInterpolationSecond( float takingSecond )
	{
		interpolation.transSecond = takingSecond;
	}
	int  SkinningOperator::GetMotionCount() const
	{
		return ( !pResource ) ? 0 : scast<int>( pResource->motionHolder.GetMotionCount() );
	}
	bool SkinningOperator::IsAssignableIndex( int motionIndex ) const
	{
		if ( !pResource ) { return false; }
		return ( 0 <= motionIndex && motionIndex < GetMotionCount() );
	}
	void SkinningOperator::AssignMotion( int motionIndex )
	{
		if ( !pResource ) { return; }
		// else

		if ( motionIndex < 0 || GetMotionCount() <= motionIndex )
		{
			_ASSERT_EXPR( 0, L"Error: Passed motion index is out of range!" );
			return;
		}
		// else

		if ( motionIndex != interpolation.currMotionIndex )
		{
			interpolation.currMotionIndex	= motionIndex;
			interpolation.transPercent		= 0.0f;
			interpolation.prevPose			= interpolation.lerpedPose;
		}

		const auto &motion = pResource->motionHolder.GetMotion( motionIndex );

		animator.SetRepeatRange( motion );
		pose.AssignSkeletal( animator.CalcCurrentPose( motion ) );
	}
	void SkinningOperator::UpdateMotion( float elapsedTime, int motionIndex )
	{
		if ( !pResource ) { return; }
		// else

		animator.Update( elapsedTime );
		AssignMotion( motionIndex );

		UpdateInterpolation( elapsedTime );
	}
	void SkinningOperator::UpdateInterpolation( float elapsedTime )
	{
		Interpolation &lerp = interpolation;

		if ( 1.0f <= lerp.transPercent ) { return; }
		// else

		// Finish immediately
		if ( IsZero( lerp.transSecond ) || lerp.transSecond < 0.0f )
		{
			lerp.transPercent	= 1.0f;
			lerp.lerpedPose		= pose;
			return;
		}
		// else

		const float updateSecond = 1.0f / lerp.transSecond;
		lerp.transPercent += updateSecond * elapsedTime;

		if ( 1.0f <= lerp.transPercent )
		{
			lerp.lerpedPose = pose;
			return;
		}
		// else

		lerp.lerpedPose = Donya::Model::Pose::Interpolate( lerp.prevPose, pose, lerp.transPercent );
	}

	bool Load( const std::string &filePath, StaticSet *pOut )
	{
		if ( !pOut ) { return false; }
		if ( !Donya::IsExistFile( filePath ) ) { return false; }
		// else

		Donya::Loader loader{};
		if ( !loader.Load( filePath ) ) { return false; }
		// else

		const auto &source = loader.GetModelSource();
		pOut->model = Donya::Model::StaticModel::Create( source, loader.GetFileDirectory() );
		pOut->pose.AssignSkeletal( source.skeletal );

		return pOut->model.WasInitializeSucceeded();
	}
	bool Load( const std::string &filePath, SkinningSet *pOut )
	{
		if ( !pOut ) { return false; }
		if ( !Donya::IsExistFile( filePath ) ) { return false; }
		// else

		Donya::Loader loader{};
		if ( !loader.Load( filePath ) ) { return false; }
		// else

		const auto &source	= loader.GetModelSource();
		pOut->model			= Donya::Model::SkinningModel::Create( source, loader.GetFileDirectory() );
		pOut->skeletal		= source.skeletal;
		pOut->motionHolder.AppendSource( source );

		return pOut->model.WasInitializeSucceeded();
	}

#if USE_IMGUI
	void PartApply::ShowImGuiNode( const std::string &nodeCaption )
	{
		if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
		// else

		if ( ImGui::TreeNode( u8"適用ボーンの設定" ) )
		{
			ImGui::Helper::ResizeByButton( &applyRootBoneNames );

			const size_t count = applyRootBoneNames.size();
			for ( size_t i = 0; i < count; ++i )
			{
				ImGui::Helper::ShowStringNode
				(
					u8"ルートボーン名" + Donya::MakeArraySuffix( i ),
					nodeCaption + u8"ApplyBoneName" + std::to_string( i ),
					&applyRootBoneNames[i]
				);
			}

			ImGui::TreePop();
		}

		ImGui::Helper::ShowStringNode( u8"モーション名", nodeCaption + "MotionName", &motionName );

		ImGui::SliderFloat3( u8"平行移動に使用する割合", &rootTranslationBlendPercent.x, 0.0f, 1.0f );
		ImGui::Text( u8"現在モーション[0.0f] <- -> [1.0f]遷移先モーション" );

		ImGui::TreePop();
	}
#endif // USE_IMGUI
}
