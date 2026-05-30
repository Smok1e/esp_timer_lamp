#include <array>
#include <mutex>
#include <memory>
#include <cstring>
#include <ctime>

#include <FreeRTOS/FreeRTOS.h>
#include <freertos/task.h>

#include <esp_system.h>
#include <esp_wifi.h>
#include <esp_log.h>
#include <esp_netif.h>
#include <esp_event.h>
#include <esp_sntp.h>
#include <esp_netif_sntp.h>

#include <driver/gpio.h>

#include <nvs_flash.h>

#include <lwip/err.h>
#include <lwip/sockets.h>

#include <sh1106.hpp>
#include <render.hpp>
#include <font.hpp>
#include <icon.hpp>

#include <vector3.hpp>

#include <sensor_manager.hpp>
#include <peripheral_manager.hpp>
#include <ui_manager.hpp>
#include <webserver.hpp>

//========================================

class Main
{
public:
	friend class SensorManager;
	friend class PeripheralManager;
	friend class UIManager;
	friend class Webserver;
	
	Main() = default;

	void run();
	void stop();
	
private:
	static void WifiEventHandler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
	void onWifiEvent(int32_t event_id, void* event_data);
	void onIpEvent(int32_t event_id, void* event_data);

	void initNVS();
	void initWifi();
	void initSNTP();
	
	SensorManager     m_sensor_manager     = {};
	PeripheralManager m_peripheral_manager = {};
	UIManager         m_ui_manager         = {};
	Webserver         m_webserver          = {};
	
	bool       m_wifi_connected   = false;
	TickType_t m_last_blink_ticks = xTaskGetTickCount();
	bool       m_last_blink_level = false;
	bool       m_running          = true;
	
	EventGroupHandle_t m_wifi_event_group = 0;

};

//========================================