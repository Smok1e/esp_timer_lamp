#include <peripheral_manager.hpp>
#include <main.hpp>
#include <utils.hpp>

#include <esp_log.h>

#include <driver/gpio.h>

//======================================== Constants

constexpr const char* TAG = "peripheral";

constexpr auto RELAY_PIN      = static_cast<gpio_num_t>(CONFIG_PERIPHERALS_RELAY_PIN     );
constexpr auto HUMIDIFIER_PIN = static_cast<gpio_num_t>(CONFIG_PERIPHERALS_HUMIDIFIER_PIN);

//======================================== Lifecycle

void PeripheralManager::init(Main* main)
{
	m_main = main;
	m_main->m_ui_manager.setInitializationStage("Peripherals");
	
	m_task_semaphore_handle = xSemaphoreCreateBinaryStatic(&m_task_semaphore_buffer);
	
	// Lightning
	m_day_start_time = ParseSecondsSinceMidnight(CONFIG_PERIPHERALS_DAY_START_TIME);
	m_day_end_time   = ParseSecondsSinceMidnight(CONFIG_PERIPHERALS_DAY_STOP_TIME );
	
	ESP_ERROR_CHECK(gpio_reset_pin(RELAY_PIN));
	ESP_ERROR_CHECK(gpio_set_direction(RELAY_PIN, GPIO_MODE_OUTPUT));
	ESP_ERROR_CHECK(gpio_set_level(RELAY_PIN, false));
	
	// Humidifier
	ESP_ERROR_CHECK(gpio_reset_pin(HUMIDIFIER_PIN));
	ESP_ERROR_CHECK(gpio_set_direction(HUMIDIFIER_PIN, GPIO_MODE_OUTPUT));
	ESP_ERROR_CHECK(gpio_set_level(HUMIDIFIER_PIN, false));
	
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

//======================================== Setters/getters

void PeripheralManager::setLightningActive(bool active)
{
	if (active == m_lightning_active)
		return;
	
	ESP_LOGI(
		TAG,
		"%sactivating lightning",
		active
			? ""
			: "de"
	);
	
	ESP_ERROR_CHECK(gpio_set_level(RELAY_PIN, m_lightning_active = active));
}

void PeripheralManager::setHumidifierActive(bool active)
{
	if (active == m_humidifier_active)
		return;
	
	ESP_LOGI(
		TAG,
		"%sactivating humidifier",
		active
			? ""
			: "de"
	);
	
	
	ESP_ERROR_CHECK(gpio_set_level(HUMIDIFIER_PIN, active));
	m_humidifier_active = active;
}

bool PeripheralManager::isDaytime() const
{
	return m_day_time;
}

bool PeripheralManager::isLightningActive() const
{
	return m_lightning_active;
}

bool PeripheralManager::isHumidifierActive() const
{
	return m_humidifier_active;
}

//======================================== Task loop

void PeripheralManager::task()
{
	ESP_LOGI(TAG, "task started");
	
	while (m_task_running)
	{
		auto seconds = SecondsSinceMidnight();
		m_day_time = m_day_start_time <= seconds && seconds < m_day_end_time;
		
		// Lightning
		if (m_lightning_active != m_day_time)
			setLightningActive(m_day_time);
		
		// Humidifier
		if (m_main->m_sensor_manager.isAirSensorAvailable())
		{
			auto humidity_min = m_day_time
				? CONFIG_PERIPHERALS_HUMIDIFIER_HUMIDITY_MIN_DAY
				: CONFIG_PERIPHERALS_HUMIDIFIER_HUMIDITY_MIN_NIGHT;
			
			auto humidity_max = m_day_time
				? CONFIG_PERIPHERALS_HUMIDIFIER_HUMIDITY_MAX_DAY
				: CONFIG_PERIPHERALS_HUMIDIFIER_HUMIDITY_MAX_NIGHT;
			
			auto humidity = m_main->m_sensor_manager.getAirHumidity();
			if      (humidity < humidity_min) setHumidifierActive(true );
			else if (humidity > humidity_max) setHumidifierActive(false);
		}
		
		vTaskDelay(pdMS_TO_TICKS(10));
	}
	
	xSemaphoreGive(m_task_semaphore_handle);
	ESP_LOGI(TAG, "task ended");
	
	vTaskDelete(nullptr);
}

//========================================