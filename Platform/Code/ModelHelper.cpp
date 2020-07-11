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

		const auto &motion = pResource->motionHolder.GetMotion( motionIndex );
		pose.AssignSkeletal( animator.CalcCurrentPose( motion ) );

		animator.SetRepeatRange( motion );
	}
	void SkinningOperator::UpdateMotion( float elapsedTime, int motionIndex )
	{
		if ( !pResource ) { return; }
		// else

		animator.Update( elapsedTime );
		AssignMotion( motionIndex );
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
}
