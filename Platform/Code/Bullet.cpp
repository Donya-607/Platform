#include "Bullet.h"

#include <algorithm>			// Use std::remove_if

#include "Donya/Sound.h"

#include "BulletParam.h"
#include "Bullets/Buster.h"
#include "Bullets/SkullBullet.h"
#include "Bullets/SuperBall.h"
#include "Bullets/Bone.h"
#include "Bullets/TogeheroBody.h"
#include "Common.h"				// Use IsShowCollision()
#include "Effect/EffectAdmin.h"
#include "Effect/EffectKind.h"
#include "FilePath.h"
#include "ModelHelper.h"
#include "Music.h"
#include "Parameter.h"

namespace Bullet
{
	namespace
	{
		constexpr size_t kindCount = scast<size_t>( Kind::KindCount );
		constexpr const char *modelFolderName = "Bullet/";
		constexpr std::array<const char *, kindCount> modelNames
		{
			"Buster",
			"SkullBuster",
			"SkullShield",
			"SuperBall",
			"Bone",
			"Togehero",
		};

		static std::array<std::shared_ptr<ModelHelper::SkinningSet>, kindCount> modelPtrs{ nullptr };

		bool LoadModels()
		{
			const std::string folderName = modelFolderName;
			bool succeeded = true;
			std::string filePath{};
			for ( size_t i = 0; i < kindCount; ++i )
			{
				if ( modelPtrs[i] ) { continue; }
				// else

				filePath = MakeModelPath( folderName + modelNames[i] );

				if ( !Donya::IsExistFile( filePath ) )
				{
					const std::string msg = "Error: File is not found: " + filePath + "\n";
					Donya::OutputDebugStr( msg.c_str() );
					succeeded = false;
					continue;
				}
				// else

				modelPtrs[i] = std::make_shared<ModelHelper::SkinningSet>();
				const bool result = ModelHelper::Load( filePath, modelPtrs[i].get() );
				if ( !result )
				{
					modelPtrs[i].reset(); // Make not loaded state

					const std::string msg = "Failed: Loading failed: " + filePath;
					Donya::OutputDebugStr( msg.c_str() );
					succeeded = false;
					continue;
				}
				// else
			}

			return succeeded;
		}
		constexpr bool IsOutOfRange( Kind kind )
		{
			return ( kindCount <= scast<size_t>( kind ) );
		}
		std::shared_ptr<ModelHelper::SkinningSet> GetModelPtrOrNullptr( Kind kind )
		{
			if ( IsOutOfRange( kind ) ) { return nullptr; }
			// else

			const auto &ptr = modelPtrs[scast<size_t>( kind )];
			if ( !ptr )
			{
				_ASSERT_EXPR( 0, L"Error: The Bullet's model is not initialized!" );
				return nullptr;
			}
			// else

			return ptr;
		}
		constexpr const char *GetModelName( Kind kind )
		{
			if ( IsOutOfRange( kind ) ) { return nullptr; }
			// else

			return modelNames[scast<size_t>( kind )];
		}
	}

	namespace Parameter
	{
		void Load()
		{
			Impl::LoadBuster();
			Impl::LoadGeneral();
			Impl::LoadSkullBuster();
			Impl::LoadSkullShield();
			Impl::LoadSuperBall();
			Impl::LoadBone();
			Impl::LoadTogeheroBody();
		}

	#if USE_IMGUI
		void Update( const std::string &nodeCaption )
		{
			if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
			// else

			Impl::UpdateGeneral		( u8"General" );
			Impl::UpdateBuster		( u8"Buster" );
			Impl::UpdateSkullBuster	( u8"SkullBuster" );
			Impl::UpdateSkullShield	( u8"SkullShield" );
			Impl::UpdateSuperBall	( u8"SuperBall" );
			Impl::UpdateBone		( u8"Bone" );
			Impl::UpdateTogeheroBody( u8"TogeheroBody" );

			ImGui::TreePop();
		}
	#endif // USE_IMGUI

