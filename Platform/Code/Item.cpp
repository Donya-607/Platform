#include "Item.h"

#include <numeric>			// Use std::accumulate

#include "Donya/Random.h"
#include "Donya/Sound.h"

#include "Common.h"			// Use IsShowCollision()
#include "FilePath.h"
#include "ItemParam.h"
#include "ModelHelper.h"
#include "Music.h"
#include "Parameter.h"

namespace Item
{
	namespace
	{
		constexpr size_t kindCount = scast<size_t>( Kind::KindCount );
		constexpr const char *modelFolderName = "Item/";
		constexpr std::array<const char *, kindCount> modelNames
		{
			"ExtraLife",
			"LifeEnergy_Big",
			"LifeEnergy_Small",
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
				_ASSERT_EXPR( 0, L"Error: The Item's model is not initialized!" );
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

	Kind LotteryDropKind()
	{
		constexpr Kind dontDrop = Kind::KindCount;

		const auto &percents = Parameter::GetItem().dropPercents;

		const int randomLimit = std::accumulate( percents.cbegin(), percents.cend(), 0 );
		if ( randomLimit <= 0 ) { return dontDrop; }
		// else

		const int random = Donya::Random::GenerateInt( randomLimit );

		const size_t count = percents.size();
		size_t chosen = count;

		int border = 0;
		for ( size_t i = 0; i < count; ++i )
		{
			border += percents[i];
			if ( random < border )
			{
				chosen = i;
				break;
			}
		}

		if ( kindCount <= chosen ) { return dontDrop; }
		// else

		return scast<Kind>( chosen );
	}
	void DropItemByLottery( const Donya::Vector3 &wsGeneratePos )
	{
		const Kind dropKind = LotteryDropKind();
		// If invalid is chosen
		if ( dropKind == Kind::KindCount ) { return; }
		// else

		InitializeParam tmp;
		tmp.kind		= dropKind;
		tmp.aliveSecond	= Parameter::GetItem().disappearSecond;
		tmp.wsPos		= wsGeneratePos;
		Admin::Get().RequestGeneration( tmp );
	};

	namespace Parameter
	{
		void Load()
		{
			Impl::LoadItem();
		}

	#if USE_IMGUI
		void Update( const std::string &nodeCaption )
		{
			if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
			// else

			Impl::UpdateItem( u8"Item" );

			ImGui::TreePop();
		}
	#endif // USE_IMGUI

		static ParamOperator<ItemParam> itemParam{ "Item" };
		const ItemParam &GetItem()
		{
			return itemParam.Get();
		}

		namespace Impl
		{
			void LoadItem()
			{
				itemParam.LoadParameter();
			}
		#if USE_IMGUI
			void UpdateItem( const std::string &nodeCaption )
			{
				itemParam.ShowImGuiNode( nodeCaption );
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
		return GetModelName( kind );
		/*
		switch ( kind )
		{
		case Kind::ExtraLife:			return "ExtraLife";
		case Kind::LifeEnergy_Big:		return "LifeEnery_Big";
		case Kind::LifeEnergy_Small:	return "LifeEnery_Small";
		default: break;
		}

		_ASSERT_EXPR( 0, L"Error: Unexpected kind!" );
		return "ERROR_KIND";
		*/
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
	void InitializeParam::ShowImGuiNode( const std::string &nodeCaption )
	{
		if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
		// else

		ShowKindNode( u8"種生成する類", &kind );
		ImGui::DragFloat ( u8"生存時間（マイナスで永続）",		&aliveSecond );
		ImGui::DragFloat3( u8"ワールド座標",					&wsPos.x, 0.01f );

		ImGui::TreePop();
	}

	void ItemParam::General::ShowImGuiNode( const std::string &nodeCaption, bool useTreeNode )
	{
		if ( useTreeNode && !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
		// else

		ImGui::DragFloat( u8"出現時のＹ初速",			&dropBoundStrength,	0.01f );
		ImGui::DragFloat( u8"重力[m/s]",				&gravity,			0.01f );
		ImGui::DragFloat( u8"モーション再生速度",		&animePlaySpeed,	0.01f );
		ImGui::Helper::ShowAABBNode( u8"地形との当たり判定",			&body		);
		ImGui::Helper::ShowAABBNode( u8"取得時に用いる当たり判定",		&catchArea	);

		if ( useTreeNode ) { ImGui::TreePop(); }
	}
	void ItemParam::Energy::ShowImGuiNode( const std::string &nodeCaption, bool useTreeNode )
	{
		if ( useTreeNode && !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
		// else

		ImGui::DragInt( u8"回復量", &recoveryAmount );
		general.ShowImGuiNode( "", /* useTreeNode = */ false );

		if ( useTreeNode ) { ImGui::TreePop(); }
	}
	void ItemParam::ShowImGuiNode()
	{
		extraLife.ShowImGuiNode			( u8"１ＵＰ"			);
		lifeEnergyBig.ShowImGuiNode		( u8"ＨＰ回復・大"	);
		lifeEnergySmall.ShowImGuiNode	( u8"ＨＰ回復・小"	);

		ImGui::DragFloat( u8"ドロップしたものが消える秒数",	&disappearSecond,	0.01f );
		ImGui::DragFloat( u8"消える前の点滅・秒数",			&flushingSecond,	0.01f );
		ImGui::DragFloat( u8"消える前の点滅・間隔",			&flushingInterval,	0.01f );
		disappearSecond		= std::max( 0.001f, disappearSecond		);
		flushingSecond		= std::max( 0.001f, flushingSecond		);
		flushingInterval	= std::max( 0.001f, flushingInterval	);

		constexpr size_t dropCount = kindCount + 1;
		if ( dropPercents.size() != dropCount ) { dropPercents.resize( dropCount ); }
		if ( ImGui::TreeNode( u8"ドロップ率の設定" ) )
		{
			std::string caption{};
			for ( size_t i = 0; i < kindCount; ++i )
			{
				caption = GetKindName( scast<Kind>( i ) );
				ImGui::DragInt( caption.c_str(), &dropPercents[i] );
				dropPercents[i] = std::max( 1, dropPercents[i] );
			}

			auto &emptyPercent = dropPercents.back();
			ImGui::DragInt( u8"ドロップしない確率", &emptyPercent );
			emptyPercent = std::max( 1, emptyPercent );

			ImGui::TreePop();
		}
	}
#endif // USE_IMGUI

	void Item::Flusher::Start()
	{
		timer	= 0.0f;
		active	= true;
	}
	void Item::Flusher::Update( float elapsedTime )
	{
		if ( !active ) { return; }
		// else
		timer += elapsedTime;
	}
	bool Item::Flusher::IsActive() const
	{
		return active;
	}
	bool Item::Flusher::Drawable() const
	{
		if ( !active ) { return true; }
		// else

		/*
		--- cycle
		Undrawable
		--- cycle * 0.5f
		Drawable
		--- 0.0f
		*/

		const float &cycle = Parameter::GetItem().flushingInterval;
		const float remain = std::fmodf( timer, cycle );
		return ( remain < cycle * 0.5f );
	}

	namespace
	{
		const ItemParam::General *GetGeneralOrNull( Kind kind )
		{
			const auto &data = Parameter::GetItem();

			switch ( kind )
			{
			case Kind::ExtraLife:			return &data.extraLife;
			case Kind::LifeEnergy_Big:		return &data.lifeEnergyBig.general;
			case Kind::LifeEnergy_Small:	return &data.lifeEnergySmall.general;
			default: break;
			}

			_ASSERT_EXPR( 0, L"Error: Unexpected kind!" );
			return nullptr;
		}
	}

	void Item::Init( const InitializeParam &parameter, const Map &terrain )
	{
		initializer		= parameter;

		model.Initialize( GetModelPtrOrNullptr( GetKind() ) );
		model.AssignMotion( 0 );
		AssignMyBody( parameter.wsPos );
		velocity		= 0.0f;
		orientation		= Donya::Quaternion::Identity();
		aliveTimer		= 0.0f;
		wantRemove		= false;

		// If the generated position is inside of terrain
		beBuried		= IsHitToAnyOf( FetchAroundSolids( 0.0f, terrain ) );
		if ( !beBuried )
		{
			const auto *pData = GetGeneralOrNull( GetKind() );
			velocity.y	= ( pData ) ? pData->dropBoundStrength : 0.0f;
		}
	}
	void Item::Uninit() {} // No op
	void Item::Update( float elapsedTime, const Donya::Collision::Box3F &wsScreen, const Map &terrain )
	{
	#if USE_IMGUI
		// Apply for be able to see an adjustment immediately
		{
			AssignMyBody( body.pos );
		}
	#endif // USE_IMGUI

		aliveTimer += elapsedTime;

		UpdateRemoveCondition( wsScreen );

		flusher.Update( elapsedTime );

		if ( !beBuried )
		{
			velocity.y -= GetGravity() * elapsedTime;
		}

		const auto *pData = GetGeneralOrNull( GetKind() );
		const float animePlaySpeed = ( pData ) ? pData->animePlaySpeed : 1.0f;
		model.UpdateMotion( elapsedTime * animePlaySpeed, 0 );
	}
	void Item::PhysicUpdate( float elapsedTime, const Map &terrain )
	{
		const auto movement		= velocity * elapsedTime;
		const auto aroundSolids	= FetchAroundSolids( elapsedTime, terrain );

		if ( beBuried )
		{
			if ( !IsHitToAnyOf( aroundSolids ) )
			{
				beBuried = false;
			}
		
			velocity = 0.0f;
			return;
		}
		// else

		Actor::MoveX( movement.x, aroundSolids );
		Actor::MoveZ( movement.z, aroundSolids );

		const int collideIndex = Actor::MoveY( movement.y, aroundSolids );
		if ( collideIndex != -1 ) // If collided to any
		{
			velocity.y = 0.0f;
		}

		catchArea.pos = body.pos; // We must apply world position to hurt box also.
	}
	void Item::Draw( RenderingHelper *pRenderer ) const
	{
		if ( !pRenderer			) { return; }
		if ( !model.pResource	) { return; }
		// else

		const Donya::Vector3 &drawPos = body.pos; // I wanna adjust the hit-box to fit for drawing model, so I don't apply the offset for the position of drawing model.
		const Donya::Vector4x4 W = MakeWorldMatrix( 1.0f, /* enableRotation = */ true, drawPos );
		const float alpha = ( flusher.Drawable() ) ? 1.0f : 0.0f;

		Donya::Model::Constants::PerModel::Common modelConstant{};
		modelConstant.drawColor		= Donya::Vector4{ 1.0f, 1.0f, 1.0f, alpha };
		modelConstant.worldMatrix	= W;;
		pRenderer->UpdateConstant( modelConstant );
		pRenderer->ActivateConstantModel();

		pRenderer->Render( model.pResource->model, model.GetCurrentPose() );

		pRenderer->DeactivateConstantModel();
	}
	void Item::DrawHitBox( RenderingHelper *pRenderer, const Donya::Vector4x4 &matVP ) const
	{
		if ( !Common::IsShowCollision() ) { return; }
		// else
		
	#if DEBUG_MODE
		Donya::Model::Cube::Constant constant;
		constant.matViewProj	= matVP;
		constant.lightDirection	= -Donya::Vector3::Up();

		auto DrawProcess = [&]( const Donya::Collision::Box3F &box, const Donya::Vector4 &color )
		{
			const Donya::Vector4x4 W = MakeWorldMatrix
			(
				box.size,
				/* enableRotation = */ false,
				box.WorldPosition()
			);

			constant.matWorld  = W;
			constant.drawColor = color;
			pRenderer->ProcessDrawingCube( constant );
		};
	
		constexpr Donya::Vector4 bodyColor{ 0.0f, 1.0f, 0.3f, 0.6f };
		constexpr Donya::Vector4 areaColor{ 1.0f, 1.0f, 0.3f, 0.6f };
		const auto  &area		= catchArea;
		const float bodyNear	= body.WorldPosition().z - body.size.z;
		const float areaNear	= area.WorldPosition().z - area.size.z;
		// Drawing the far box first
		if ( bodyNear < areaNear )
		{
			DrawProcess( area, areaColor );
			DrawProcess( body, bodyColor );
		}
		else
		{
			DrawProcess( body, bodyColor );
			DrawProcess( area, areaColor );
		}
	#endif // DEBUG_MODE
	}
	bool					Item::IsDynamic()		const
	{
		return ( initializer.aliveSecond < 0.0f ) ? false : true;
	}
	bool					Item::ShouldRemove()	const
	{
		return wantRemove;
	}
	Donya::Collision::Box3F	Item::GetCatchArea()	const
	{
		return catchArea;
	}
	Kind					Item::GetKind()			const
	{
		return initializer.kind;
	}
	InitializeParam			Item::GetInitializer()	const
	{
		return initializer;
	}
	void Item::WasCaught() const
	{
		wantRemove = true;
		Donya::Sound::Play( Music::CatchItem );
	}
	std::vector<Donya::Collision::Box3F> Item::FetchAroundSolids( float elapsedTime, const Map &terrain ) const
	{
		const auto myBody		= GetHitBox();
		const auto movement		= velocity * elapsedTime;
		const auto aroundTiles	= terrain.GetPlaceTiles( myBody, movement );
			  auto aroundSolids	= Map::ToAABBSolids( aroundTiles, terrain, myBody );
		Donya::AppendVector( &aroundSolids, terrain.GetExtraSolids() );

		return aroundSolids;
	}
	bool Item::IsHitToAnyOf( const std::vector<Donya::Collision::Box3F> &solids ) const
	{
		const auto me = GetHitBox();
		for ( const auto &it : solids )
		{
			if ( Donya::Collision::IsHit( me, it ) )
			{
				return true;
			}
		}

		return false;
	}
	bool Item::OnOutSideScreen( const Donya::Collision::Box3F &wsScreen )
	{
		return !Donya::Collision::IsHit( catchArea, wsScreen, /* considerExistFlag = */ false );
	}
	void Item::UpdateRemoveCondition( const Donya::Collision::Box3F &wsScreen )
	{
		if ( wantRemove ) { return; }
		// else

		const float &aliveLimit = initializer.aliveSecond;
		if ( aliveLimit < 0.0f ) { return; } // Minus time specifies don't remove until caught
		// else

		if ( !flusher.IsActive() )
		{
			const float beginFlushingSecond = aliveLimit - Parameter::GetItem().flushingSecond;
			if ( beginFlushingSecond <= aliveTimer )
			{
				flusher.Start();
			}
		}

		const bool willDisappear = ( aliveLimit <= aliveTimer );
		if ( willDisappear || OnOutSideScreen( wsScreen ) )
		{
			wantRemove = true;
		}
	}
	float Item::GetGravity() const
	{
		constexpr float errorGravity = -1.0f;
		const auto *pData = GetGeneralOrNull( GetKind() );
		return ( pData ) ? pData->gravity : errorGravity;
	}
	void Item::AssignMyBody( const Donya::Vector3 &wsPos )
	{
		const auto *pData = GetGeneralOrNull( GetKind() );
		if ( !pData ) { return; }
		// else

		body.pos			= wsPos;
		body.offset			= pData->body.offset;
		body.size			= pData->body.size;
		catchArea.pos		= wsPos;
		catchArea.offset	= pData->catchArea.offset;
		catchArea.size		= pData->catchArea.size;

		body.offset			= orientation.RotateVector( body.offset			);
		catchArea.offset	= orientation.RotateVector( catchArea.offset	);
	}
	Donya::Vector4x4 Item::MakeWorldMatrix( const Donya::Vector3 &scale, bool enableRotation, const Donya::Vector3 &translation ) const
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
	bool Item::ShowImGuiNode( const std::string &nodeCaption )
	{
		if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return false; }
		// else
		
		if ( ImGui::Button( ( nodeCaption + u8"を削除する" ).c_str() ) )
		{
			wantRemove = true;
		}
		
		initializer.ShowImGuiNode( u8"初期化パラメータ" );
		ImGui::DragFloat ( u8"生きている秒数",	&aliveTimer, 0.1f  );
		ImGui::DragFloat3( u8"ワールド座標",		&body.pos.x, 0.01f );
		ImGui::DragFloat3( u8"速度",				&velocity.x, 0.01f );
		ImGui::Helper::ShowFrontNode( "", &orientation );

		ImGui::TreePop();
		return true;
	}
#endif // USE_IMGUI


	void Admin::Uninit()
	{
		for ( auto &it : items )
		{
			it.Uninit();
		}
	}
	void Admin::Update( float elapsedTime, const Donya::Collision::Box3F &wsScreen, const Map &terrain )
	{
		GenerateRequestedItems( terrain );
		generateRequests.clear();

		for ( auto &it : items )
		{
			it.Update( elapsedTime, wsScreen, terrain );
		}

		RemoveItemsIfNeeds();
	}
	void Admin::PhysicUpdate( float elapsedTime, const Map &terrain )
	{
		for ( auto &it : items )
		{
			it.PhysicUpdate( elapsedTime, terrain );
		}
	}
	void Admin::Draw( RenderingHelper *pRenderer ) const
	{
		if ( !pRenderer ) { return; }
		// else

		for ( const auto &it : items )
		{
			it.Draw( pRenderer );
		}
	}
	void Admin::DrawHitBoxes( RenderingHelper *pRenderer, const Donya::Vector4x4 &matVP ) const
	{
		if ( !Common::IsShowCollision() || !pRenderer ) { return; }
		// else

		for ( const auto &it : items )
		{
			it.DrawHitBox( pRenderer, matVP );
		}
	}
	void Admin::ClearInstances()
	{
		for ( auto &it : items )
		{
			it.Uninit();
		}
		items.clear();
	}
	void Admin::RemoveDynamicInstances()
	{
		// Finalize only item that will be removed
		for ( auto &it : items )
		{
			if ( it.IsDynamic() )
			{
				it.Uninit();
			}
		}

		auto result = std::remove_if
		(
			items.begin(), items.end(),
			[]( Item &element )
			{
				return element.IsDynamic();
			}
		);
		items.erase( result, items.end() );
	}
	void Admin::RequestGeneration( const InitializeParam &initializer )
	{
		generateRequests.emplace_back( initializer );
	}
	bool Admin::LoadItems( int stageNumber, const Map &terrain, bool fromBinary )
	{
		ClearInstances();

		const std::string filePath	= ( fromBinary )
									? MakeStageParamPathBinary( ID, stageNumber )
									: MakeStageParamPathJson  ( ID, stageNumber );
		Donya::Serializer tmp;
		const bool succeeded		= ( fromBinary )
									? tmp.LoadBinary( *this, filePath.c_str(), ID )
									: tmp.LoadJSON	( *this, filePath.c_str(), ID );

		// I should call Init()
		for ( auto &it : items )
		{
			it.Init( it.GetInitializer(), terrain );
		}

		return succeeded;
	}
	size_t Admin::GetInstanceCount() const
	{
		return items.size();
	}
	bool Admin::IsOutOfRange( size_t instanceIndex ) const
	{
		return ( GetInstanceCount() <= instanceIndex ) ? true : false;
	}
	const Item *Admin::GetInstanceOrNullptr( size_t instanceIndex ) const
	{
		if ( IsOutOfRange( instanceIndex ) ) { return nullptr; }
		// else
		return &items[instanceIndex];
	}
	void Admin::GenerateRequestedItems( const Map &terrain )
	{
		for ( const auto &it : generateRequests )
		{
			Item tmp{};
			tmp.Init( it, terrain );
			items.emplace_back( std::move( tmp ) );
		}
	}
	void Admin::RemoveItemsIfNeeds()
	{
		auto itr = std::remove_if
		(
			items.begin(), items.end(),
			[]( Item &element )
			{
				return element.ShouldRemove();
			}
		);
		items.erase( itr, items.end() );
	}
#if USE_IMGUI
	void Admin::RemakeByCSV( const CSVLoader &loadedData, const Map &terrain )
	{
		auto IsItemID	= []( int id )
		{
			return ( StageFormat::ItemStart <= id && id <= StageFormat::ItemLast );
		};
		auto IsValidID	= []( int id )
		{
			id -= StageFormat::ItemStart;
			return ( 0 <= id && id < scast<int>( Kind::KindCount ) );
		};
		auto Append		= [&]( int id, size_t row, size_t column )
		{
			if ( !IsItemID( id ) ) { return; }
			// else

			if ( !IsValidID( id ) )
			{
				std::string msg = u8"実装されていないアイテムの番号を検出しました。\n";
				msg += u8"行:" + std::to_string( row    ) + u8", ";
				msg += u8"列:" + std::to_string( column ) + u8", ";
				msg += u8"ID:" + std::to_string( id     ) + u8".\n";

				Donya::ShowMessageBox
				(
					msg, "Announce",
					MB_ICONEXCLAMATION | MB_OK
				);
				return;
			}
			// else

			InitializeParam init;
			init.kind			= scast<Kind>( id - StageFormat::ItemStart ); // Convert to zero-based
			init.aliveSecond	= -1.0f; // Specify to living as infinitely
			init.wsPos			= Map::ToWorldPos( row, column );
			Item tmp{};
			tmp.Init( init, terrain );
			items.emplace_back( std::move( tmp ) );
		};

		ClearInstances();

		const auto &data = loadedData.Get();
		const size_t rowCount = data.size();
		for ( size_t r = 0; r < rowCount; ++r )
		{
			const size_t columnCount = data[r].size();
			for ( size_t c = 0; c < columnCount; ++c )
			{
				Append( data[r][c], r, c );
			}
		}
	}
	void Admin::SaveItems( int stageNumber, bool fromBinary )
	{
		const std::string filePath	= ( fromBinary )
									? MakeStageParamPathBinary( ID, stageNumber )
									: MakeStageParamPathJson  ( ID, stageNumber );
		MakeDirectoryIfNotExists( filePath );
		MakeFileIfNotExists( filePath, fromBinary );

		Donya::Serializer tmp;
		( fromBinary )
		? tmp.SaveBinary( *this, filePath.c_str(), ID )
		: tmp.SaveJSON	( *this, filePath.c_str(), ID );
	}
	void Admin::ShowImGuiNode( const std::string &nodeCaption, int stageNo, const Map &terrain )
	{
		if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
		// else

		if ( ImGui::TreeNode( u8"実体たち" ) )
		{
			const size_t enemyCount = items.size();

			for ( size_t i = 0; i < enemyCount; ++i )
			{
				ShowInstanceNode( i );
			}

			ImGui::TreePop();
		}

		const auto result = ParameterHelper::ShowIONode();
		using Op = ParameterHelper::IOOperation;
		if ( result == Op::Save )
		{
			SaveItems( stageNo, true  );
			SaveItems( stageNo, false );
		}
		else if ( result == Op::LoadBinary )
		{
			LoadItems( stageNo, terrain, true );
		}
		else if ( result == Op::LoadJson )
		{
			LoadItems( stageNo, terrain, false );
		}

		ImGui::TreePop();
	}
	void Admin::ShowInstanceNode( size_t index )
	{
		if ( IsOutOfRange( index ) ) { return; }
		// else

		auto &item = items[index];

		std::string caption = Donya::MakeArraySuffix( index );
		bool  treeIsOpen = item.ShowImGuiNode( caption );
		if ( !treeIsOpen )
		{
			caption = "[";
			caption += GetModelName( item.GetKind() );
			caption += "]";
			ImGui::SameLine();
			ImGui::Text( caption.c_str() );
		}
	}
#endif // USE_IMGUI
}
