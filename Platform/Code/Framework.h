#pragma once

#include <memory>

#include "Donya/UseImGui.h"	// Use for USE_IMGUI macro.

#include "SceneManager.h"

class Framework
{
private:
	std::unique_ptr<SceneMng> pSceneMng = nullptr;
public:
	Framework()  = default;
	~Framework() = default;
	Framework( const Framework &  ) = delete;
	Framework(		 Framework && ) = delete;
	Framework &operator = ( const Framework &  ) = delete;
	Framework &operator = (		  Framework && ) = delete;
public:
	bool Init();
	void Uninit();

	// The "elapsedTime" is elapsed seconds from last frame.
	void Update( float elapsedTime );

	// The "elapsedTime" is elapsed seconds from last frame.
	void Draw( float elapsedTime );
private:
	bool LoadSounds();
	
#if USE_IMGUI
	void DebugShowInformation();
#endif // USE_IMGUI
};
