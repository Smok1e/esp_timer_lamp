#pragma once

#include <driver/i2c_master.h>


//=======================================================

class AHT20
{
public:
	struct MeasurementData
	{
		float humidity;
		float temperature;
	};
	
	AHT20() = default;
	AHT20(const AHT20& copy) = delete;
	~AHT20();
	
	esp_err_t init(
		i2c_master_bus_handle_t i2c_bus_handle,
		int i2c_freq,
		uint8_t device_address = 0x38
	);
	
	MeasurementData measure();
	MeasurementData measureMultisample(size_t sample_count = 10);
	
private:
	enum Command: uint8_t
	{
		Status             = 0b01110001,
		Initialize         = 0b10111110,
		TriggerMeasurement = 0b10101100,
		SoftReset          = 0b10111010
	};
	
	i2c_master_dev_handle_t m_device_handle = nullptr;

};

//=======================================================