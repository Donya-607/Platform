#pragma once

namespace Effect
{
	enum class Kind
	{
		ChargeContinue,

		KindCount
	};
	constexpr const char *GetEffectName( Kind attr )
	{
		switch ( attr )
		{
		case Effect::Kind::ChargeContinue:	return "Charge_Continue";
		default: break;
		}

		// Fail safe
		return "ERROR_ATTR";
	}
}