		static ParamOperator<GeneralParam> generalParam{ "General", "Bullet/" };
		const GeneralParam &GetGeneral()
		{
			return generalParam.Get();
		}
		namespace Impl
		{
			void LoadGeneral()
			{
				generalParam.LoadParameter();
			}
		#if USE_IMGUI
			void UpdateGeneral( const std::string &nodeCaption )
			{
				generalParam.ShowImGuiNode( nodeCaption );
			}
		#endif // USE_IMGUI
		}
	}

	bool LoadResource()
	{
		Parameter::Load();
		return LoadModels();
	}

#if USE_IMGUI
	constexpr const char *GetKindName( Kind kind )
	{
		switch ( kind )
		{
		case Kind::Buster:		return "Buster";
		case Kind::SkullBuster:	return "SkullBuster";
		case Kind::SkullShield:	return "SkullShield";
		case Kind::SuperBall:	return "SuperBall";
		case Kind::Bone:		return "Bone";
		default: break;
		}

		_ASSERT_EXPR( 0, L"Error: Unexpected kind!" );
		return "ERROR_KIND";
	}
	void ShowKindNode( const std::string &nodeCaption, Kind *p )
	{
		if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
		// else

		for ( size_t i = 0; i < kindCount; ++i )
		{
			const Kind current = scast<Kind>( i );
			if ( ImGui::RadioButton( GetKindName( current ), *p == current ) )
			{
				*p = current;
			}
		}

		ImGui::TreePop();
	}
	void FireDesc::ShowImGuiNode( const std::string &nodeCaption, bool isRelativePos )
	{
		if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
		// else

		ShowKindNode( u8"����������", &kind );

		ImGui::DragFloat( u8"����[m/s]", &initialSpeed, 0.1f );
		initialSpeed = std::max( 0.0f, initialSpeed );

		ImGui::SliderFloat3( u8"����", &direction.x, -1.0f, 1.0f );
		if ( ImGui::Button( u8"�����𐳋K��" ) )
		{
			direction.Normalize();
		}

		const char *caption = ( isRelativePos ) ? u8"�����ʒu�i���[�J���E���΁j" : u8"���[���h���W";
		ImGui::DragFloat3( caption, &position.x, 0.1f );

		bool addDamage = ( !pAdditionalDamage ) ? false : true;
		ImGui::Checkbox( u8"�_���[�W��ǉ����邩", &addDamage );
		if ( addDamage )
		{
			if ( !pAdditionalDamage )
			{
				pAdditionalDamage = std::make_shared<Definition::Damage>();
			}

			ImGui::Text( u8"��������e�̎��_���[�W�E�������ɍ��Z���܂�" );
			pAdditionalDamage->ShowImGuiNode( u8"���Z�_���[�W�ݒ�" );
		}
		else
		{
			pAdditionalDamage.reset();
			ImGui::TextDisabled( u8"���Z�_���[�W�ݒ�" );
		}

		ImGui::TreePop();
	}

	void GeneralParam::ShowImGuiNode()
	{
		ImGui::DragFloat( u8"���ˌ�̑��x",		&reflectSpeed,  0.01f );
		ImGui::DragFloat( u8"���ˊp�x(degree)",	&reflectDegree, 1.0f  );
		reflectSpeed  = std::max( 0.1f, reflectSpeed  );
	}
	void BasicParam::ShowImGuiNode( const std::string &nodeCaption )
	{
		if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
		// else

		damage.ShowImGuiNode( u8"��{�_���[�W�ݒ�" );
		ImGui::DragFloat ( u8"���[�V�����Đ����x",					&animePlaySpeed,	0.01f );
		ImGui::DragFloat3( u8"�����蔻��E�I�t�Z�b�g",				&hitBoxOffset.x,	0.01f );
		ImGui::DragFloat3( u8"�����蔻��EAABB�T�C�Y�i�������w��j",	&hitBoxSize.x,		0.01f );
		ImGui::DragFloat ( u8"�����蔻��ESphere���a",				&hitBoxSize.x,		0.01f );
		ImGui::Text( u8"�iSphere�̏ꍇ�͂w���������a�Ƃ��Ďg�p����܂��j" );
		hitBoxSize.x = std::max( 0.0f, hitBoxSize.x );
		hitBoxSize.y = std::max( 0.0f, hitBoxSize.y );
		hitBoxSize.z = std::max( 0.0f, hitBoxSize.z );

		ImGui::TreePop();
	}
#endif // USE_IMGUI

