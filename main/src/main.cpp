#include <main.hpp>

//======================================== Constants

static const char* TAG = "main";

constexpr size_t      MAX_IP4_LEN          = 16;
constexpr auto        GPIO_PIN_LED         = static_cast<gpio_num_t>(CONFIG_GPIO_PIN_LED     );

constexpr auto        WIFI_CONNECTED_BIT   = BIT0;

constexpr TickType_t  BLINK_PERIOD         = pdMS_TO_TICKS(100);
constexpr TickType_t  SNTP_TIMEOUT         = pdMS_TO_TICKS(1000 * CONFIG_SNTP_TIMEOUT);

#ifdef CONFIG_WIFI_SECURITY_WPA2
constexpr auto        WIFI_SECURITY        = WIFI_AUTH_WPA2_PSK;
#else
constexpr auto        WIFI_SECURITY        = WIFI_AUTH_WPA3_PSK;
#endif

//======================================== Wifi

void Main::initNVS()
{
	m_ui_manager.setInitializationStage("NVS");
	
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
	m_ui_manager.setInitializationStage("Wifi");
	
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

//======================================== SNTP

void Main::initSNTP()
{
	m_ui_manager.setInitializationStage("Time sync");
	
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
}

void Main::run()
{
	m_ui_manager.init(this);
	m_ui_manager.start();
	m_ui_manager.setState(UIManager::State::Initialization);
	
	m_peripheral_manager.init(this);
	
	initNVS();
	initWifi();
	initSNTP();
	
	m_webserver.init(this);
	m_sensor_manager.init(this);
	
	m_sensor_manager.start();
	m_peripheral_manager.start();
	
	m_ui_manager.setState(UIManager::State::Running);
	
	ESP_LOGI(TAG, "task started");
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
		
		vTaskDelay(pdMS_TO_TICKS(10));
	}
	
	ESP_LOGI(TAG, "task ended");
	ESP_LOGI(TAG, "rebooting");
	esp_restart();
}

void Main::stop()
{
	m_running = false;
	
	m_sensor_manager.stop();
	m_peripheral_manager.stop();
	m_ui_manager.stop();
}

//========================================

extern "C" void app_main()
{
	Main instance;
	instance.run();
}

//========================================