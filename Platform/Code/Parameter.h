#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "Donya/Serializer.h"
#include "Donya/Template.h"
#include "Donya/UseImGui.h"

#include "FilePath.h"

#if USE_IMGUI
namespace ParameterHelper
{
	enum class IOOperation
	{
		None,
		Save,
		LoadBinary,
		LoadJson
	};
	IOOperation ShowIONode();
}
#endif // USE_IMGUI


/// <summary>
/// The template type must has below methods:
/// -template&lt;class Archive&gt; serialize( Archive &amp; ) : used for cereal API.
/// -ShowImGuiNode( void ) : only when debug mode.
/// </summary>
template<typename ParameterClass>
class ParamOperator
{
private:
	const char *objectName = "UNINITIALIZED";
	const char *folderName = "UNINITIALIZED";
	ParameterClass param{};
public:
	/// <summary>
	/// "objectIdentifier" will used as identifier of class, and output file name.
	/// </summary>
	ParamOperator( const char *objectIdentifier )
		: objectName( objectIdentifier ), folderName( "" ), param()
	{}
	/// <summary>
	/// "objectIdentifier" will be used as the identifier of class, and output file name.
	/// "folderName" will be used as the relative directory of an output file, it must end by '/'.
	/// It will output as: "ABC/folderName/objectIdentifier" .bin or .json
	/// </summary>
	ParamOperator( const char *objectIdentifier, const char *folderName )
		: objectName( objectIdentifier ), folderName( folderName ), param()
	{}
	ParamOperator( const ParamOperator &  ) = delete;
	ParamOperator(       ParamOperator && ) = delete;
	ParamOperator &operator = ( const ParamOperator &  ) = delete;
	ParamOperator &operator = (       ParamOperator && ) = delete;
	~ParamOperator() = default;
private:
	friend class cereal::access;
	template<class Archive>
	void serialize( Archive &archive )
	{
		archive( CEREAL_NVP( param ) );
	}
public:
	void LoadParameter()
	{
	#if DEBUG_MODE
		LoadJson();
		// If a user was changed only a json file, the user wanna apply the changes to binary file also.
		// So save here.
		SaveBinary();
	#else
		LoadBinary();
	#endif // DEBUG_MODE
	}
public:
	const ParameterClass &Get() const
	{
		return param;
	}
	void Set( const ParameterClass &assignment )
	{
		param = assignment;
	}
private:
	std::string MakeFolderName() const
	{
		return std::string{ folderName } + objectName;
	}
	void LoadBinary()
	{
		constexpr bool fromBinary = true;
		Donya::Serializer::Load( param, MakeParameterPathBinary( MakeFolderName() ).c_str(), objectName, fromBinary );
	}
	void LoadJson()
	{
		constexpr bool fromBinary = false;
		Donya::Serializer::Load( param, MakeParameterPathJson( MakeFolderName() ).c_str(), objectName, fromBinary );
	}
	void SaveBinary()
	{
		constexpr bool fromBinary = true;
		Donya::Serializer::Save( param, MakeParameterPathBinary( MakeFolderName() ).c_str(), objectName, fromBinary );
	}
	void SaveJson()
	{
		constexpr bool fromBinary = false;
		Donya::Serializer::Save( param, MakeParameterPathJson( MakeFolderName() ).c_str(), objectName, fromBinary );
	}
#if USE_IMGUI
private:
	void IOByNodeResult()
	{
		const auto result = ParameterHelper::ShowIONode();
		using Op = ParameterHelper::IOOperation;
		if ( result == Op::Save )
		{
			SaveBinary();
			SaveJson  ();
		}
		else if ( result == Op::LoadBinary )
		{
			LoadBinary();
		}
		else if ( result == Op::LoadJson )
		{
			LoadJson();
		}
	}
public:
	void ShowImGuiNode( const std::string &nodeCaption )
	{
		if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
		// else
		
		param.ShowImGuiNode();
		IOByNodeResult();

		ImGui::TreePop();
	}
	/// <summary>
	/// We regards the signature of template type as: "void Func( ParameterClass &amp; )"
	/// We call that as: ShowImGuiNode( obj );
	/// </summary>
	template<typename ShowImGuiNodeMethod>
	void ShowImGuiNode( const std::string &nodeCaption, ShowImGuiNodeMethod &ShowImGuiNode )
	{
		if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
		// else
		
		ShowImGuiNode( param );
		IOByNodeResult();

		ImGui::TreePop();
	}
#endif // USE_IMGUI
};
