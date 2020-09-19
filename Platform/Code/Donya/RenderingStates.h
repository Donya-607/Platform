#pragma once

struct D3D11_DEPTH_STENCIL_DESC;
struct D3D11_RASTERIZER_DESC;
struct D3D11_SAMPLER_DESC;
struct ID3D11Device;
struct ID3D11DeviceContext;

namespace Donya
{
	// The Blend-State is there at "Blend.h".
	// Because the timing of creating this file was different from "Blend.h".
	// And I think the using frequency by a user is lower than "Blend.h".

	/// <summary>
	/// Provides caching system for global access.
	/// </summary>
	namespace DepthStencil
	{
		enum Defined : int
		{
			Write_PassLess		= -1,	// Z-Test:ON. Z-Write:On. Pass if less than Z-buffer.
			Write_PassLessEq	= -2,	// Z-Test:ON. Z-Write:On. Pass if less equal than Z-buffer.
			Write_PassGreater	= -3,	// Z-Test:ON. Z-Write:On. Pass if greater than Z-buffer.
			Write_PassGreaterEq	= -4,	// Z-Test:ON. Z-Write:On. Pass if greater equal than Z-buffer.
			NoTest_Write		= -5,	// Z-Test:OFF. Z-Write:On.  Pass state is default.
			NoTest_NoWrite		= -6,	// Z-Test:OFF. Z-Write:OFF. Pass state is default.
		};

		/// <summary>
		/// Create and cache a depth-stencil-state object to an internal map.<para></para>
		/// If you set nullptr(default) to "pDevice", I use default(library's) device.<para></para>
		/// Returns true if succeeded a creation, or already created.<para></para>
		/// I recommend using enum value to the identifier.
		/// </summary>
		bool CreateState( int identifier, const D3D11_DEPTH_STENCIL_DESC &registerDesc, ID3D11Device *pDevice = nullptr );
		/// <summary>
		/// Create and cache the pre-defined depth-stencil-state objects to an internal map.<para></para>
		/// If you set nullptr(default) to "pDevice", I use default(library's) device.<para></para>
		/// Returns true if succeeded a creation, or already created.
		/// </summary>
		bool CreateDefinedStates( ID3D11Device *pDevice = nullptr );

		/// <summary>
		/// Returns true if the identifier already used for creating. This confirmation is also used internally at the create method.
		/// </summary>
		bool IsAlreadyExists( int identifier );
		/// <summary>
		/// Returns unused identifier, or 0 if an available number is not found.<para></para>
		/// The argument specify the searching direction of sign, but a negative value is used for library's identifier, so I recommend to leaving specify true(default value).
		/// </summary>
		int  FindUsableIdentifier( bool findPositiveValue = true );

		/// <summary>
		/// Activate the specify depth-stencil-state.<para></para>
		/// If you set nullptr(default) to "pContext", I use default(library's) immediate-context.<para></para>
		/// Returns false if failed activation(maybe the identifier does not cache?).
		/// </summary>
		bool Activate( int identifier, ID3D11DeviceContext *pContext = nullptr );
		/// <summary>
		/// Deactivate current depth-stencil-state.<para></para>
		/// If you set nullptr(default) to "pContext", I use default(library's) immediate-context.<para></para>
		/// Returns false if failed deactivation(maybe the identifier does not cache?).
		/// </summary>
		void Deactivate( ID3D11DeviceContext *pContext = nullptr );

		/// <summary>
		/// Release all a cached depth-stencil-states. The identifiers of released state will be invalid.
		/// </summary>
		void ReleaseAllCachedStates();
	}

