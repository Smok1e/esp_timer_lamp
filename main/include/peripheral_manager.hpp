#pragma once

#include <freertos/FreeRTOS.h>

#include <ctime>

//========================================

class Main;

class PeripheralManager
{
public:
	PeripheralManager() = default;
	
	void init(Main* main);
	void start();
	void stop();
	
	void setLightningActive(bool active);
	void setHumidifierActive(bool active);
	
	bool isDaytime() const;
	bool isLightningActive() const;
	bool isHumidifierActive() const;
	
private:
	void task();
	
	Main* m_main = nullptr;
	
	time_t m_day_start_time = 0;
	time_t m_day_end_time = 0;
	bool   m_day_time = false;
	bool   m_lightning_active = false;
	bool   m_humidifier_active = false;
	
	bool              m_task_running = true;
	StaticSemaphore_t m_task_semaphore_buffer = {};
	SemaphoreHandle_t m_task_semaphore_handle = 0;
	
};

//========================================