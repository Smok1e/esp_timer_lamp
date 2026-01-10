#pragma once

#include <span>

#include <vector2.hpp>

//========================================

class Icon
{
public:
	explicit Icon(std::span<const uint8_t> data);
	Icon(const uint8_t begin[], const uint8_t end[]);
	Icon(const Icon& copy) = delete;
	
	const Vector2u& getSize() const;
	bool getPixel(const Vector2u& position) const;
	
private:
	Vector2u m_size {};
	std::span<const uint8_t> m_data {};
	
};

//========================================