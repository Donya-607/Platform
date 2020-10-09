#include <locale.h>
#include <time.h>
#include <windows.h>

#include "Donya/Constant.h"
#include "Donya/Donya.h"
#include "Donya/Useful.h"

#include "Common.h"
#include "Effect/EffectAdmin.h"
#include "Framework.h"
#include "Icon.h"

namespace
{
	constexpr auto mbTellFatalError = MB_OK | MB_ICONERROR;
}

INT WINAPI wWinMain( _In_ HINSTANCE instance, _In_opt_ HINSTANCE prevInstance, _In_ LPWSTR cmdLine, _In_ INT cmdShow )
{
#if DEBUG_MODE
	// reference:https://docs.microsoft.com/ja-jp/visualstudio/debugger/crt-debug-heap-details?view=vs-2015
	_CrtSetDbgFlag
	(
		_CRTDBG_ALLOC_MEM_DF
		| _CRTDBG_LEAK_CHECK_DF
		// | _CRTDBG_CHECK_ALWAYS_DF
	);
	// When memory leak detected, if you assign the output number to "_crtBreakAlloc",
	// the program will be stop in that memory allocate place. e.g.: _crtBreakAlloc = 123;
	// _crtBreakAlloc = ;
#endif

	setlocale( LC_ALL, "JPN" );

	srand( scast<unsigned int>( time( NULL ) ) );

	bool initResult = true;

	Donya::LibraryInitializer desc{};
	desc.screenWidth		= Common::ScreenWidth();
	desc.screenHeight		= Common::ScreenHeight();
	desc.windowCaption		= "Mimit";
	desc.enableCaptionBar	= true;
	desc.fullScreenMode		= false;
	initResult = Donya::Init( cmdShow, desc );
	if ( !initResult )
	{
		Donya::ShowMessageBox( L"System Initialization is failed.", L"ERROR", mbTellFatalError );
		return Donya::Uninit();
	}
	// else

	Donya::SetWindowIcon( instance, IDI_ICON );

	Effect::Admin::Get().Init( Donya::GetDevice(), Donya::GetImmediateContext() );
	
	Framework framework{};
	initResult = framework.Init();
	if ( !initResult )
	{
		Donya::ShowMessageBox( L"Game Initialization is failed.", L"ERROR", mbTellFatalError );
		return Donya::Uninit();
	}
	// else

	while ( Donya::MessageLoop() )
	{
		Donya::ClearViews();

		Donya::SystemUpdate();
		framework.Update( Donya::GetElapsedTime() );

		framework.Draw( Donya::GetElapsedTime() );
		Donya::Present();
	}

	framework.Uninit();

	auto   returnValue = Donya::Uninit();
	return returnValue;
}