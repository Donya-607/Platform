#pragma once

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
			void Update( float elapsedTime );
			void Draw( float drawDepth );
		public:
			void Start( const Donya::Vector2 &ssBasePos );
			void Stop();
		};
		class String
		{
		private:
			float			timer = 0.0f;
			Donya::Vector2	scale{ 1.0f, 1.0f };
			Donya::Vector2	posOffset{};
			Donya::Vector2	pivot{ 0.5f, 0.5f };
			Donya::Vector2	basePos{};
			bool			active = false;
		public:
			void Init();
			void Update( float elapsedTime );
			void Draw( float drawDepth );
		public:
			void Start( const Donya::Vector2 &ssBasePos );
			void Stop();
		};
	private:
		float	timer		= 0.0f;
		float	BGMaskAlpha	= 1.0f;
		Icon	partIcon;
		String	partString;
	public:
		void Init();
		void Uninit();

		void Update( float elapsedTime );

		void Draw( float drawDepth );
	public:
		void Start( const Donya::Vector2 &ssBasePos );
		void Stop();
	};
}
