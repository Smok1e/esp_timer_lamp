#pragma once

#include <utility>

#include <freertos/FreeRTOS.h>

#include <esp_log.h>

#include <driver/spi_master.h>
#include <driver/spi_common.h>
#include <driver/gpio.h>

//========================================

class SH1106
{
public:
	SH1106() = default;
	SH1106(const SH1106& copy) = delete;
	~SH1106();
	
	void init(
		spi_host_device_t spi_host,
		gpio_num_t pin_dc,
		gpio_num_t pin_reset,
		int spi_freq,
		uint16_t width = 128,
		uint16_t height = 64
	);
	
	void flush();
	
	std::pair<uint16_t, uint16_t> getSize() const;
	
	bool setPixel(int x, int y, bool value);
	
	void setContrast(uint8_t contrast);
	uint8_t getContrast() const;
	
	void setInverted(bool value);
	bool isInverted() const;
	
	void clear(bool value = false);
	
	const uint8_t* getPixelData() const;
	
private:
	enum Command
	{
		SetColumnAddressLowerBits              = 0b00000000, // 4 lower bits interpreted as 4 lower bits of column address of display RAM
		SetColumnAddressHigherBits             = 0b00010000, // 4 lower bits interpreted as 4 higher bits of column address of display RAM
		SetPumpVoltageValue                    = 0b00110000, // 2 lower bits interpreted as pump voltage value
		SetDisplayStartLine                    = 0b01000000, // 6 lower bits interpreted as line address
		SetContrastControlMode                 = 0b10000001, // the following byte will be treated as contrast value
		SetContrast                            = 0b01000000, // 6 lower bits are interpreted as contrast value
		SetSegmentRemap                        = 0b10100000, // lower bit sets right (0) or left (1) rotation
		SetEntireDisplayOn                     = 0b10100100, // lower bit selects normal display (0) or entire display ON (1)
		SetReverseMode                         = 0b10100110, // lower bit sets normal indication (0) or reverse (1)
		SetMultiplexRatio                      = 0b10101000, // the following byte will be treated as multiplex ratio [0, 63]
		SetDCDCControlMode                     = 0b10101101, // the following byte will be treated as DC-DC ON/OFF mode command
		SetDCDCMode                            = 0b10001010, // lower bit sets DC-DC converter off (0) or on (1)
		SetDisplayOn                           = 0b10101110, // lower bit sets OLED panel on (1) or off (0)
		SetPageAddress                         = 0b10110000, // 3 lower bits interpreted as page address [0, 7]
		SetCommonOutputScanDirection           = 0b11000000, // 4 lower bits sets scan mode to LR (8) or to RL (0)
		SetDisplayOffsetControlMode            = 0b11010011, // the following byte will be treated as the mapping of display start line [0, 63]
		SetDivideRatioOscFreqControlMode       = 0b11010101, // the following byte will be treated as oscillator frequency and divide ratio (4 higher and lower bits accordingly)
		SetDischargePrechargeControlMode       = 0b11011001, // the following byte will be treated as discharge and precharge period (4 hugher and lower bits accodringly)
		SetCommonPadsHardwareConfigControlMode = 0b11011010, // the following byte will be treated as sequential/alternative mode set
		SetCommonPadsHardwareConfig            = 0b00000010, // sets sequential (0x02 | 0x10) or alternative (0x02 | 0x00) mode
		SetVCOMDeselectLevelControlMode        = 0b11011011, // the following byte will be treated as common pad output voltage level at deselect stage
		SetReadModifyWriteStart                = 0b11100000, // Read-Modify-Write start
		SetReadModifyWriteEnd                  = 0b11101110, // Read-Modify-Write end
		NOP                                    = 0b11100011  // No operaton
	};
	
	static uint16_t s_max_width;
	static uint16_t s_max_height;
	static size_t   s_buffer_size;
	
	gpio_num_t          m_pin_dc        = GPIO_NUM_NC;
	gpio_num_t          m_pin_reset     = GPIO_NUM_NC;
	spi_device_handle_t m_device_handle = nullptr;
	uint16_t            m_width         = 0;
	uint16_t            m_height        = 0;
	
	uint8_t* m_pixel_data = nullptr;
	
	bool    m_inverted = false;
	uint8_t m_contrast = 0x3F;
	
	void sendCommand(uint8_t cmd);
	void setColumnAddress(uint8_t column);
	
};

//========================================