	/// <summary>
	/// Provides caching system for global access.
	/// </summary>
	namespace Rasterizer
	{
		enum Defined : int
		{
			Solid_CullBack_CCW		= -1,	// Fill:Solid. Cull:Back. Front:CCW. DepthClip:True. DepthBias:AllZero. Scissor:False. MSAA:False. AALine:False.
			Solid_CullBack_CW		= -2,	// Fill:Solid. Cull:Back. Front:CW. DepthClip:True. DepthBias:AllZero. Scissor:False. MSAA:False. AALine:False.
			Solid_CullNone_NoClip	= -3,	// Fill:Solid. Cull:None. DepthClip:False. DepthBias:AllZero. Scissor:False. MSAA:False. AALine:False.
			Solid_CullNone			= -4,	// Fill:Solid. Cull:None. DepthClip:True. DepthBias:AllZero. Scissor:False. MSAA:False. AALine:False.
			Wired_CullBack_CCW		= -5,	// Fill:Wireframe. Cull:Back. Front:CCW. DepthClip:True. DepthBias:AllZero. Scissor:False. MSAA:False. AALine:False.
			Wired_CullBack_CW		= -6,	// Fill:Wireframe. Cull:Back. Front:CW. DepthClip:True. DepthBias:AllZero. Scissor:False. MSAA:False. AALine:False.
			Wired_CullNone_NoClip	= -7,	// Fill:Wireframe. Cull:None. DepthClip:False. DepthBias:AllZero. Scissor:False. MSAA:False. AALine:False.
			Wired_CullNone			= -8,	// Fill:Wireframe. Cull:None. DepthClip:True. DepthBias:AllZero. Scissor:False. MSAA:False. AALine:False.
		};

		/// <summary>
		/// Create and cache a rasterizer-state object to an internal map.<para></para>
		/// If you set nullptr(default) to "pDevice", I use default(library's) device.<para></para>
		/// Returns true if succeeded a creation, or already created.<para></para>
		/// I recommend using enum value to the identifier.
		/// </summary>
		bool CreateState( int identifier, const D3D11_RASTERIZER_DESC &registerDesc, ID3D11Device *pDevice = nullptr );
		/// <summary>
		/// Create and cache the pre-defined rasterizer-state objects to an internal map.<para></para>
		/// If you set nullptr(default) to "pDevice", I use default(library's) device.<para></para>
		/// Returns true if succeeded a creation, or already created.
		/// </summary>
		bool CreateDefinedStates( ID3D11Device *pDevice = nullptr );

		/// <summary>
		/// Returns true if the identifier already used for creating. This confirmation is also used internally at the create method.
		/// </summary>
		bool IsAlreadyExists( int identifier );
		/// <summary>
		/// Returns unused identifier, or 0 if an available number is not found.<para></para>
		/// The argument specify the searching direction of sign, but a negative value is used for library's identifier, so I recommend to leaving specify true(default value).
		/// </summary>
		int  FindUsableIdentifier( bool findPositiveValue = true );

		/// <summary>
		/// Activate the specify rasterizer-state.<para></para>
		/// If you set nullptr(default) to "pContext", I use default(library's) immediate-context.<para></para>
		/// Returns false if failed activation(maybe the identifier does not cache?).
		/// </summary>
		bool Activate( int identifier, ID3D11DeviceContext *pContext = nullptr );
		/// <summary>
		/// Deactivate current rasterizer-state.<para></para>
		/// If you set nullptr(default) to "pContext", I use default(library's) immediate-context.<para></para>
		/// Returns false if failed deactivation(maybe the identifier does not cache?).
		/// </summary>
		void Deactivate( ID3D11DeviceContext *pContext = nullptr );

		/// <summary>
		/// Release all a cached rasterizer-states. The identifiers of released state will be invalid.
		/// </summary>
		void ReleaseAllCachedStates();
	}

	/// <summary>
	/// Provides caching system for global access.
	/// </summary>
	namespace Sampler
	{
		enum Defined : int
		{
			Point_Wrap			= -1,	// Filter:Point. Address:Wrap. Comparison:Never.
			Linear_Wrap			= -2,	// Filter:Linear. Address:Wrap. Comparison:Never.
			Linear_Border_Black	= -3,	// Filter:Linear. Address:Border. BorderColor:(0, 0, 0, 1). Comparison:Never.
			Aniso_Wrap			= -4,	// Filter:Anisotropic. Address:Wrap. Comparison:Never.
			Aniso_Clamp			= -5,	// Filter:Anisotropic. Address:Clamp. Comparison:Never.
			Point_Border_Black	= -6,	// Filter:Point. Address:Border. BorderColor:(0, 0, 0, 1). Comparison:Never.
			Point_Border_Clear	= -7,	// Filter:Point. Address:Border. BorderColor:(0, 0, 0, 0). Comparison:Never.
			Point_Border_White	= -8,	// Filter:Point. Address:Border. BorderColor:(1, 1, 1, 1). Comparison:Never.
		};

