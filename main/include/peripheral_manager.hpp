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
	
	bool isLightningActive() const;
	
private:
	void task();
	
	Main* m_main = nullptr;
	
	time_t m_lightning_on_time = 0;
	time_t m_lightning_off_time = 0;
	bool   m_lightning_active = false;
	
	bool              m_task_running = true;
	StaticSemaphore_t m_task_semaphore_buffer = {};
	SemaphoreHandle_t m_task_semaphore_handle = 0;
	
};

//========================================