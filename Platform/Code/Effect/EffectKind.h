#pragma once

namespace Effect
{
	enum class Kind
	{
		CatchItem,
		Charge_Complete,
		Charge_Loop,
		Charge_Loop_Charged,
		ChargedBustersTracing,
		Death,
		DefeatEnemy_Small,
		Hit_Buster,
		Hit_ChargedBuster,
		Hit_Shoryuken,
		HurtDamage,
		Player_Appear,
		Player_Leave,
		Player_Slide_Begin,
		READY,
		Protected,

		KindCount
	};
	constexpr const char *GetEffectName( Kind attr )
	{
		switch ( attr )
		{
		case Effect::Kind::CatchItem:				return "CatchItem";
		case Effect::Kind::Charge_Complete:			return "Charge_Complete";
		case Effect::Kind::Charge_Loop:				return "Charge_Loop";
		case Effect::Kind::Charge_Loop_Charged:		return "Charge_Loop_Charged";
		case Effect::Kind::ChargedBustersTracing:	return "ChargedBustersTracing";
		case Effect::Kind::Death:					return "Death";
		case Effect::Kind::DefeatEnemy_Small:		return "DefeatEnemy_Small";
		case Effect::Kind::Hit_Buster:				return "Hit_Buster";
		case Effect::Kind::Hit_ChargedBuster:		return "Hit_ChargedBuster";
		case Effect::Kind::Hit_Shoryuken:			return "Hit_Shoryuken";
		case Effect::Kind::HurtDamage:				return "HurtDamage";
		case Effect::Kind::Player_Appear:			return "Player_Appear";
		case Effect::Kind::Player_Leave:			return "Player_Leave";
		case Effect::Kind::Player_Slide_Begin:		return "Player_Slide_Begin";
		case Effect::Kind::READY:					return "READY";
		case Effect::Kind::Protected:				return "Protected";
		default: break;
		}

		// Fail safe
		return "ERROR_ATTR";
	}
}