	void Base::Init( const FireDesc &parameter )
	{
		model.Initialize( GetModelPtrOrNullptr( GetKind() ) );
		model.AssignMotion( 0 );
		
		InitBody( parameter );

		velocity	= parameter.direction * parameter.initialSpeed;
		UpdateOrientation( parameter.direction );
		damage		= GetDamageParameter();
		if ( parameter.pAdditionalDamage )
		{
			damage.Combine( *parameter.pAdditionalDamage );
		}
		wantRemove	= false;
	}
	void Base::Update( float elapsedTime, const Donya::Collision::Box3F &wsScreen )
	{
	#if USE_IMGUI
		// Apply for be able to see an adjustment immediately
		{
			const bool useAABB		= !hitSphere.exist;
			const bool useSphere	= !body.exist;
			if ( !useAABB || !useSphere ) // Except both true
			{
				if ( useAABB	) { AssignBodyParameter( body.pos		); }
				if ( useSphere	) { AssignBodyParameter( hitSphere.pos	); }
			}
		}
	#endif // USE_IMGUI

		body.UpdateIgnoreList( elapsedTime );
		hitSphere.UpdateIgnoreList( elapsedTime );
		collidedCallingCount = 0;

		secondToRemove -= elapsedTime;
		if ( secondToRemove <= 0.0f )
		{
			wantRemove = true;
		}

		if ( wasCollided )
		{
			CollidedProcess();
		}
		if ( wasProtected != ProtectedInfo::None && wasProtected != ProtectedInfo::Processed )
		{
			ProtectedProcess();
		}

		if ( removeIfOutScreen && OnOutSide( wsScreen ) )
		{
			wantRemove = true;
		}
	}
	void Base::PhysicUpdate( float elapsedTime, const Map &terrain )
	{
		const auto oldPos	= body.pos;
		const auto movement	= velocity * elapsedTime;
		Solid::Move( movement, {}, {} ); // It moves the "body" only

		const auto delta = body.pos - oldPos;

		// Move() moves only the "body", so apply here.
		hitSphere.pos += delta;
	}
	void Base::Draw( RenderingHelper *pRenderer ) const
	{
		if ( !pRenderer			) { return; }
		if ( !model.pResource	) { return; }
		// else

		const bool useAABB = !hitSphere.exist;
		// I wanna adjust the hit-box to fit for drawing model, so I don't apply the offset for the position of drawing model.
		const Donya::Vector3   &drawPos	= ( useAABB ) ? body.pos : hitSphere.pos;
		const Donya::Vector4x4 W		= MakeWorldMatrix( 1.0f, /* enableRotation = */ true, drawPos );

		Donya::Model::Constants::PerModel::Common modelConstant{};
		modelConstant.drawColor			= Donya::Vector4{ 1.0f, 1.0f, 1.0f, 1.0f };
		modelConstant.worldMatrix		= W;
		pRenderer->UpdateConstant( modelConstant );
		pRenderer->ActivateConstantModel();

		pRenderer->Render( model.pResource->model, model.pose );

		pRenderer->DeactivateConstantModel();
	}
	void Base::DrawHitBox( RenderingHelper *pRenderer, const Donya::Vector4x4 &VP ) const
	{
		if ( !Common::IsShowCollision() || !pRenderer ) { return; }
		// else

	#if DEBUG_MODE
		constexpr Donya::Vector4 color{ 1.0f, 1.0f, 1.0f, 0.6f };
		constexpr Donya::Vector3 lightDir = -Donya::Vector3::Up();

		const bool useAABB		= !hitSphere.exist;
		const bool useSphere	= !body.exist;
		if ( !useAABB && !useSphere ) { return; }
		// else

		const Donya::Vector3 size	=
									( useAABB	) ? body.size * 2.0f :
									( useSphere	) ? hitSphere.radius * 2.0f :
									0.0f;
		const Donya::Vector3 wsPos	=
									( useAABB	) ? body.WorldPosition() :
									( useSphere	) ? hitSphere.WorldPosition() :
									0.0f;
		const Donya::Vector4x4 W	= MakeWorldMatrix( size, /* enableRotation = */ false, wsPos );

		if ( useAABB )
		{
			Donya::Model::Cube::Constant constant;
			constant.matWorld			= W;
			constant.matViewProj		= VP;
			constant.drawColor			= color;
			constant.lightDirection		= lightDir;
			pRenderer->ProcessDrawingCube( constant );
		}
		else
		{
			Donya::Model::Sphere::Constant constant;
			constant.matWorld			= W;
			constant.matViewProj		= VP;
			constant.drawColor			= color;
			constant.lightDirection		= lightDir;
			pRenderer->ProcessDrawingSphere( constant );
		}
	#endif // DEBUG_MODE
	}
	bool Base::Destructible() const
	{
		return false; // a derived class may returns true
	}
	bool Base::ShouldRemove() const
	{
		return wantRemove;
	}
	bool Base::WasProtected() const
	{
		return ( wasProtected != ProtectedInfo::None ) ? true : false;
	}
	bool Base::OnOutSide( const Donya::Collision::Box3F &wsScreen ) const
	{
		// I am not consider an unused hit box(size or radius is zero) in here,
		// so I make to true an invalid one certainly.
		const bool outAABB		= ( body.size.IsZero()			|| !Donya::Collision::IsHit( body,		wsScreen, /* considerExistFlag = */ false ) );
		const bool outSphere	= ( IsZero( hitSphere.radius )	|| !Donya::Collision::IsHit( hitSphere,	wsScreen, /* considerExistFlag = */ false ) );
		return outAABB & outSphere; // Which one is true certainly, so I should combine it.
	}
	void Base::CollidedToObject( bool otherIsBroken ) const
	{
		// Play a SE and an effect only first time. Because it may called multiple times in the same frame
		collidedCallingCount++;
		if ( collidedCallingCount == 1 )
		{
			GenerateCollidedEffect();
			PlayCollidedSE();
		}

		const bool isSolid = Definition::Damage::Contain( Definition::Damage::Type::ForcePierce, damage.type );
		if ( isSolid ) { return; }
		// else

		const bool pierceable = Definition::Damage::Contain( Definition::Damage::Type::Pierce, damage.type );
		if ( otherIsBroken && pierceable ) { return; }
		// else

		wasCollided = true;
	}
	void Base::ProtectedBy( const Donya::Collision::Box3F &otherBody ) const
	{
		const auto myCenter		= ( hitSphere.exist ) ? hitSphere.WorldPosition() : body.WorldPosition();
		const auto otherMax		= otherBody.Max();
		const auto otherMin		= otherBody.Min();
		const float distLeft	= otherMin.x - myCenter.x;
		const float distRight	= otherMax.x - myCenter.x;
		ProtectedByImpl( distLeft, distRight );
	}
	void Base::ProtectedBy( const Donya::Collision::Sphere3F &otherBody ) const
	{
		const auto myCenter		= ( hitSphere.exist ) ? hitSphere.WorldPosition() : body.WorldPosition();
		const auto otherCenter	= otherBody.WorldPosition();
		const auto otherRight	= otherCenter.x + otherBody.radius;
		const auto otherLeft	= otherCenter.x - otherBody.radius;
		const float distLeft	= otherLeft  - myCenter.x;
		const float distRight	= otherRight - myCenter.x;
		ProtectedByImpl( distLeft, distRight );
	}
	void Base::ProtectedByImpl( float distLeft, float distRight ) const
	{
		if ( wasProtected == ProtectedInfo::Processed ) { return; }
		// else

		bool fromRightSide{};
		if ( 0.0f <= distLeft  ) { fromRightSide = true;  }
		else
		if ( distRight <= 0.0f ) { fromRightSide = false; }
		else
		{
			/*
			|	<-distLeft->	|	<-distRight->	|
			Min				myCenter				Max
			*/
			fromRightSide = ( fabsf( distLeft ) <= fabsf( distRight ) );
		}

		wasProtected	= ( fromRightSide )
						? ProtectedInfo::ByRightSide
						: ProtectedInfo::ByLeftSide;
	}
	Donya::Collision::Sphere3F	Base::GetHitSphere()			const
	{
		return hitSphere;
	}
	Donya::Collision::Box3F		Base::GetHitBoxSubtractor()		const
	{
		return Donya::Collision::Box3F::Nil(); // Usually do not use
	}
	Donya::Collision::Sphere3F	Base::GetHitSphereSubtractor()	const
	{
		return Donya::Collision::Sphere3F::Nil(); // Usually do not use
	}
	Definition::Damage Base::GetDamage() const
	{
		return damage;
	}
	void Base::SetWorldPosition( const Donya::Vector3 &wsPos )
	{
		hitSphere.pos = body.pos = wsPos;
	}
	void Base::SetVelocity( const Donya::Vector3 &newVelocity )
	{
		velocity = newVelocity;
	}
	void Base::SetLifeTime( float second )
	{
		secondToRemove = second;
	}
	void Base::AllowRemovingByOutOfScreen()
	{
		removeIfOutScreen = true;
	}
	void Base::DisallowRemovingByOutOfScreen()
	{
		removeIfOutScreen = false;
	}
	void Base::GenerateProtectedEffect() const
	{
		Effect::Admin::Get().GenerateInstance( Effect::Kind::Protected, GetPosition() );
	}
	void Base::PlayProtectedSE() const
	{
		Donya::Sound::Play( Music::Bullet_Protected );
	}
	void Base::CollidedProcess()
	{
		collidedCallingCount	= 0;
		wasCollided				= false;
		wantRemove				= true;
		body.exist				= false;
		hitSphere.exist			= false;
	}
	void Base::ProtectedProcess()
	{
		wasCollided		= false;
		body.exist		= false;
		hitSphere.exist	= false;

		const auto &data		= Parameter::GetGeneral();
		const bool fromRight	= wasProtected == ProtectedInfo::ByRightSide;
		const float directionRadian	= ( fromRight )
									? ToRadian( 180.0f - data.reflectDegree )
									: ToRadian( data.reflectDegree );
		const float cos = cosf( directionRadian );
		const float sin = sinf( directionRadian );

		velocity.x = cos * data.reflectSpeed;
		velocity.y = sin * data.reflectSpeed;
		velocity.z = 0.0f;
		// The orientation is not change

		wasProtected	= ProtectedInfo::Processed;

		SetLifeTime( FLT_MAX );
		AllowRemovingByOutOfScreen();

		GenerateProtectedEffect();
		PlayProtectedSE();
	}
	void Base::InitBody( const FireDesc &parameter )
	{
		// Default hit box is used as AABB
		body.exist			= true;
		hitSphere.radius	= 0.0f;
		hitSphere.exist		= false;
		AssignBodyParameter( parameter.position );
		body.id				= Donya::Collision::GetUniqueID();
		body.ownerID		= parameter.owner;
		body.ignoreList.clear();
		hitSphere.id		= body.id;
		hitSphere.ownerID	= body.ownerID;
		hitSphere.ignoreList.clear();
	}
	void Base::UpdateOrientation( const Donya::Vector3 &direction )
	{
		if ( direction.IsZero() ) { return; }
		// else
		orientation = Donya::Quaternion::LookAt( Donya::Vector3::Front(), direction );
		
		const bool useAABB		= !hitSphere.exist;
		const bool useSphere	= !body.exist;
		if ( useAABB	) { AssignBodyParameter( body.pos		); }
		if ( useSphere	) { AssignBodyParameter( hitSphere.pos	); }
	}
	void Base::UpdateMotionIfCan( float elapsedTime, int motionIndex )
	{
		if ( wasProtected != ProtectedInfo::None ) { return; }
		// else
		if ( !model.IsAssignableIndex( motionIndex ) ) { return; }
		// else

		model.UpdateMotion( elapsedTime, motionIndex );
	}
	Donya::Vector4x4 Base::MakeWorldMatrix( const Donya::Vector3 &scale, bool enableRotation, const Donya::Vector3 &translation ) const
	{
		Donya::Vector4x4 W{};
		W._11 = scale.x;
		W._22 = scale.y;
		W._33 = scale.z;
		if ( enableRotation ) { W *= orientation.MakeRotationMatrix(); }
		W._41 = translation.x;
		W._42 = translation.y;
		W._43 = translation.z;
		return W;
	}
#if USE_IMGUI
	void Base::ShowImGuiNode( const std::string &nodeCaption )
	{
		if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
		// else

		if ( ImGui::Button( ( nodeCaption + u8"���폜" ).c_str() ) )
		{
			wantRemove = true;
		}

		ImGui::Helper::ShowAABBNode ( u8"��", &body );
		ImGui::DragFloat3( u8"���x", &velocity.x, 0.1f );
		ImGui::Helper::ShowFrontNode( "", &orientation, /* freezeUpAxis = */ false );

		ImGui::TreePop();
	}
#endif // USE_IMGUI