		/// <summary>
		/// Create and cache a sampler-state object to an internal map.<para></para>
		/// If you set nullptr(default) to "pDevice", I use default(library's) device.<para></para>
		/// I recommend using enum value to the identifier.
		/// </summary>
		bool CreateState( int identifier, const D3D11_SAMPLER_DESC &registerDesc, ID3D11Device *pDevice = nullptr );
		/// <summary>
		/// Create and cache the pre-defined sampler-state objects to an internal map.<para></para>
		/// If you set nullptr(default) to "pDevice", I use default(library's) device.<para></para>
		/// Returns true if succeeded a creation, or already created.
		/// </summary>
		bool CreateDefinedStates( ID3D11Device *pDevice = nullptr );

		/// <summary>
		/// Returns true if the identifier already used for creating. This confirmation is also used internally at the create method.
		/// </summary>
		bool IsAlreadyExists( int identifier );
		/// <summary>
		/// Returns unused identifier, or 0 if an available number is not found.<para></para>
		/// The argument specify the searching direction of sign, but a negative value is used for library's identifier, so I recommend to leaving specify true(default value).
		/// </summary>
		int  FindUsableIdentifier( bool findPositiveValue = true );

		/// <summary>
		/// Activate the specify sampler-state.<para></para>
		/// If you set nullptr(default) to "pContext", I use default(library's) immediate-context.<para></para>
		/// Returns false if failed activation(maybe the identifier does not cache?).
		/// </summary>
		bool SetVS( int identifier, unsigned int setSlot, ID3D11DeviceContext *pContext = nullptr );
		/// <summary>
		/// Activate the specify sampler-state.<para></para>
		/// If you set nullptr(default) to "pContext", I use default(library's) immediate-context.<para></para>
		/// Returns false if failed activation(maybe the identifier does not cache?).
		/// </summary>
		bool SetGS( int identifier, unsigned int setSlot, ID3D11DeviceContext *pContext = nullptr );
		/// <summary>
		/// Activate the specify sampler-state.<para></para>
		/// If you set nullptr(default) to "pContext", I use default(library's) immediate-context.<para></para>
		/// Returns false if failed activation(maybe the identifier does not cache?).
		/// </summary>
		bool SetPS( int identifier, unsigned int setSlot, ID3D11DeviceContext *pContext = nullptr );
		/// <summary>
		/// Deactivate current sampler-state.<para></para>
		/// If you set nullptr(default) to "pContext", I use default(library's) immediate-context.<para></para>
		/// Returns false if failed deactivation(maybe the identifier does not cache?).
		/// </summary>
		void ResetVS( unsigned int setSlot, ID3D11DeviceContext *pContext = nullptr );
		/// <summary>
		/// Deactivate current sampler-state.<para></para>
		/// If you set nullptr(default) to "pContext", I use default(library's) immediate-context.<para></para>
		/// Returns false if failed deactivation(maybe the identifier does not cache?).
		/// </summary>
		void ResetGS( unsigned int setSlot, ID3D11DeviceContext *pContext = nullptr );
		/// <summary>
		/// Deactivate current sampler-state.<para></para>
		/// If you set nullptr(default) to "pContext", I use default(library's) immediate-context.<para></para>
		/// Returns false if failed deactivation(maybe the identifier does not cache?).
		/// </summary>
		void ResetPS( unsigned int setSlot, ID3D11DeviceContext *pContext = nullptr );
		
		/// <summary>
		/// Release all a cached sampler-states. The identifiers of released state will be invalid.
		/// </summary>
		void ReleaseAllCachedStates();
	}
}
