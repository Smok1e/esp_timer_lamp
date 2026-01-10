#include <main.hpp>

//======================================== Constants

static const char* TAG = "main";

constexpr size_t      MAX_IP4_LEN          = 16;
constexpr auto        GPIO_PIN_RELAY       = static_cast<gpio_num_t>(CONFIG_GPIO_PIN_RELAY   );
constexpr auto        GPIO_PIN_LED         = static_cast<gpio_num_t>(CONFIG_GPIO_PIN_LED     );
constexpr auto        I2C_PIN_SDA          = static_cast<gpio_num_t>(CONFIG_SENSOR_PIN_SDA   );
constexpr auto        I2C_PIN_SCL          = static_cast<gpio_num_t>(CONFIG_SENSOR_PIN_SCL   );
constexpr auto        DISPLAY_PIN_DC       = static_cast<gpio_num_t>(CONFIG_DISPLAY_PIN_DC   );
constexpr auto        DISPLAY_PIN_RESET    = static_cast<gpio_num_t>(CONFIG_DISPLAY_PIN_RESET);

constexpr auto        ADC_UNIT             = static_cast<adc_unit_t   >(CONFIG_SOIL_MOISTURE_SENSOR_ADC_UNIT - 1);
constexpr auto        ADC_CHANNEL          = static_cast<adc_channel_t>(CONFIG_SOIL_MOISTURE_SENSOR_ADC_CHANNEL );

constexpr auto        WIFI_CONNECTED_BIT   = BIT0;

constexpr TickType_t  BLINK_PERIOD         = pdMS_TO_TICKS(100);
constexpr TickType_t  SNTP_TIMEOUT         = pdMS_TO_TICKS(1000 * CONFIG_SNTP_TIMEOUT);
constexpr TickType_t  MEASUREMENT_INTERVAL = pdMS_TO_TICKS(1000 * CONFIG_SENSOR_MEASUREMENT_INTERVAL);

#ifdef CONFIG_WIFI_SECURITY_WPA2
constexpr auto        WIFI_SECURITY        = WIFI_AUTH_WPA2_PSK;
#else
constexpr auto        WIFI_SECURITY        = WIFI_AUTH_WPA3_PSK;
#endif

//======================================== Utils

unsigned SecondsSinceMidnight(const tm& info)
{
	return info.tm_sec + 60 * (info.tm_min + 60 * info.tm_hour);
}

unsigned ParseSecondsSinceMidnight(const char* str)
{
	tm info = {};
	if (strptime(str, "%H:%M:%S", &info) == str)
	{
		ESP_LOGE("parser", "%s does not match the HH:MM:SS format; aborting", str);
		abort();
	}
	
	return SecondsSinceMidnight(info);
}

//======================================== Display

void Main::initDisplay()
{
	spi_bus_config_t spi_bus_config = {};
	spi_bus_config.miso_io_num = -1;
	spi_bus_config.mosi_io_num = CONFIG_DISPLAY_PIN_MOSI;
	spi_bus_config.sclk_io_num = CONFIG_DISPLAY_PIN_SCK;
	spi_bus_config.quadwp_io_num = -1;
	spi_bus_config.quadhd_io_num = -1;
	spi_bus_config.max_transfer_sz = 0xFFFF;

	auto result = spi_bus_initialize(SPI2_HOST, &spi_bus_config, SPI_DMA_CH_AUTO);
	if (result != ESP_OK && result != ESP_ERR_INVALID_STATE)
		ESP_ERROR_CHECK(result);
	
	ESP_LOGI(TAG, "SPI bus initialized");
	
	m_display.init(
		SPI2_HOST,
		DISPLAY_PIN_DC,
		DISPLAY_PIN_RESET,
		1'000'000 * CONFIG_DISPLAY_SPI_FREQ_MHZ
	);
	
	ESP_LOGI(TAG, "display initialized");
}

//======================================== Wifi

void Main::initNVS()
{
	renderStatus("NVS");
	
	auto result = nvs_flash_init();
	if (result == ESP_ERR_NVS_NO_FREE_PAGES || result == ESP_ERR_NVS_NEW_VERSION_FOUND)
	{
		ESP_ERROR_CHECK(nvs_flash_erase());
		result = nvs_flash_init();
	}
	ESP_ERROR_CHECK(result);
	
	ESP_LOGI(TAG, "nvs initialized");
}