	void Admin::Update( float elapsedTime, const Donya::Collision::Box3F &wsScreen )
	{
		GenerateRequestedFires();
		generateRequests.clear();
		copyRequests.clear();

		for ( auto &pIt : bulletPtrs )
		{
			if ( !pIt ) { continue; }
			// else

			pIt->Update( elapsedTime, wsScreen );

			if ( pIt->ShouldRemove() )
			{
				pIt->Uninit(); // It will be removed at RemoveInstancesIfNeeds(), so we should finalize here.
			}
		}

		RemoveInstancesIfNeeds();
	}
	void Admin::PhysicUpdate( float elapsedTime, const Map &terrain )
	{
		// TODO: Should detect a remove sign and erase that in here?

		for ( auto &pIt : bulletPtrs )
		{
			if ( !pIt ) { continue; }
			// else

			pIt->PhysicUpdate( elapsedTime, terrain );

			if ( pIt->ShouldRemove() )
			{
				pIt->Uninit();
			}
		}

		RemoveInstancesIfNeeds();
	}
	void Admin::Draw( RenderingHelper *pRenderer ) const
	{
		if ( !pRenderer ) { return; }
		// else
		for ( const auto &pIt : bulletPtrs )
		{
			if ( !pIt ) { continue; }
			// else

			pIt->Draw( pRenderer );
		}
	}
	void Admin::DrawHitBoxes( RenderingHelper *pRenderer, const Donya::Vector4x4 &VP ) const
	{
		if ( !pRenderer ) { return; }
		// else
		for ( const auto &pIt : bulletPtrs )
		{
			if ( !pIt ) { continue; }
			// else

			pIt->DrawHitBox( pRenderer, VP );
		}
	}
	void Admin::ClearInstances()
	{
		for ( auto &pIt : bulletPtrs )
		{
			if ( pIt ) { pIt->Uninit(); }
		}
		bulletPtrs.clear();
	}
	void Admin::RequestFire( const FireDesc &parameter )
	{
		generateRequests.emplace_back( parameter );
	}
	void Admin::AddCopy( const std::shared_ptr<Base> &pBullet )
	{
		copyRequests.emplace_back( pBullet );
	}
	size_t Admin::GetInstanceCount() const
	{
		return bulletPtrs.size();
	}
	bool Admin::IsOutOfRange( size_t instanceIndex ) const
	{
		return ( GetInstanceCount() <= instanceIndex ) ? true : false;
	}
	std::shared_ptr<const Base> Admin::GetInstanceOrNullptr( size_t instanceIndex ) const
	{
		if ( IsOutOfRange( instanceIndex ) ) { return nullptr; }
		// else
		return bulletPtrs[instanceIndex];
	}
	namespace
	{
		template<typename ReturnType>
		std::shared_ptr<ReturnType> FindInstanceOrNullptrImpl( const std::vector<std::shared_ptr<Base>> &rangeA, const std::vector<std::shared_ptr<Base>> &rangeB, const std::shared_ptr<Base> &key )
		{
			if ( !key ) { return nullptr; }
			// else

			for ( const auto &it : rangeA )
			{
				// Compare only the address of managed object
				if ( it.get() == key.get() )
				{
					return it;
				}
			}
			for ( const auto &it : rangeB )
			{
				// Compare only the address of managed object
				if ( it.get() == key.get() )
				{
					return it;
				}
			}

			return nullptr;
		}
	}
	std::shared_ptr<Base> Admin::FindInstanceOrNullptr( const std::shared_ptr<Base> &pBullet )
	{
		// This method may come before the generation of requests
		return FindInstanceOrNullptrImpl<Base>( bulletPtrs, copyRequests, pBullet );
	}
	std::shared_ptr<const Base> Admin::FindInstanceOrNullptr( const std::shared_ptr<Base> &pBullet ) const
	{
		// This method may come before the generation of requests
		return FindInstanceOrNullptrImpl<const Base>( bulletPtrs, copyRequests, pBullet );
	}
	void Admin::GenerateRequestedFires()
	{
		auto Append = [&]( const FireDesc &parameter )
		{
			std::shared_ptr<Base> tmp = nullptr;

			switch ( parameter.kind )
			{
			case Kind::Buster:		tmp = std::make_shared<Buster>();		break;
			case Kind::SkullBuster:	tmp = std::make_shared<SkullBuster>();	break;
			case Kind::SkullShield:	tmp = std::make_shared<SkullShield>();	break;
			case Kind::SuperBall:	tmp = std::make_shared<SuperBall>();	break;
			case Kind::Bone:		tmp = std::make_shared<Bone>();			break;
			case Kind::TogeheroBody:tmp = std::make_shared<TogeheroBody>();	break;
			default: _ASSERT_EXPR( 0, L"Error: Unexpected bullet kind!" );	return;
			}

			if ( !tmp ) { return; }
			// else

			tmp->Init( parameter );
			bulletPtrs.emplace_back( std::move( tmp ) );
		};

		for ( const auto &it : generateRequests )
		{
			Append( it );
		}

		for ( auto &it : copyRequests )
		{
			bulletPtrs.emplace_back( std::move( it ) );
		}
	}
	void Admin::RemoveInstancesIfNeeds()
	{
		auto result = std::remove_if
		(
			bulletPtrs.begin(), bulletPtrs.end(),
			[]( std::shared_ptr<Base> &pElement )
			{
				return ( pElement ) ? pElement->ShouldRemove() : true;
			}
		);
		bulletPtrs.erase( result, bulletPtrs.end() );
	}
#if USE_IMGUI
	void Admin::ShowImGuiNode( const std::string &nodeCaption )
	{
		if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
		// else

		if ( ImGui::TreeNode( u8"���̑���" ) )
		{
			std::string caption;
			const size_t count = bulletPtrs.size();
			for ( size_t i = 0; i < count; ++i )
			{
				if ( !bulletPtrs[i] ) { continue; }
				// else

				caption =  "[";
				if ( i < 100 ) { caption += "_"; } // Align
				if ( i < 10  ) { caption += "_"; } // Align
				caption += std::to_string( i );
				caption += "]";
				bulletPtrs[i]->ShowImGuiNode( caption );
			}

			ImGui::TreePop();
		}

		ImGui::TreePop();
	}
#endif // USE_IMGUI
}
