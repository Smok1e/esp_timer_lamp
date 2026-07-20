#include <cstring>

#include <lightning_mode.hpp>

//========================================

const char* LightningModeToStr(LightningMode mode)
{
	switch (mode)
	{
		case LightningMode::Auto: return "auto";
		case LightningMode::On:   return "on";
		case LightningMode::Off:  return "off";
	}
	
	return "";
}

std::optional<LightningMode> StrToLightningMode(std::string_view string)
{
	if (string == "auto")
		return LightningMode::Auto;
	
	if (string == "on")
		return LightningMode::On;

	if (string == "off")
		return LightningMode::Off;
	
	return std::nullopt;
}

//========================================