void Main::initWifi()
{
	renderStatus("Wifi connection");
	
	m_wifi_event_group = xEventGroupCreate();
	
	ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_loop_create_default());

	auto* netif = esp_netif_create_default_wifi_sta();
	ESP_ERROR_CHECK(esp_netif_set_hostname(netif, CONFIG_WIFI_HOSTNAME));

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));

	esp_event_handler_instance_t instance_any_id, instance_got_ip;
	ESP_ERROR_CHECK(esp_event_handler_instance_register(
		WIFI_EVENT,
		ESP_EVENT_ANY_ID,
		&Main::WifiEventHandler,
		this,
		&instance_any_id
	));

	ESP_ERROR_CHECK(esp_event_handler_instance_register(
		IP_EVENT,
		IP_EVENT_STA_GOT_IP,
		&Main::WifiEventHandler,
		this,
		&instance_got_ip
	));

	wifi_config_t wifi_config = {};
	wifi_config.sta.threshold.authmode = WIFI_SECURITY;

	strncpy(reinterpret_cast<char*>(wifi_config.sta.ssid    ), CONFIG_WIFI_SSID,     sizeof(wifi_config.sta.ssid    ));
	strncpy(reinterpret_cast<char*>(wifi_config.sta.password), CONFIG_WIFI_PASSWORD, sizeof(wifi_config.sta.password));

	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
	ESP_ERROR_CHECK(esp_wifi_start());

	xEventGroupWaitBits(
		m_wifi_event_group,
		WIFI_CONNECTED_BIT,
		false,
		false,
		portMAX_DELAY
	);
	
	ESP_LOGI(TAG, "wifi station initialized");
}

void Main::WifiEventHandler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
	Main* instance = reinterpret_cast<Main*>(arg);

	if      (event_base == WIFI_EVENT) instance->onWifiEvent(event_id, event_data);
	else if (event_base == IP_EVENT  ) instance->onIpEvent  (event_id, event_data);
}

void Main::onWifiEvent(int32_t event_id, void *event_data)
{
	switch (event_id)
	{
		case WIFI_EVENT_STA_START:
			esp_wifi_connect();
			ESP_LOGI(TAG, "connecting to %s...", CONFIG_WIFI_SSID);
			break;

		case WIFI_EVENT_STA_DISCONNECTED:
		{
			m_wifi_connected = false;
			
			esp_wifi_connect();
			ESP_LOGI(
				TAG,
				"wifi disconnected (%d); reconnecting to %s...",
				static_cast<int>(
					reinterpret_cast<wifi_event_sta_disconnected_t*>(event_data)->reason
				),
				CONFIG_WIFI_SSID
			);

			break;
		}
	}
}

void Main::onIpEvent(int32_t event_id, void* event_data)
{
	switch (event_id)
	{
		case IP_EVENT_STA_GOT_IP:
		{
			m_wifi_connected = true;
			
			auto* event = reinterpret_cast<ip_event_got_ip_t*>(event_data);
			ESP_LOGI(TAG, "successfully connected to the AP; got ip: " IPSTR, IP2STR(&event->ip_info.ip));
			
			xEventGroupSetBits(m_wifi_event_group, WIFI_CONNECTED_BIT);
			
			break;
		}
	}
}

//======================================== GPIO

void Main::initGPIO()
{
	renderStatus("GPIO");
	
	// Relay config
	ESP_ERROR_CHECK(gpio_reset_pin(GPIO_PIN_RELAY));
	ESP_ERROR_CHECK(gpio_set_direction(GPIO_PIN_RELAY, GPIO_MODE_OUTPUT));
	ESP_ERROR_CHECK(gpio_set_level(GPIO_PIN_RELAY, false));
	
	// Indicator led config
	if (GPIO_PIN_LED != GPIO_NUM_NC)
	{
		ESP_ERROR_CHECK(gpio_reset_pin(GPIO_PIN_LED));
		ESP_ERROR_CHECK(gpio_set_direction(GPIO_PIN_LED, GPIO_MODE_OUTPUT));
		ESP_ERROR_CHECK(gpio_set_level(GPIO_PIN_LED, false));
	}
	
	ESP_LOGI(TAG, "GPIO configured");
}

//======================================== SNTP

