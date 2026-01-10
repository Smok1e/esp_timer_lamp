#include <peripheral_manager.hpp>
#include <utils.hpp>

#include <esp_log.h>

#include <driver/gpio.h>

//======================================== Constants

constexpr const char* TAG = "peripheral";

constexpr auto GPIO_PIN_RELAY = static_cast<gpio_num_t>(CONFIG_GPIO_PIN_RELAY);

//======================================== Lifecycle

void PeripheralManager::init(Main* main)
{
	m_main = main;
	
	m_task_semaphore_handle = xSemaphoreCreateBinaryStatic(&m_task_semaphore_buffer);
	
	// Lightning
	m_lightning_on_time  = ParseSecondsSinceMidnight(CONFIG_RELAY_START_TIME);
	m_lightning_off_time = ParseSecondsSinceMidnight(CONFIG_RELAY_STOP_TIME );
	
	ESP_ERROR_CHECK(gpio_reset_pin(GPIO_PIN_RELAY));
	ESP_ERROR_CHECK(gpio_set_direction(GPIO_PIN_RELAY, GPIO_MODE_OUTPUT));
	ESP_ERROR_CHECK(gpio_set_level(GPIO_PIN_RELAY, false));
	
	ESP_LOGI(TAG, "relay initialized");
}

void PeripheralManager::start()
{
	xTaskCreate(
		[](void* arg) -> void
		{
			reinterpret_cast<PeripheralManager*>(arg)->task();
		},
		"PeripheralManager",
		4096,
		this,
		5,
		nullptr
	);
}

void PeripheralManager::stop()
{
	m_task_running = false;
	xSemaphoreTake(m_task_semaphore_handle, portMAX_DELAY);
}

//======================================== Getters

bool PeripheralManager::isLightningActive() const
{
	return m_lightning_active;
}

//======================================== Task loop

void PeripheralManager::task()
{
	ESP_LOGI(TAG, "task started");
	
	while (m_task_running)
	{
		auto seconds = SecondsSinceMidnight();
		auto lightning_active = m_lightning_on_time <= seconds && seconds < m_lightning_off_time;
		if (lightning_active != m_lightning_active)
		{
			ESP_LOGI(TAG, "activating lightning");
			ESP_ERROR_CHECK(gpio_set_level(GPIO_PIN_RELAY, m_lightning_active));
			
			m_lightning_active = lightning_active;
		}
		
		vTaskDelay(pdMS_TO_TICKS(10));
	}
	
	xSemaphoreGive(m_task_semaphore_handle);
	ESP_LOGI(TAG, "task ended");
	
	vTaskDelete(nullptr);
}

//========================================