#pragma once

namespace Effect
{
	enum class Kind
	{
		CatchItem,
		Charge_Complete,
		Charge_Loop,
		Death,
		DefeatEnemy_Small,
		Hit_Buster,
		HurtDamage,
		Player_Slide_Begin,

		KindCount
	};
	constexpr const char *GetEffectName( Kind attr )
	{
		switch ( attr )
		{
		case Effect::Kind::CatchItem:			return "CatchItem";
		case Effect::Kind::Charge_Complete:		return "Charge_Complete";
		case Effect::Kind::Charge_Loop:			return "Charge_Loop";
		case Effect::Kind::Death:				return "Death";
		case Effect::Kind::DefeatEnemy_Small:	return "DefeatEnemy_Small";
		case Effect::Kind::Hit_Buster:			return "Hit_Buster";
		case Effect::Kind::HurtDamage:			return "HurtDamage";
		case Effect::Kind::Player_Slide_Begin:	return "Player_Slide_Begin";
		default: break;
		}

		// Fail safe
		return "ERROR_ATTR";
	}
}