void Main::initSNTP()
{
	renderStatus("Synchronizing");
	
	m_relay_start_time = ParseSecondsSinceMidnight(CONFIG_RELAY_START_TIME);
	m_relay_stop_time  = ParseSecondsSinceMidnight(CONFIG_RELAY_STOP_TIME );
	
	setenv("TZ", CONFIG_SNTP_TIMEZONE, true);
	tzset();
	ESP_LOGI(TAG, "set timezone %s", CONFIG_SNTP_TIMEZONE);
	
	esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG(CONFIG_SNTP_SERVER);
	ESP_ERROR_CHECK(esp_netif_sntp_init(&config));
	ESP_LOGI(TAG, "SNTP initialized, syhcnronizing time...");
	
	if (esp_err_t ret; (ret = esp_netif_sntp_sync_wait(SNTP_TIMEOUT)) != ESP_OK)
	{
		if (ret == ESP_ERR_TIMEOUT)
			ESP_LOGE(TAG, "SNTP timeout exceeded");
		
		ESP_ERROR_CHECK(ret);
	}
	
	time_t now = 0;
	time(&now);
	localtime_r(&now, &m_local_time);
	
	char buffer[64] = "";
	strftime(buffer, std::size(buffer), "%Y-%m-%d %H:%M:%S", &m_local_time);
	
	ESP_LOGI(TAG, "time synchronized: %s", buffer);
}

//======================================== Web interface

void Main::initWebserver()
{
	renderStatus("Webserver");
	m_webserver.init(this);
	ESP_LOGI(TAG, "webserver initialized");
}

//======================================== Air temperature/humidity sensor

void Main::initEnvironmentSensor()
{
	renderStatus("Sensor");
	
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
	
	m_sensor.init(i2c_bus_handle, 1000 * CONFIG_SENSOR_I2C_FREQ_KHZ, CONFIG_SENSOR_I2C_DEVICE_ADDRESS);
	ESP_LOGI(TAG, "AHT20 sensor initialized");
}

//======================================== Soil humidity sensor

void Main::initSoilMoistureSensor()
{
	renderStatus("ADC");
	
	adc_oneshot_unit_init_cfg_t init_config = {};
	init_config.unit_id = ADC_UNIT;
	
	ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &m_adc_handle));
	
	adc_oneshot_chan_cfg_t channel_config = {};
	channel_config.atten = ADC_ATTEN_DB_12;
	channel_config.bitwidth = ADC_BITWIDTH_DEFAULT;
	ESP_ERROR_CHECK(adc_oneshot_config_channel(m_adc_handle, ADC_CHANNEL, &channel_config));
	
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
	
	ESP_LOGI(TAG, "soil moisture sensor initialized");
}

//========================================

Vector3f RotateAroundX(const Vector3f& point, float sin_phi, float cos_phi)
{
	return Vector3f(
		point.x,
		point.y * cos_phi - point.z * sin_phi,
		point.y * sin_phi + point.z * cos_phi
	);
}

Vector3f RotateAroundY(const Vector3f& point, float sin_phi, float cos_phi)
{
	return Vector3f(
		point.x * cos_phi + point.z * sin_phi,
		point.y,
		-point.x * sin_phi + point.z * cos_phi
	);
}

Vector3f RotateAroundZ(const Vector3f& point, float sin_phi, float cos_phi)
{
	return Vector3f(
		point.x * cos_phi - point.y * sin_phi,
		point.x * sin_phi + point.y * cos_phi,
		point.z
	);
}

//========================================

void Main::renderStatus(std::string_view status)
{
	m_display.clear();
	
	Vector2u display_size = m_display.getSize();
	for (unsigned x = 0; x < display_size.x; x++)
		for (unsigned y = 0; y < display_size.y; y++)
			if ((y + x) % 10 == 0) m_display.setPixel(x, y, true);
	
	render::Text(
		m_display,
		m_font,
		Vector2i(m_display.getSize()) / 2 - m_font.getTextSize(status) / 2,
		status,
		true,
		true
	);
	
	m_display.flush();
}

