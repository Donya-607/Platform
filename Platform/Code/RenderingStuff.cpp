#include "RenderingStuff.h"

#include <vector>

#include "Donya/Vector.h"

#include "Common.h"

bool RenderingStuffInstance::Initialize()
{
	if ( ptr ) { return true; }
	// else

	ptr = std::make_unique<RenderingStuff>();

	constexpr Donya::Int2 wholeScreenSize
	{
		Common::ScreenWidth(),
		Common::ScreenHeight(),
	};

	bool succeeded = true;

	if ( !ptr->renderer	.Init() )					{ succeeded = false; }
	if ( !ptr->displayer.Init() )					{ succeeded = false; }
	if ( !ptr->bloomer	.Init( wholeScreenSize ) )	{ succeeded = false; }
	if ( !CreateShaders	() )						{ succeeded = false; }
	if ( !CreateSurfaces() )						{ succeeded = false; }

	if ( !succeeded )
	{
		// Make not loaded state
		ptr.reset();
	}

	return succeeded;
}

void RenderingStuffInstance::AssignBloomParameter( const BloomApplier::Parameter &parameter )
{
	if ( !ptr ) { return; }
	// else

	ptr->bloomer.AssignParameter( parameter );
}
void RenderingStuffInstance::ClearBuffers()
{
	if ( !ptr ) { return; }
	// else

	constexpr Donya::Vector4 clearColor{ 0.0f, 0.0f, 0.0f, 1.0f };
	ptr->bloomer		.ClearBuffers( clearColor );
	ptr->screenSurface	.Clear( clearColor );
	ptr->shadowMap		.Clear( clearColor );
}

bool RenderingStuffInstance::CreateShaders()
{
	constexpr const char *VSPath = "./Data/Shaders/DisplayQuadVS.cso";
	constexpr const char *PSPath = "./Data/Shaders/DisplayQuadPS.cso";
	constexpr auto IEDescs = Donya::Displayer::Vertex::GenerateInputElements();

	// The vertex shader requires IE-descs as std::vector<>
	const std::vector<D3D11_INPUT_ELEMENT_DESC> IEDescsV{ IEDescs.begin(), IEDescs.end() };

	bool succeeded = true;

	if ( !ptr->quadShader.VS.CreateByCSO( VSPath, IEDescsV	) ) { succeeded = false; }
	if ( !ptr->quadShader.PS.CreateByCSO( PSPath			) ) { succeeded = false; }

	return succeeded;
}
bool RenderingStuffInstance::CreateSurfaces()
{
	if ( !ptr ) { return false; }
	// else

	constexpr Donya::Int2 wholeScreenSize
	{
		Common::ScreenWidth(),
		Common::ScreenHeight(),
	};

	bool succeeded	= true;
	bool result		= true;

	result = ptr->screenSurface.Init
	(
		wholeScreenSize.x,
		wholeScreenSize.y,
		DXGI_FORMAT_R16G16B16A16_FLOAT
	);
	if ( !result ) { succeeded = false; }

	result = ptr->shadowMap.Init
	(
		wholeScreenSize.x << 1,
		wholeScreenSize.y << 1,
		DXGI_FORMAT_R32_FLOAT,		true,
		DXGI_FORMAT_R32_TYPELESS,	true
	);
	if ( !result ) { succeeded = false; }

	return succeeded;
}
