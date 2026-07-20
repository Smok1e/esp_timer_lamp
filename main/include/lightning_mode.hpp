#pragma once

#include <optional>
#include <string_view>

//========================================

enum class LightningMode: uint8_t
{
	Auto,
	On,
	Off
};

//========================================

const char* LightningModeToStr(LightningMode mode);
std::optional<LightningMode> StrToLightningMode(std::string_view string);

//========================================