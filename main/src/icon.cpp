#include <icon.hpp>

//========================================

Icon::Icon(std::span<const uint8_t> data)
{
	m_size.x = data[0];
	m_size.y = data[1];
	
	m_data = std::span<const uint8_t>(data.begin() + 2, data.end());
}

Icon::Icon(const uint8_t begin[], const uint8_t end[]):
	Icon(std::span(begin, end))
{}

//========================================

const Vector2u& Icon::getSize() const
{
	return m_size;
}

bool Icon::getPixel(const Vector2u& position) const
{
	size_t index = m_size.x * position.y + position.x;
	return (m_data[index / 8] >> (index % 8)) & 1;
}

//========================================