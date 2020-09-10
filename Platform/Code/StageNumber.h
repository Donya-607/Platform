#pragma once

namespace Definition
{
	namespace StageNumber
	{
		namespace Impl
		{
			constexpr int title	= -1;
			constexpr int game	= 0;
		}

		constexpr int Title()	{ return Impl::title;	}
		constexpr int Game()	{ return Impl::game;	}
	}
}
