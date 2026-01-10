#pragma once

#include <freertos/FreeRTOS.h>

#include <aht20.hpp>

#include <esp_adc/adc_oneshot.h>
#include <esp_adc/adc_cali.h>

//========================================

class Main;

class SensorManager
{
public:
	SensorManager() = default;
	
	void init(Main* main);
	void start();
	void stop();
	
	bool  isAirSensorAvailable() const;
	float getAirTemperature() const;
	float getAirHumidity() const;
	float getSoilMoisture() const;
	
private:
	void task();
	
	Main* m_main = nullptr;
	
	AHT20                  m_sensor {};
	bool                   m_sensor_available = false;
	AHT20::MeasurementData m_air_measurement {};
	
	adc_oneshot_unit_handle_t m_adc_unit_handle = 0;
	adc_cali_handle_t         m_adc_cali_handle = 0;
	float                     m_soil_moisture_measurement = 0.f;
	
	bool              m_task_running = true;
	StaticSemaphore_t m_task_semaphore_buffer = {};
	SemaphoreHandle_t m_task_semaphore_handle = 0;
	
};

//========================================