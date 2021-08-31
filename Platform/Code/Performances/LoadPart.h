#pragma once

#include "../Donya/Color.h"
#include "../Donya/Vector.h"

#include "../UI.h"

namespace Performer
{
	class LoadPart
	{
	public:
		static void LoadParameter();
	#if USE_IMGUI
		static void ShowImGuiNode( const std::string &nodeCaption );
	#endif // USE_IMGUI
	public:
		class Icon
		{
		private:
			float			timer = 0.0f;
			Donya::Vector2	stretch	{ 1.0f, 1.0f };
			Donya::Vector2	bounce{};
			Donya::Vector2	basePos{};
			UIObject		sprite;
			bool			active = false;
		public:
			void Init();
			void Update( float deltaTime );
			void Draw( float drawDepth, float drawAlpha );
		public:
			void Start( const Donya::Vector2 &ssBasePos );
			void Stop();
		};
		class String
		{
		private:
			float			timer = 0.0f;
			std::vector<float> popWeights; // 0.0f[stay] ~ 1.0f[pop]. size() == string-length
			Donya::Vector2	scale{ 1.0f, 1.0f };
			Donya::Vector2	posOffset{};
			Donya::Vector2	pivot{ 0.5f, 0.5f };
			Donya::Vector2	basePos{};
			bool			active = false;
		public:
			void Init();
			void Update( float deltaTime );
			void Draw( float drawDepth, float drawAlpha );
		public:
			void Start( const Donya::Vector2 &ssBasePos );
			void Stop();
		};
	private:
		float	timer		= 0.0f;
		float	alpha		= 1.0f;
		Icon	partIcon;
		String	partString;
		bool	active		= false;
		Donya::Vector3 maskColor{ 0.0f, 0.0f, 0.0f };
	public:
		void Init();
		void Uninit();
		void UpdateIfActive( float deltaTime );
		void DrawIfActive( float drawDepth );
	public:
		void Start( const Donya::Vector2 &ssBasePos, const Donya::Color::Code &color );
		void Stop();
	};
}
