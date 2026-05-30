#include <sh1106.hpp>

#include <cmath>
#include <cstring>

//========================================

uint16_t SH1106::s_max_width   = 132;
uint16_t SH1106::s_max_height  = 64;
size_t   SH1106::s_buffer_size = s_max_width * s_max_height;

//========================================

SH1106::~SH1106()
{
	heap_caps_free(m_pixel_data);
	ESP_ERROR_CHECK(spi_bus_remove_device(m_device_handle));
}

//========================================

void SH1106::init(
	spi_host_device_t spi_host,
	gpio_num_t pin_dc,
	gpio_num_t pin_reset,
	int spi_freq,
	uint16_t width, /*= 128*/
	uint16_t height /*= 64 */
)
{
	m_pin_dc    = pin_dc;
	m_pin_reset = pin_reset;
	m_width     = width;
	m_height    = height;
	
	setScissor();
	
	m_pixel_data = reinterpret_cast<uint8_t*>(heap_caps_malloc(s_buffer_size, MALLOC_CAP_DMA));
	
	spi_device_interface_config_t device_config = {};
	device_config.clock_speed_hz = spi_freq;
	device_config.mode = 0;
	device_config.spics_io_num = GPIO_NUM_NC;
	device_config.queue_size = 1;
	ESP_ERROR_CHECK(spi_bus_add_device(spi_host, &device_config, &m_device_handle));

	if (m_pin_reset != GPIO_NUM_NC)
	{
		ESP_ERROR_CHECK(gpio_reset_pin(m_pin_reset));
		ESP_ERROR_CHECK(gpio_set_direction(m_pin_reset, GPIO_MODE_OUTPUT));
		ESP_ERROR_CHECK(gpio_set_level(m_pin_reset, 0));
		vTaskDelay(pdMS_TO_TICKS(10));
		ESP_ERROR_CHECK(gpio_set_level(m_pin_reset, 1));
	}
	
	ESP_ERROR_CHECK(gpio_reset_pin(m_pin_dc));
	ESP_ERROR_CHECK(gpio_set_direction(m_pin_dc, GPIO_MODE_OUTPUT));

	sendCommand(Command::SetCommonOutputScanDirection | 8   );
	sendCommand(Command::SetSegmentRemap              | true);
	sendCommand(Command::SetDisplayOn                 | true);
	
	vTaskDelay(pdMS_TO_TICKS(100));
	
	clear();
	flush();
}

void SH1106::flush()
{
	constexpr size_t pages = 8;
	for (size_t page = 0; page < pages; page++)
	{
		sendCommand(Command::SetPageAddress | page);
		setColumnAddress(0);
		
		spi_transaction_t transaction = {};
		transaction.tx_buffer = m_pixel_data + page * s_max_width;
		transaction.length = s_max_width * 8;
		
		sendCommand(Command::SetReadModifyWriteStart);
		ESP_ERROR_CHECK(gpio_set_level(m_pin_dc, true));
		ESP_ERROR_CHECK(spi_device_transmit(m_device_handle, &transaction));
		sendCommand(Command::SetReadModifyWriteEnd);
	}
}

std::pair<uint16_t, uint16_t> SH1106::getSize() const
{
	return {m_width, m_height};
}

bool SH1106::setPixel(int x, int y, bool value)
{
	if (!(m_scissor_src_x <= x && x < m_scissor_dst_x && m_scissor_src_y <= y && y < m_scissor_dst_y))
		return false;
	
	auto pixel = (y + (s_max_height - m_height) / 2) * s_max_width + (x + (s_max_width - m_width) / 2);
	
	auto byte = pixel % s_max_width + pixel / (8 * s_max_width) * s_max_width;
	auto bit = (pixel / s_max_width) % 8;
	
	(m_pixel_data[byte] &= ~(1 << bit)) |= (value << bit);
	return true;
}

void SH1106::setDisplayOn(bool on)
{
	m_on = on;
	sendCommand(Command::SetDisplayOn | on);
}

bool SH1106::isDisplayOn() const
{
	return m_on;
}

void SH1106::setContrast(uint8_t contrast)
{
	sendCommand(Command::SetContrastControlMode);
	sendCommand(Command::SetContrast | (m_contrast = contrast));
}

uint8_t SH1106::getContrast() const
{
	return m_contrast;
}

void SH1106::setInverted(bool value)
{
	sendCommand(Command::SetReverseMode | (m_inverted = value));
}

bool SH1106::isInverted() const
{
	return m_inverted;
}

void SH1106::setScissor(int x, int y, int width, int height)
{
	m_scissor_src_x = x;
	m_scissor_src_y = y;
	m_scissor_dst_x = x + width;
	m_scissor_dst_y = y + height;
}

void SH1106::setScissor()
{
	m_scissor_src_x = 0;
	m_scissor_src_y = 0;
	m_scissor_dst_x = m_width;
	m_scissor_dst_y = m_height;
}

void SH1106::clear(bool value /*= false*/)
{
	std::fill(m_pixel_data, m_pixel_data + s_buffer_size, value * ~0);
}

const uint8_t* SH1106::getPixelData() const
{
	return m_pixel_data;
}

//========================================

void SH1106::sendCommand(uint8_t cmd)
{
	gpio_set_level(m_pin_dc, false);
	
	spi_transaction_t transaction = {};
	transaction.tx_data[0] = cmd;
	transaction.flags = SPI_TRANS_USE_TXDATA;
	transaction.length = 8;
	
	ESP_ERROR_CHECK(spi_device_transmit(m_device_handle, &transaction));
}

void SH1106::setColumnAddress(uint8_t column)
{
	sendCommand(Command::SetColumnAddressLowerBits  | (column & 0x0F)     );
	sendCommand(Command::SetColumnAddressHigherBits | (column & 0xF0) >> 4);
}

//========================================