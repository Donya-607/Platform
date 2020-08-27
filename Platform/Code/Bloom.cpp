#include "Bloom.h"

#include "Donya/Blend.h"
#include "Donya/Color.h"
#include "Donya/RenderingStates.h"

#if USE_IMGUI
void  BloomApplier::Parameter::ShowImGuiNode( const std::string &nodeCaption )
{
	if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
	// else

	ImGui::SliderFloat	( u8"輝度抽出の閾値（下限）",	&luminanceThreshold, 0.0f, 3.0f );
	ImGui::DragFloat	( u8"ぼかし範囲",			&blurRange, 0.01f );
	ImGui::SliderInt	( u8"ぼかし回数",			&blurSampleCount, 1, maxBlurSampleCount );

	blurRange = std::max( 0.0f, blurRange );

	ImGui::TreePop();
}
#endif // USE_IMGUI

bool  BloomApplier::Init( const Donya::Int2 &wholeScreenSize )
{
	if ( wasInitialized ) { return true; }
	// else

	bool succeeded = true;

	applicationScreenSize = wholeScreenSize;
	baseSurfaceSize.x = applicationScreenSize.x >> 1;
	baseSurfaceSize.y = applicationScreenSize.y >> 1;

	// Create blur-buffers
	{
		blurBuffers.resize( blurBufferCount );
		for ( size_t i = 0; i < blurBufferCount; ++i )
		{
			const bool result = blurBuffers[i].Init
			(
				baseSurfaceSize.x >> i,
				baseSurfaceSize.y >> i,
				blurBufferFormat
			);
			if ( !result )
			{
				_ASSERT_EXPR( 0, L"Failed: Create a blur-buffer." );
				succeeded = false;
			}
			else
			{
				blurBuffers[i].Clear( Donya::Color::Code::BLACK );
			}
		}

		const bool result = highLuminanceSurface.Init
		(
			applicationScreenSize.x,
			applicationScreenSize.y,
			blurBufferFormat
		);
		if ( !result )
		{
			succeeded = false;
		}
		else
		{
			highLuminanceSurface.Clear( Donya::Color::Code::BLACK );
		}
	}

	if ( !display.Init() )
	{
		succeeded = false;
	}

	// Create shaders
	{
		constexpr const char *VSPath			= "./Data/Shader/DisplayQuadVS.cso";
		constexpr const char *PSLuminancePath	= "./Data/Shader/ExtractLuminancePS.cso";
		constexpr const char *PSBlurPath		= "./Data/Shader/ApplyBlurPS.cso";
		constexpr const char *PSCombinePath		= "./Data/Shader/DrawBlurBuffersPS.cso";
		constexpr auto IEDescs = Donya::Displayer::Vertex::GenerateInputElements();

		// The vertex shader requires IE-descs as std::vector<>
		const std::vector<D3D11_INPUT_ELEMENT_DESC> IEDescsV{ IEDescs.begin(), IEDescs.end() };
		if ( !VS.CreateByCSO( VSPath, IEDescsV ) ) { succeeded = false; }

		if ( !PSHighLuminance	.CreateByCSO( PSLuminancePath	) ) { succeeded = false; }
		if ( !PSBlur			.CreateByCSO( PSBlurPath		) ) { succeeded = false; }
		if ( !PSCombine			.CreateByCSO( PSCombinePath		) ) { succeeded = false; }
	}

	// Create cbuffers
	{
		if ( !cbHighLuminance.Create()	) { succeeded = false; }
		if ( !cbBlur.Create()			) { succeeded = false; }
	}

	wasInitialized = succeeded;
	return succeeded;
}
void  BloomApplier::SetShaderResourcesPS( size_t startIndex )
{
	for ( size_t i = 0; i < blurBufferCount; ++i )
	{
		blurBuffers[i].SetRenderTargetShaderResourcePS( i + startIndex );
	}
}
void  BloomApplier::ResetShaderResourcesPS( size_t startIndex )
{
	for ( size_t i = 0; i < blurBufferCount; ++i )
	{
		blurBuffers[i].ResetShaderResourcePS( i + startIndex );
	}
}
float BloomApplier::CalcGaussianWeight( const Donya::Vector2 &pos, float deviation )
{
	return expf
	(
		-( pos.x*pos.x + pos.y*pos.y )
		/ // ---------------------
		( 2.0f * deviation*deviation )
	);
}
void  BloomApplier::UpdateGaussianBlurParams( float bufferWholeWidth, float bufferWholeHeight, const Donya::Vector2 &unitBlurDirection, float multiply )
{
	// See: http://project-asura.com/program/d3d11/d3d11_010.html

	if ( IsZero( bufferWholeWidth ) || IsZero( bufferWholeHeight ) )
	{
		_ASSERT_EXPR( 0, L"Error: Zero-division detected!" );
		return;
	}
	// else

	const float texCoordU = 1.0f / bufferWholeWidth;
	const float texCoordV = 1.0f / bufferWholeHeight;

	auto &data = cbBlur.data;

	float totalWeight = 0.0f;

	// [0] is center
	data.params[0].uvOffset.x	= 0.0f;
	data.params[0].uvOffset.y	= 0.0f;
	data.params[0].distribution	= CalcGaussianWeight( Donya::Vector2::Zero(), parameter.blurRange ) * multiply;
	totalWeight = data.params[0].distribution;

	// It sampling toward positive side,
	// then assign negated value to negative side.
	const int halfSampleCount = parameter.blurSampleCount >> 1;

	// Calc weights of positive side
	for ( int i = 0 + 1/* Ignore center */; i < halfSampleCount; ++i )
	{
		const float floatIndex = scast<float>( i );

		data.params[i].uvOffset.x	= unitBlurDirection.x * texCoordU * floatIndex;
		data.params[i].uvOffset.y	= unitBlurDirection.y * texCoordV * floatIndex;
		data.params[i].distribution = CalcGaussianWeight( unitBlurDirection * floatIndex, parameter.blurRange ) * multiply;
		totalWeight += data.params[i].distribution * 2.0f; // Also consider the negative side
	}

	// Normalize the weights
	if ( !IsZero( totalWeight ) )
	{
		for ( int i = 0; i < halfSampleCount; ++i )
		{
			data.params[i].distribution /= totalWeight;
		}
	}

	// Assign to negative side
	const int offsetToPositive = halfSampleCount - 1;
	for ( int i = halfSampleCount; i < parameter.blurSampleCount; ++i )
	{
		const int positiveIndex = ( i < offsetToPositive ) ? 0 : i - offsetToPositive; // Prevent to be negative
		auto &negativeSide = data.params[i];
		auto &positiveSide = data.params[positiveIndex];
		negativeSide.uvOffset		= -positiveSide.uvOffset;
		negativeSide.distribution	= positiveSide.distribution;
	}
}
void  BloomApplier::AssignParameter( const Parameter &newParameter )
{
	parameter = newParameter;
}
void  BloomApplier::ClearBuffers( const Donya::Vector4 &clearColor )
{
	for ( auto &it : blurBuffers )
	{
		it.Clear( clearColor );
	}
};
void  BloomApplier::WriteLuminance( const Donya::Surface &sceneSurface )
{
	VS.Activate();

	cbHighLuminance.data.threshold = parameter.luminanceThreshold;

	PSHighLuminance.Activate();
	cbHighLuminance.Activate( 0U, false, true );

	highLuminanceSurface.SetRenderTarget();
	highLuminanceSurface.SetViewport();
	// sceneSurface.SetViewport();

	sceneSurface.SetRenderTargetShaderResourcePS( 0U );

	display.Draw
	(
		sceneSurface.GetSurfaceSizeF(),
		Donya::Vector2::Zero()
	);

	sceneSurface.ResetShaderResourcePS( 0U );

	Donya::Surface::ResetRenderTarget();

	cbHighLuminance.Deactivate();
	PSHighLuminance.Deactivate();

	VS.Deactivate();
}
void  BloomApplier::WriteBlur()
{
	if ( blurBuffers.empty() ) { return; }
	// else

	VS.Activate();
	PSBlur.Activate();

	Donya::Surface *pSourceSurface = &highLuminanceSurface;
	Donya::Surface *pOutputSurface = &blurBuffers[0];

	pSourceSurface->SetViewport();

	Donya::Vector2	blurSize = baseSurfaceSize.Float();
	float			multiply = 1.0f;

	auto DrawProcess = [&]( const Donya::Vector2 &blurDirection )
	{
		if ( !pSourceSurface || !pOutputSurface ) { return; }
		// else

		pOutputSurface->Clear( Donya::Color::Code::BLACK );
		pOutputSurface->SetRenderTarget();

		pSourceSurface->SetRenderTargetShaderResourcePS( 0U );
		Donya::Sampler::SetPS( Donya::Sampler::Defined::Linear_Border_Black, 0U );

		UpdateGaussianBlurParams( blurSize.x, blurSize.y, blurDirection, multiply );
		cbBlur.Activate( 0U, false, true );
		display.Draw
		(
			pOutputSurface->GetSurfaceSizeF(),
			Donya::Vector2::Zero()
		);
		cbBlur.Deactivate();

		pSourceSurface->ResetShaderResourcePS( 0U );
		Donya::Sampler::ResetPS( 0U );

		Donya::Surface::ResetRenderTarget();
	};

	constexpr auto blurRight	= Donya::Vector2::Right();
	constexpr auto blurUp		= Donya::Vector2::Up();

	DrawProcess( blurRight );
	pSourceSurface = &blurBuffers[0];
	pOutputSurface = &blurBuffers[1];
	DrawProcess( blurUp );

	blurSize *= 0.5f;
	multiply *= 2.0f;

	pSourceSurface = &blurBuffers[1];
	pOutputSurface = &blurBuffers[2];

	for ( size_t i = 1; i < ( blurBufferCount >> 1 ); ++i )
	{
		DrawProcess( blurRight );
		pSourceSurface = &blurBuffers[i*2 + 0];
		pOutputSurface = &blurBuffers[i*2 + 1];
		DrawProcess( blurUp );

		blurSize *= 0.5f;
		multiply *= 2.0f;

		if ( i*2 + 2 < blurBufferCount )
		{
			pSourceSurface = &blurBuffers[i*2 + 1];
			pOutputSurface = &blurBuffers[i*2 + 2];
		}
	}

	pSourceSurface = nullptr;
	pOutputSurface = nullptr;

	PSBlur.Deactivate();
	VS.Deactivate();
}
void  BloomApplier::DrawBlurBuffersByAddBlend( const Donya::Vector2 &drawingSize )
{
	const Donya::Vector2 drawSize = ( drawingSize.IsZero() ) ? applicationScreenSize.Float() : drawingSize;

	VS.Activate();
	PSCombine.Activate();

	Donya::Sampler::SetPS( Donya::Sampler::Defined::Linear_Border_Black, 0 );
	Donya::Blend::Activate( Donya::Blend::Mode::ADD_NO_ATC );

	display.Draw( drawSize, Donya::Vector2::Zero() );

	Donya::Blend::Activate( Donya::Blend::Mode::ALPHA_NO_ATC );
	Donya::Sampler::ResetPS( 0 );

	PSCombine.Deactivate();
	VS.Deactivate();
}
#if USE_IMGUI
void  BloomApplier::DrawHighLuminanceToImGui( const Donya::Vector2 &drawSize )
{
	highLuminanceSurface.DrawRenderTargetToImGui( drawSize );
}
void  BloomApplier::DrawBlurBuffersToImGui( const Donya::Vector2 &drawSize )
{
	for ( const auto &it : blurBuffers )
	{
		it.DrawRenderTargetToImGui( drawSize );
	}
}
#endif // USE_IMGUI
