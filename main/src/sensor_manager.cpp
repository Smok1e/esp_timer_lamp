#include <sensor_manager.hpp>
#include <main.hpp>

#include <esp_log.h>

#include <cmath>

//======================================== Constants

constexpr const char* TAG = "sensor";

constexpr auto I2C_PIN_SDA = static_cast<gpio_num_t>(CONFIG_SENSOR_PIN_SDA);
constexpr auto I2C_PIN_SCL = static_cast<gpio_num_t>(CONFIG_SENSOR_PIN_SCL);

constexpr auto ADC_UNIT    = static_cast<adc_unit_t   >(CONFIG_SOIL_MOISTURE_SENSOR_ADC_UNIT - 1);
constexpr auto ADC_CHANNEL = static_cast<adc_channel_t>(CONFIG_SOIL_MOISTURE_SENSOR_ADC_CHANNEL );

constexpr auto MEASUREMENT_INTERVAL = pdMS_TO_TICKS(1000 * CONFIG_SENSOR_MEASUREMENT_INTERVAL);

//======================================== Lifecycle

void SensorManager::init(Main* main)
{
	m_main = main;
	m_main->m_ui_manager.setInitializationStage("Sensors");
	
	m_task_semaphore_handle = xSemaphoreCreateBinaryStatic(&m_task_semaphore_buffer);
	
	// SHT20 I2C air temperature/humidity sensor
	i2c_master_bus_config_t i2c_bus_config = {};
	i2c_bus_config.i2c_port = I2C_NUM_0;
	i2c_bus_config.sda_io_num = I2C_PIN_SDA;
	i2c_bus_config.scl_io_num = I2C_PIN_SCL;
	i2c_bus_config.clk_source = I2C_CLK_SRC_DEFAULT;
	i2c_bus_config.glitch_ignore_cnt = 7;
	i2c_bus_config.flags.enable_internal_pullup = true;
	
	i2c_master_bus_handle_t i2c_bus_handle = nullptr;
	ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_config, &i2c_bus_handle));
	ESP_LOGI(TAG, "I2C bus initialized");
	
	if (
		esp_err_t err;
		(
			err = m_sensor.init(
				i2c_bus_handle,
				1000 * CONFIG_SENSOR_I2C_FREQ_KHZ,
				CONFIG_SENSOR_I2C_DEVICE_ADDRESS
			)
		)
		== ESP_OK
	)
	{
		ESP_LOGI(TAG, "AHT20 sensor initialized");
		m_sensor_available = true;
	}
	
	else
		ESP_LOGW(TAG, "AHT20 sensor initialization failed: %s", esp_err_to_name(err));
	
	// Capacitive soil moisture sensor
	adc_oneshot_unit_init_cfg_t init_config = {};
	init_config.unit_id = ADC_UNIT;
	
	ESP_ERROR_CHECK(
		adc_oneshot_new_unit(
			&init_config,
			&m_adc_unit_handle
		)
	);
	
	adc_oneshot_chan_cfg_t channel_config = {};
	channel_config.atten = ADC_ATTEN_DB_12;
	channel_config.bitwidth = ADC_BITWIDTH_DEFAULT;
	
	ESP_ERROR_CHECK(
		adc_oneshot_config_channel(
			m_adc_unit_handle,
			ADC_CHANNEL,
			&channel_config
		)
	);
	
	adc_cali_curve_fitting_config_t cali_config = {};
	cali_config.unit_id = ADC_UNIT;
	cali_config.chan = ADC_CHANNEL;
	cali_config.atten = ADC_ATTEN_DB_12;
	cali_config.bitwidth = ADC_BITWIDTH_DEFAULT;
	
	ESP_ERROR_CHECK(
		adc_cali_create_scheme_curve_fitting(
			&cali_config,
			&m_adc_cali_handle
		)
	);
	
	ESP_LOGI(TAG, "ADC initialized");
}

void SensorManager::start()
{
	xTaskCreate(
		[](void* arg) -> void
		{
			reinterpret_cast<SensorManager*>(arg)->task();
		},
		"SensorManager",
		4096,
		this,
		5,
		nullptr
	);
}

void SensorManager::stop()
{
	m_task_running = false;
	xSemaphoreTake(m_task_semaphore_handle, portMAX_DELAY);
}

//======================================== Getters

bool SensorManager::isAirSensorAvailable() const
{
	return m_sensor_available;
}

float SensorManager::getAirTemperature() const
{
	return m_air_measurement.temperature;
}

float SensorManager::getAirHumidity() const
{
	return m_air_measurement.humidity;
}

float SensorManager::getSoilMoisture() const
{
	return m_soil_moisture_measurement;
}

//======================================== Task loop

void SensorManager::task()
{
	ESP_LOGI(TAG, "task started");
	
	while (m_task_running)
	{
		auto measurement_start_ticks = xTaskGetTickCount();
		
		// Air properties
		if (m_sensor_available)
			m_air_measurement = m_sensor.measureMultisample();
		
		// Soil moisture
		int voltage_mv_sum = 0;
		for (size_t i = 0; i < CONFIG_SOIL_MOISTURE_SENSOR_SAMPLE_COUNT; i++)
		{
			int voltage_mv = 0;
			ESP_ERROR_CHECK(
				adc_oneshot_get_calibrated_result(
					m_adc_unit_handle,
					m_adc_cali_handle,
					ADC_CHANNEL,
					&voltage_mv
				)
			);
			
			voltage_mv_sum += voltage_mv;
		}
		
		m_soil_moisture_measurement = static_cast<float>(
			voltage_mv_sum
		) / (1000.f * CONFIG_SOIL_MOISTURE_SENSOR_SAMPLE_COUNT);
		
		// Waiting until next measurement
		auto elapsed = xTaskGetTickCount() - measurement_start_ticks;
		if (elapsed >= MEASUREMENT_INTERVAL)
		{
			ESP_LOGW(TAG, "single measurement took %d ms", static_cast<int>(pdTICKS_TO_MS(elapsed)));
			vTaskDelay(pdMS_TO_TICKS(10));
		}
		
		else
			vTaskDelay(MEASUREMENT_INTERVAL - elapsed);
	}
	
	xSemaphoreGive(m_task_semaphore_handle);
	ESP_LOGI(TAG, "task ended");
	
	vTaskDelete(nullptr);
}

//========================================