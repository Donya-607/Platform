#pragma once

#include "../Donya/Vector.h"

#include "../UI.h"

namespace Performer
{
	class LoadPart
	{
	public:
		class Icon
		{
		private:
			float			timer = 0.0f;
			Donya::Vector2	scale	{ 1.0f, 1.0f };
			Donya::Vector2	stretch	{ 1.0f, 1.0f };
			Donya::Vector2	posOffset{};
			Donya::Vector2	bounce{};
			UIObject		sprite;
		public:
			void Init();
			void Update( float elapsedTime );
			void Draw();
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
		public:
			void Init();
			void Update( float elapsedTime );
			void Draw();
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

		void Draw();
	public:
		void Start( const Donya::Vector2 &ssBasePos );
		void Stop();
	};
}
