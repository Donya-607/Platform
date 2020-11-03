#pragma once

// see https://maminus.hatenadiary.org/entry/20130226/1361895337
// I want to lightly this file.
#ifdef _M_IX86
#define _X86_
#else
#define _AMD64_
#endif
#include <windef.h>

struct ID3D11Device;
struct ID3D11DeviceContext;

namespace Donya
{
	struct LibraryInitializer
	{
		int  screenWidth;			// Specify the window width. Default is 800.
		int  screenHeight;			// Specify the window height. Default is 600.
		const char *windowCaption;	// Specify the window-name. this name also used to class-name. Default is "Donya".
		bool enableCaptionBar;		// Specify add a caption-bar on window. Default is true.
		bool isAppendFPS;			// Specify append an FPS information behind window-caption. This flag enables only when debug-mode(default is true).
		bool fullScreenMode;		// Specify the window-mode. Default is false.
		bool enableMultiThreaded;	// Specify use multi-thread. Default is true.
	public:
		LibraryInitializer();
	};
	/// <summary>
	/// Initialize Donya library, if not initialized yet.<para></para>
	/// Use Windows::Foundation::Initialize() with RO_INIT_SINGLETHREADED(enableMultiThreaded=false) or RO_INIT_MULTITHREADED(enableMultiThreaded=true).<para></para>
	/// If initialize failed, returns false.<para></para>
	/// In release mode, the isAppendFPS flag fixed to false.
	/// </summary>
	bool Init( int nCmdShow, const LibraryInitializer &description );

	/// <summary>
	/// Returns false if failed.
	/// </summary>
	bool SetWindowIcon( const HINSTANCE &hInstance, int iconID );

	/// <summary>
	/// Please call to first in game-loop.<para></para>
	/// If I received WM_QUIT, returns false.
	/// </summary>
	int MessageLoop();

	/// <summary>
	/// Please call after MessageLoop().<para></para>
	/// This function doing:<para></para>
	/// Keyboard::Update(),<para></para>
	/// ScreenShake::Update(),<para></para>
	/// Sound::Update().
	/// </summary>
	void SystemUpdate();

	/// <summary>
	/// Represent delta-time.
	/// </summary>
	float GetElapsedTime();

	float GetFPS();

	/// <summary>
	/// Returns caption was passed when Donya::Init().
	/// </summary>
	const char *GetWindowCaption();

	/// <summary>
	/// Doing OMSetRenderTargets() with library's views, RSSetViewports().<para></para>
	/// /// If you set nullptr to the context it uses the immediate context.
	/// </summary>
	void SetDefaultRenderTargets( ID3D11DeviceContext *pContext = nullptr );
	/// <summary>
	/// Doing ClearRenderTargetView(), ClearDepthStencilView().<para></para>
	/// fill color range is 0.0f ~ 1.0f.<para></para>
	/// If you set nullptr to the context it uses the immediate context.
	/// </summary>
	void ClearViews( FLOAT R = 0.0f, FLOAT G = 0.0f, FLOAT B = 0.0f, FLOAT A = 1.0f, ID3D11DeviceContext *pContext = nullptr );
	/// <summary>
	/// Doing ClearRenderTargetView(), ClearDepthStencilView().<para></para>
	/// fill color range is 0.0f ~ 1.0f.<para></para>
	/// If you set nullptr to the context it uses the immediate context.
	/// </summary>
	void ClearViews( const FLOAT ( &fillColor )[4], ID3D11DeviceContext *pContext = nullptr );

	/// <summary>
	/// Returns the result of the creation.
	/// If the argument is nullptr, it reeturns E_INVALIDARG.
	/// </summary>
	HRESULT CreateDeferredContext( ID3D11DeviceContext **pOutput );

	/// <summary>
	/// Doing IDXGISwapChain::Present(), check and assertion return value.<para></para>
	/// Returns false when failed.
	/// </summary>
	bool Present( UINT syncInterval = 0, UINT flags = 0 );

	/// <summary>
	/// Returns value is the exit code.<para></para>
	/// You should use the value in return value of WinMain().
	/// </summary>
	int Uninit();

	HWND				&GetHWnd();
	ID3D11Device		*GetDevice();
	ID3D11DeviceContext	*GetImmediateContext();

	/// <summary>
	/// This namespace there for Donya library.
	/// </summary>
	namespace Private
	{
		int		RegisteredScreenWidth();
		float	RegisteredScreenWidthF();
		long	RegisteredScreenWidthL();

		int		RegisteredScreenHeight();
		float	RegisteredScreenHeightF();
		long	RegisteredScreenHeightL();

		int		RegisteredHalfScreenWidth();
		float	RegisteredHalfScreenWidthF();
		long	RegisteredHalfScreenWidthL();

		int		RegisteredHalfScreenHeight();
		float	RegisteredHalfScreenHeightF();
		long	RegisteredHalfScreenHeightL();
	}
}
