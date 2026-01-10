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

#include <esp_adc/adc_oneshot.h>
#include <esp_adc/adc_cali.h>
#include <esp_adc/adc_cali_scheme.h>

#include <driver/gpio.h>

#include <nvs_flash.h>

#include <lwip/err.h>
#include <lwip/sockets.h>

#include <aht20.hpp>
#include <sh1106.hpp>
#include <render.hpp>
#include <font.hpp>
#include <icon.hpp>
#include <webserver.hpp>

#include <vector3.hpp>

//========================================

extern const uint8_t FONT_BEGIN            [] asm("_binary_font_bin_start"       );
extern const uint8_t FONT_END              [] asm("_binary_font_bin_end"         );

extern const uint8_t ICON_TEMPERATURE_BEGIN[] asm("_binary_temperature_bin_start");
extern const uint8_t ICON_TEMPERATURE_END  [] asm("_binary_temperature_bin_end"  );

extern const uint8_t ICON_HUMIDITY_BEGIN   [] asm("_binary_humidity_bin_start"   );
extern const uint8_t ICON_HUMIDITY_END     [] asm("_binary_humidity_bin_end"     );

extern const uint8_t ICON_SOIL_BEGIN       [] asm("_binary_soil_bin_start"       );
extern const uint8_t ICON_SOIL_END         [] asm("_binary_soil_bin_end"         );

extern const uint8_t ICON_DEGREE_BEGIN     [] asm("_binary_degree_bin_start"     );
extern const uint8_t ICON_DEGREE_END       [] asm("_binary_degree_bin_end"       );

//========================================

unsigned SecondsSinceMidnight(const tm& info);
unsigned ParseSecondsSinceMidnight(const char* str);

//========================================

class Main
{
public:
	friend class Webserver;
	
	Main() = default;

	void run();
	void stop();
	
private:
	static void WifiEventHandler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
	void onWifiEvent(int32_t event_id, void* event_data);
	void onIpEvent(int32_t event_id, void* event_data);

	static void ColorModeHandlerTaskProc(void* arg);
	void colorModeHandler();

	void initDisplay();
	void initNVS();
	void initWifi();
	void initGPIO();
	void initSNTP();
	void initWebserver();
	void initEnvironmentSensor();
	void initSoilMoistureSensor();
	
	void renderStatus(std::string_view status);
	void render();
	
	void measurementTaskProc();
	
	unsigned   m_relay_start_time = {};
	unsigned   m_relay_stop_time  = {};
	bool       m_relay_on         = false;
	bool       m_wifi_connected   = false;
	TickType_t m_last_blink_ticks = xTaskGetTickCount();
	bool       m_last_blink_level = false;
	tm         m_local_time       = {};
	TickType_t m_cube_start_ticks = {};
	int        m_update_progress  = -1;
	bool       m_running          = true;
	
	std::mutex m_render_mutex {};
	
	EventGroupHandle_t m_wifi_event_group = 0;
	SemaphoreHandle_t  m_measurement_semaphore = 0;
	
	httpd_handle_t m_httpd_handle = 0;
	
	AHT20                  m_sensor      = {};
	AHT20::MeasurementData m_measurement = {};
	
	adc_oneshot_unit_handle_t m_adc_handle = 0;
	adc_cali_handle_t         m_adc_cali_handle = 0;
	float                     m_soil_moisture_sensor_voltage = 0;
	
	SH1106 m_display = {};
	Webserver m_webserver = {};
	
	Font m_font             { FONT_BEGIN,             FONT_END             };
	Icon m_icon_temperature { ICON_TEMPERATURE_BEGIN, ICON_TEMPERATURE_END };
	Icon m_icon_humidity    { ICON_HUMIDITY_BEGIN,    ICON_HUMIDITY_END    };
	Icon m_icon_soil        { ICON_SOIL_BEGIN,        ICON_SOIL_END        };
	Icon m_icon_degree      { ICON_DEGREE_BEGIN,      ICON_DEGREE_END      };
};

//========================================