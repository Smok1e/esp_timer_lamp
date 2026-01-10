#include <aht20.hpp>

#include <esp_log.h>
#include <esp_err.h>
#include <esp_check.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <utility>

//=======================================================

constexpr auto TIMEOUT = pdMS_TO_TICKS(100);
static const char* TAG = "sht20";

//=======================================================

esp_err_t AHT20::init(
	i2c_master_bus_handle_t i2c_bus_handle,
	int i2c_freq,
	uint8_t device_address
)
{
	i2c_device_config_t config = {};
	config.dev_addr_length = I2C_ADDR_BIT_LEN_7;
	config.device_address = device_address;
	config.scl_speed_hz = i2c_freq;
	
	ESP_RETURN_ON_ERROR(
		i2c_master_bus_add_device(
			i2c_bus_handle,
			&config,
			&m_device_handle
		),
		TAG,
		"unable to add device"
	);
	
	// Waiting 40ms as required by the specification
	vTaskDelay(pdMS_TO_TICKS(40));
	
	// Reading status
	uint8_t command = std::to_underlying(Command::Status);
	uint8_t status = 0;
	ESP_RETURN_ON_ERROR(
		i2c_master_transmit_receive(
			m_device_handle,
			&command,
			sizeof(command),
			&status,
			sizeof(status),
			TIMEOUT
		),
		TAG,
		"status reading failure"
	);
	
	if (!(status & BIT3))
	{
		ESP_LOGI(TAG, "initialization required");
		
		// Sending initialization sequence and waiting 10ms as required by the specification
		uint8_t data[3] = {
			std::to_underlying(Command::Initialize),
			0x08,
			0x00
		};
		
		ESP_ERROR_CHECK(
			i2c_master_transmit(
				m_device_handle,
				data,
				sizeof(data),
				TIMEOUT
			)
		);
		
		vTaskDelay(pdMS_TO_TICKS(10));
	}
	
	return ESP_OK;
}

AHT20::~AHT20()
{
	ESP_ERROR_CHECK(i2c_master_bus_rm_device(m_device_handle));
}

//=======================================================

AHT20::MeasurementData AHT20::measure()
{
	uint8_t data[] = {
		std::to_underlying(Command::TriggerMeasurement),
		0x33,
		0x00
	};
	
	ESP_ERROR_CHECK(
		i2c_master_transmit(
			m_device_handle,
			data,
			sizeof(data),
			TIMEOUT
		)
	);
	
	// Waiting 80ms as required by the specification
	vTaskDelay(pdMS_TO_TICKS(80));
	
	uint8_t status = 0;
	ESP_ERROR_CHECK(
		i2c_master_receive(
			m_device_handle,
			&status,
			sizeof(status),
			TIMEOUT
		)
	);
	
	if (status & BIT7)
	{
		ESP_LOGE(TAG, "measurement not ready");
		abort();
	}
	
	uint8_t buffer[7] = {};
	ESP_ERROR_CHECK(
		i2c_master_receive(
			m_device_handle,
			buffer,
			sizeof(buffer),
			TIMEOUT
		)
	);
	
	uint32_t raw_temperature =  (buffer[1]         << 12) | (buffer[2] << 4) | (buffer[3] >> 4);
	uint32_t raw_humidity    = ((buffer[3] & 0x0F) << 16) | (buffer[4] << 8) |  buffer[5];

	MeasurementData result = {};
	result.humidity    = (static_cast<float>(raw_temperature) * 100) / 1048576;
	result.temperature = (static_cast<float>(raw_humidity   ) * 200) / 1048576 - 50;
	
	return result;
}

AHT20::MeasurementData AHT20::measureMultisample(size_t sample_count /*= 10*/)
{
	MeasurementData average = {};
	for (size_t i = 0; i < sample_count; i++)
	{
		auto sample = measure();
		average.temperature += sample.temperature;
		average.humidity += sample.humidity;
	}
	
	average.temperature /= sample_count;
	average.humidity    /= sample_count;
	return average;
}

//=======================================================