void Main::render()
{
	std::lock_guard<std::mutex> lock(m_render_mutex);
	
	m_display.clear();
	
	Vector2u display_size = m_display.getSize();
	
	if (m_update_progress < 0)
	{
		auto text = render::FormatTmp("%.2f", m_measurement.temperature);
		Vector2u text_size(
			m_icon_temperature.getSize().x + m_font.getTextSize(text).x + m_icon_degree.getSize().x,
			m_font.getGlyphSize().y * 3
		);
		
		Vector2u text_pos = Vector2u(0, display_size.y / 2 - text_size.y / 2);
		
		uint8_t y = 0;
		
		// Temperature
		render::Icon(
			m_display,
			m_icon_temperature,
			text_pos
		);
		
		render::Text(
			m_display,
			m_font,
			text_pos + Vector2i(m_icon_temperature.getSize().x + 4, y),
			text
		);
		
		render::Icon(
			m_display,
			m_icon_degree,
			text_pos + Vector2i(m_icon_temperature.getSize().x + 4 + m_font.getTextSize(text).x , y)
		);
		
		y += m_font.getGlyphSize().y + 1;
		
		// Humidity
		render::Icon(
			m_display,
			m_icon_humidity,
			text_pos + Vector2i(0, y)
		);
		
		render::Text(
			m_display,
			m_font,
			text_pos + Vector2i(m_icon_humidity.getSize().x + 4, y),
			render::FormatTmp("%.2f%%", m_measurement.humidity)
		);
		
		y += m_font.getGlyphSize().y + 1;
		
		// Soil moisture
		render::Icon(
			m_display,
			m_icon_soil,
			text_pos + Vector2i(0, y)
		);
		
		render::Text(
			m_display,
			m_font,
			text_pos + Vector2i(m_icon_soil.getSize().x + 4, y),
			render::FormatTmp("%.2fV", m_soil_moisture_sensor_voltage)
		);
		
		Vector2u cube_offset(text_size.x + (display_size.x - text_size.x) / 2, display_size.y / 2);
		constexpr float cube_max_size = 30;
		
		auto time = static_cast<float>(pdTICKS_TO_MS(xTaskGetTickCount() - m_cube_start_ticks)) / 1000.f;
		auto t = ((sin(time) + 1.f) / 4.f) + .5f;
		auto cube_size = cube_max_size * t;
		
		Vector2f angles(
			.5f * std::numbers::pi * time,
			.3f * std::numbers::pi * time
		);
		
		float sin_phi_x = sin(angles.x);
		float cos_phi_x = cos(angles.x);
		
		float sin_phi_y = sin(angles.y);
		float cos_phi_y = cos(angles.y);
		
		auto transform = [
			&cube_offset,
			&sin_phi_x,
			&cos_phi_x,
			&sin_phi_y,
			&cos_phi_y
		](const Vector3f& point) -> Vector2f
		{
			auto rotated = RotateAroundY(
				RotateAroundX(
					point,
					sin_phi_x,
					cos_phi_x
				),
				sin_phi_y,
				cos_phi_y
			);
			
			return cube_offset + Vector2f(rotated.x, rotated.y);
		};
		
		//     E-----------F
		//    /|          /|
		//   / |         / |
		//  A-----------B  |
		//  |  |        |  |
		//  |  G--------|--H
		//  | /         | /
		//  |/          |/
		//  C-----------D
	
		auto A = transform(Vector3f(-cube_size / 2, -cube_size / 2, -cube_size / 2));
		auto B = transform(Vector3f( cube_size / 2, -cube_size / 2, -cube_size / 2));
		auto C = transform(Vector3f(-cube_size / 2,  cube_size / 2, -cube_size / 2));
		auto D = transform(Vector3f( cube_size / 2,  cube_size / 2, -cube_size / 2));
		auto E = transform(Vector3f(-cube_size / 2, -cube_size / 2,  cube_size / 2));
		auto F = transform(Vector3f( cube_size / 2, -cube_size / 2,  cube_size / 2));
		auto G = transform(Vector3f(-cube_size / 2,  cube_size / 2,  cube_size / 2));
		auto H = transform(Vector3f( cube_size / 2,  cube_size / 2,  cube_size / 2));
	
		render::Line(m_display, A, B);
		render::Line(m_display, B, D);
		render::Line(m_display, D, C);
		render::Line(m_display, C, A);
		
		render::Line(m_display, E, F);
		render::Line(m_display, F, H);
		render::Line(m_display, H, G);
		render::Line(m_display, G, E);
		
		render::Line(m_display, A, E);
		render::Line(m_display, B, F);
		render::Line(m_display, C, G);
		render::Line(m_display, D, H);
	}
	
	else
	{
		Vector2u progress_bar_size(
			display_size.x - 16,
			16
		);
		
		std::string_view text = "firmware update";
		render::Text(
			m_display,
			m_font,
			Vector2i(display_size.x / 2 - m_font.getTextSize(text).x / 2, 0),
			text
		);
		
		auto progress_bar_position = display_size / 2 - progress_bar_size / 2;
		render::Rectangle(
			m_display,
			progress_bar_position,
			progress_bar_size,
			true,
			false
		);
		
		Vector2u progress_bar_current_size(
			progress_bar_size.x * m_update_progress / 100,
			progress_bar_size.y
		);
		
		render::Rectangle(
			m_display,
			progress_bar_position,
			progress_bar_current_size
		);
	}
	
	m_display.flush();
}

