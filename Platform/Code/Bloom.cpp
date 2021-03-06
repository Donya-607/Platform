#include "Bloom.h"

#include "Donya/Blend.h"
#include "Donya/Color.h"
#include "Donya/RenderingStates.h"

#include "Donya/Useful.h"

#if USE_IMGUI
void  BloomApplier::Parameter::ShowImGuiNode( const std::string &nodeCaption )
{
	if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
	// else

	ImGui::SliderFloat	( u8"輝度抽出の閾値（下限）",	&luminanceThreshold, 0.0f, 3.0f );
	ImGui::DragFloat	( u8"標準偏差",				&blurDeviation, 0.01f );
	ImGui::SliderInt	( u8"ぼかし回数",			&blurSampleCount, 1, maxBlurSampleCount );

	blurDeviation = std::max( 0.01f, blurDeviation );

	ImGui::TreePop();
}
#endif // USE_IMGUI

bool  BloomApplier::Init( const Donya::Int2 &wholeScreenSize )
{
	if ( wasInitialized ) { return true; }
	// else

	bool succeeded = true;

	applicationScreenSize	= wholeScreenSize;
	baseSurfaceSize.x		= applicationScreenSize.x >> 2;
	baseSurfaceSize.y		= applicationScreenSize.y >> 2;

	// Create blur-buffers
	{
		constexpr const wchar_t *createErrorMessage = L"Failed: Create a blur-buffer.";

		auto Create = []( Donya::Surface *p, const Donya::Int2 &wholeSize, const DXGI_FORMAT &format )
		{
			const bool result = p->Init
			(
				wholeSize.x,
				wholeSize.y,
				format
			);

			return result;
		};

		if ( !Create( &highLuminanceSurface, applicationScreenSize, blurBufferFormat ) )
		{
			_ASSERT_EXPR( 0, createErrorMessage );
			succeeded = false;
		}

		Donya::Int2 bufferSize = baseSurfaceSize;
		blurBuffers.resize( blurBufferCount );
		for ( size_t i = 0; i < blurBufferCount; ++i )
		{
			if ( !Create( &blurBuffers[i].first, bufferSize, blurBufferFormat ) )
			{
				_ASSERT_EXPR( 0, createErrorMessage );
				succeeded = false;
			}
			if ( !Create( &blurBuffers[i].second, bufferSize, blurBufferFormat ) )
			{
				_ASSERT_EXPR( 0, createErrorMessage );
				succeeded = false;
			}

			bufferSize.x >>= 1;
			bufferSize.y >>= 1;
		}
	}

	if ( !display.Init() )
	{
		succeeded = false;
	}

	// Create shaders
	{
		constexpr const char *VSPath			= "./Data/Shaders/DisplayQuadVS.cso";
		constexpr const char *PSLuminancePath	= "./Data/Shaders/ExtractLuminancePS.cso";
		constexpr const char *PSBlurPath		= "./Data/Shaders/ApplyBlurPS.cso";
		constexpr const char *PSCombinePath		= "./Data/Shaders/DrawBlurBuffersPS.cso";
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
		blurBuffers[i].second.SetRenderTargetShaderResourcePS( i + startIndex );
	}
}
void  BloomApplier::ResetShaderResourcesPS( size_t startIndex )
{
	for ( size_t i = 0; i < blurBufferCount; ++i )
	{
		blurBuffers[i].second.ResetShaderResourcePS( i + startIndex );
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
		it.first.Clear( clearColor );
		it.second.Clear( clearColor );
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
	Donya::Sampler::SetPS( Donya::Sampler::Defined::Linear_Border_Black, 0U );

	highLuminanceSurface.SetViewport();

	Donya::Surface *pSourceSurface = &highLuminanceSurface;
	Donya::Surface *pOutputSurface = &blurBuffers[0].first;
	Donya::Vector2	blurSize = baseSurfaceSize.Float();

	auto DrawProcess = [&]( const Donya::Vector2 &blurDirection )
	{
		if ( !pSourceSurface || !pOutputSurface ) { return; }
		// else

		pOutputSurface->Clear( Donya::Color::Code::BLACK );
		pOutputSurface->SetRenderTarget();

		pSourceSurface->SetRenderTargetShaderResourcePS( 0U );

		UpdateGaussianBlurParams( blurSize.x, blurSize.y, blurDirection );
		cbBlur.Activate( 0U, false, true );
		display.Draw
		(
			pOutputSurface->GetSurfaceSizeF(),
			Donya::Vector2::Zero()
		);
		cbBlur.Deactivate();

		pSourceSurface->ResetShaderResourcePS( 0U );

		Donya::Surface::ResetRenderTarget();
	};

	constexpr auto blurHorizontal	= Donya::Vector2::Right();
	constexpr auto blurVertical		= Donya::Vector2::Up();

	pSourceSurface = &highLuminanceSurface;
	pOutputSurface = &blurBuffers[0].first;
	DrawProcess( blurHorizontal	);

	pSourceSurface = &blurBuffers[0].first;
	pOutputSurface = &blurBuffers[0].second;
	DrawProcess( blurVertical	);

	blurSize *= 0.5f;

	pSourceSurface = &blurBuffers[0].second;
	pOutputSurface = &blurBuffers[1].first;

	// "pp" -> PingPong
	auto &pp = blurBuffers;
	for ( size_t i = 0; i < blurBufferCount - 1; ++i )
	{
		/*
		Source = [0].second;
		Output = [1].first;
		Draw( H )
		Source = [1].first;
		Output = [1].second;
		Draw( V )

		Source = [1].second;
		Output = [2].first;
		Draw( H )
		Source = [2].first;
		Output = [2].second;
		Draw( V )

		.
		.
		.
		*/

		pSourceSurface = &blurBuffers[i + 0].second;
		pOutputSurface = &blurBuffers[i + 1].first;
		DrawProcess( blurHorizontal	);

		pSourceSurface = &blurBuffers[i + 1].first;
		pOutputSurface = &blurBuffers[i + 1].second;
		DrawProcess( blurVertical	);

		blurSize *= 0.5f;
	}

	pSourceSurface = nullptr;
	pOutputSurface = nullptr;

	Donya::Sampler::ResetPS( 0U );
	PSBlur.Deactivate();
	VS.Deactivate();
}
void  BloomApplier::DrawBlurBuffers( const Donya::Vector2 &drawingSize )
{
	const Donya::Vector2 drawSize = ( drawingSize.IsZero() ) ? applicationScreenSize.Float() : drawingSize;

	VS.Activate();
	PSCombine.Activate();

	SetShaderResourcesPS( 0U );
	Donya::Sampler::SetPS( Donya::Sampler::Defined::Linear_Border_Black, 0 );
	
	display.Draw( drawSize, Donya::Vector2::Zero() );

	Donya::Sampler::ResetPS( 0 );
	ResetShaderResourcesPS( 0U );

	PSCombine.Deactivate();
	VS.Deactivate();
}
float BloomApplier::CalcGaussianWeight( const Donya::Vector2 &pos )
{
	const float &deviation = parameter.blurDeviation;
	return expf
	(
		-( pos.x*pos.x + pos.y*pos.y )
		/ // ---------------------
		( 2.0f * deviation*deviation )
	);
}
void  BloomApplier::UpdateGaussianBlurParams( float bufferWholeWidth, float bufferWholeHeight, const Donya::Vector2 &unitBlurDirection )
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
	data.params[0].distribution	= CalcGaussianWeight( Donya::Vector2::Zero() );
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
		data.params[i].distribution = CalcGaussianWeight( unitBlurDirection * floatIndex );
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
#if USE_IMGUI
void  BloomApplier::DrawHighLuminanceToImGui( const Donya::Vector2 &drawSize )
{
	highLuminanceSurface.DrawRenderTargetToImGui( drawSize );
}
void  BloomApplier::DrawBlurBuffersToImGui( const Donya::Vector2 &drawSize )
{
	for ( const auto &it : blurBuffers )
	{
		it.first.DrawRenderTargetToImGui( drawSize );
		ImGui::SameLine();
		it.second.DrawRenderTargetToImGui( drawSize );
	}
}
#endif // USE_IMGUI
