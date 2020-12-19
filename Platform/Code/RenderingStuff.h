#pragma once

#include <memory>

#include "Donya/Displayer.h"
#include "Donya/Shader.h"
#include "Donya/Surface.h"
#include "Donya/Template.h"

#include "Bloom.h"
#include "Renderer.h"


struct RenderingStuff
{
public:
	struct Shader
	{
		Donya::VertexShader	VS;
		Donya::PixelShader	PS;
	};
public:
	BloomApplier		bloomer;
	Donya::Displayer	displayer;
	Shader				quadShader;
	RenderingHelper		renderer;
	Donya::Surface		screenSurface;
	Donya::Surface		shadowMap;
};


class RenderingStuffInstance : public Donya::Singleton<RenderingStuffInstance>
{
	friend Donya::Singleton<RenderingStuffInstance>;
private:
	std::unique_ptr<RenderingStuff> ptr = nullptr;
private:
	RenderingStuffInstance() = default;
public:
	bool Initialize();
	void AssignBloomParameter( const BloomApplier::Parameter &parameter );
	void ClearBuffers();
	RenderingStuff *Ptr()
	{
		return ptr.get();
	}
private:
	bool CreateShaders();
	bool CreateSurfaces();
};
