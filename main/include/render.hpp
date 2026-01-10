#pragma once

#include <string_view>

#include <sh1106.hpp>
#include <vector2.hpp>
#include <font.hpp>
#include <icon.hpp>

//========================================

namespace render
{

namespace RoundedRectangleStyle
{

//========================================

enum: uint8_t
{
	LeftTop     = 0b000000001,
	LeftBottom  = 0b000000010,
	RightTop    = 0b000000100,
	RightBottom = 0b000001000,
	
	Outline     = 0b000010000,
	
	Left   = LeftTop    | LeftBottom,
	Right  = RightTop   | RightBottom,
	Top    = LeftTop    | RightTop,
	Bottom = LeftBottom | RightBottom,
	
	All = LeftTop | LeftBottom | RightTop | RightBottom,
	
	Default = All
};

//========================================
	
} // namespace RoundedRectangleStyle

//========================================

void Rectangle(
	SH1106& display,
	const Vector2i& position,
	const Vector2i& size,
	bool value = true,
	bool fill = true
);

void RoundedRectangle(
	SH1106&         display,
	const Vector2i& position,
	const Vector2i& size,
	int             radius,
	bool            value = true,
	uint8_t         style = RoundedRectangleStyle::Default
);

void Circle(
	SH1106& display,
	const Vector2f& center,
	float radius,
	bool value = true
);

void Line(
	SH1106& display,
	Vector2i a,
	Vector2i b,
	bool value = true
);

void Character(
	SH1106& display,
	const Font& font,
	const Vector2i& position,
	char ch,
	bool value = true
);

void Text(
	SH1106& display,
	const Font& font,
	const Vector2i& position,
	std::string_view text,
	bool value = true,
	bool fill  = false
);

void Icon(
	SH1106& display,
	const class Icon& icon,
	const Vector2i& position,
	bool value = true,
	bool fill  = false
);

//========================================

template<typename... Args>
std::string_view FormatTmp(const char* format, const Args&... args)
{
	static char buffer[128] = "";
	
	return std::string_view(
		buffer,
		snprintf(buffer, std::size(buffer), format, args...)
	);
}

//========================================

} // namespace render

//========================================