//========================================

void Main::measurementTaskProc()
{
	ESP_LOGI(TAG, "started measurement task procedure");
	
	while (m_running)
	{
		auto start_ticks = xTaskGetTickCount();
		m_measurement = m_sensor.measureMultisample(CONFIG_SENSOR_SAMPLE_COUNT);
		
		int voltage_mv_sum = 0;
		for (size_t i = 0; i < CONFIG_SOIL_MOISTURE_SENSOR_SAMPLE_COUNT; i++)
		{
			int voltage_mv = 0;
			ESP_ERROR_CHECK(
				adc_oneshot_get_calibrated_result(
					m_adc_handle,
					m_adc_cali_handle,
					ADC_CHANNEL,
					&voltage_mv
				)
			);
			
			voltage_mv_sum += voltage_mv;
		}
		
		m_soil_moisture_sensor_voltage = static_cast<float>(
			voltage_mv_sum
		) / (1000.f * CONFIG_SOIL_MOISTURE_SENSOR_SAMPLE_COUNT);
		
		auto elapsed = xTaskGetTickCount() - start_ticks;
		if (elapsed >= MEASUREMENT_INTERVAL)
		{
			ESP_LOGW(TAG, "single measurement took more than measurement interval");
			vTaskDelay(pdMS_TO_TICKS(10));
		}
		
		else
			vTaskDelay(MEASUREMENT_INTERVAL - elapsed);
	}
	
	ESP_LOGI(TAG, "measurement task has ended");
	xSemaphoreGive(m_measurement_semaphore);
	
	vTaskDelete(nullptr);
}

//========================================

void Main::run()
{
	m_measurement_semaphore = xSemaphoreCreateBinary();
	
	initDisplay();
	initGPIO();
	initNVS();
	initWifi();
	initSNTP();
	initWebserver();
	initEnvironmentSensor();
	initSoilMoistureSensor();

	xTaskCreate(
		[](void* arg) -> void
		{
			reinterpret_cast<Main*>(arg)->measurementTaskProc();
		},
		"Measurement task",
		4096,
		this,
		5,
		nullptr
	);
	
	ESP_LOGI(TAG, "main task started");
	
	m_cube_start_ticks = xTaskGetTickCount();
	while (m_running)
	{
		auto current_ticks = xTaskGetTickCount();
		
		// Blinking in case of wifi failure
		if (GPIO_PIN_LED != GPIO_NUM_NC)
		{
			if (!m_wifi_connected)
			{
				if (current_ticks - m_last_blink_ticks > BLINK_PERIOD)
				{
					ESP_ERROR_CHECK(gpio_set_level(GPIO_PIN_LED, m_last_blink_level ^= true));
					m_last_blink_ticks = current_ticks;
				}
			}
			
			else
				ESP_ERROR_CHECK(gpio_set_level(GPIO_PIN_LED, false));
		}
	
		// Updating relay
		time_t now = 0;
		time(&now);
		localtime_r(&now, &m_local_time);
		
		auto seconds = SecondsSinceMidnight(m_local_time);
		bool relay_on = m_relay_start_time <= seconds && seconds < m_relay_stop_time;
		
		if (relay_on != m_relay_on)
		{
			char buffer[32] = "";
			strftime(buffer, std::size(buffer), "%H:%M:%S", &m_local_time);
			
			ESP_LOGI(TAG, "current time is %s; %sactivating relay", buffer, relay_on? "": "de");
			ESP_ERROR_CHECK(gpio_set_level(GPIO_PIN_RELAY, relay_on));
			
			m_relay_on = relay_on;
		}
		
		render();
		
		vTaskDelay(pdMS_TO_TICKS(10));
	}
	
	ESP_LOGI(TAG, "waiting for measurement task to end");
	xSemaphoreTake(m_measurement_semaphore, portMAX_DELAY);
	
	ESP_LOGI(TAG, "main task has ended");
	ESP_LOGI(TAG, "rebooting");
	esp_restart();
}

void Main::stop()
{
	m_running = false;
}

//========================================

extern "C" void app_main()
{
	Main instance;
	instance.run();
}

//========================================