#include <main.hpp>
#include <webserver.hpp>

#include <cJSON.h>

//========================================

constexpr const char* TAG = "webserver";

extern const uint8_t INDEX_HTML_BEGIN[] asm("_binary_index_html_start");
extern const uint8_t INDEX_HTML_END  [] asm("_binary_index_html_end"  );

//========================================

Webserver::~Webserver()
{
	httpd_stop(m_httpd_handle);
	ESP_LOGI(TAG, "webserver stopped");
}

//========================================

void Webserver::init(Main* main)
{
	m_main = main;
	
	httpd_config_t config = HTTPD_DEFAULT_CONFIG();
	config.server_port = CONFIG_WEBSERVER_PORT;
	config.lru_purge_enable = true;
	config.stack_size = 8192;
	
	ESP_ERROR_CHECK(httpd_start(&m_httpd_handle, &config));
	ESP_LOGI(TAG, "webserver started on port %d", CONFIG_WEBSERVER_PORT);
	
	registerEndpoint<&Webserver::indexHandler, HTTP_GET>("/");
	
	registerEndpoint<&Webserver::apiUpdateFirmwareHandler,  HTTP_POST>("/api/update_firmware");
	registerEndpoint<&Webserver::apiGetMeasurementsHandler, HTTP_GET> ("/api/get_measurements");
}

//========================================

void Webserver::indexHandler(httpd_req_t* request)
{
	httpd_resp_send(
		request,
		reinterpret_cast<const char*>(INDEX_HTML_BEGIN),
		INDEX_HTML_END - INDEX_HTML_BEGIN
	);
}

void Webserver::apiUpdateFirmwareHandler(httpd_req_t* request)
{
	char buffer[1024] = "";
	int len = 0;
	
	ESP_LOGI(TAG, "firmware update request received");
	
	if (!(len = httpd_req_get_hdr_value_len(request, "Content-Length")))
	{
		ESP_LOGW(TAG, "Content-Length header not set");
		
		httpd_resp_set_status(request, "400");
		httpd_resp_sendstr(request, "missing Content-Length header");
		return;
	}
	
	if (len > sizeof(buffer) - 1)
	{
		ESP_LOGE(TAG, "Content-Length header is too long");
		
		httpd_resp_send_500(request);
		return;
	}
	
	ESP_ERROR_CHECK(httpd_req_get_hdr_value_str(request, "Content-Length", buffer, sizeof(buffer) - 1));
	buffer[len] = '\0';
	
	char* end = nullptr;
	size_t firmware_size = strtoull(buffer, &end, 10);
	if (end != buffer + len)
	{
		ESP_LOGW(TAG, "invalid Content-Length value");
		
		httpd_resp_set_status(request, "400");
		httpd_resp_sendstr(request, "invalid Content-Length value");
		return;
	}
	
	m_main->m_ui_manager.setState(UIManager::State::FirmwareUpdate);
	ESP_LOGI(TAG, "firmware size: %zu bytes", firmware_size);
	
	const esp_partition_t* running_partition = esp_ota_get_running_partition();
	ESP_LOGI(TAG, "running partition %s", running_partition->label);
	
	const esp_partition_t* update_partition = esp_ota_get_next_update_partition(running_partition);
	ESP_LOGI(TAG, "firmware update will be written to partition %s", update_partition->label);
	
	auto update_start_ticks = xTaskGetTickCount();
	
	esp_ota_handle_t ota_handle = 0;
	esp_err_t err = 0;
	if ((err = esp_ota_begin(update_partition, 0, &ota_handle)) != ESP_OK)
	{
		ESP_LOGE(TAG, "esp_ota_begin failed: %s", esp_err_to_name(err));
		
		httpd_resp_send_500(request);
		return;
	}
	
	ESP_LOGI(TAG, "performing firmware update...");
	
	uint8_t last_progress = 0;
	size_t written = 0;
	
	while (len = httpd_req_recv(request, buffer, sizeof(buffer)))
	{
		if ((err = esp_ota_write(ota_handle, buffer, len)) != ESP_OK)
		{
			m_main->m_ui_manager.setState(UIManager::State::Running);
			
			ESP_LOGE(TAG, "esp_ota_write failed: %s", esp_err_to_name(err));
			esp_ota_abort(ota_handle);
			
			httpd_resp_send_500(request);
			return;
		}
		
		written += len;
		uint8_t progress = 100 * written / firmware_size;
		
		m_main->m_ui_manager.setFirmwareUpdateProgress(progress);
		if (progress != last_progress)
		{
			printf("update progress: %d%%...        \r", static_cast<int>(progress));
			last_progress = progress;
		}
		
		vTaskDelay(pdMS_TO_TICKS(10));
	}
	
	printf("\n");
	
	if ((err = esp_ota_end(ota_handle)) != ESP_OK)
	{
		ESP_LOGE(TAG, "esp_ota_end failed: %s", esp_err_to_name(err));
		
		httpd_resp_send_500(request);
		return;
	}
	
	if ((err = esp_ota_set_boot_partition(update_partition)) != ESP_OK)
	{
		ESP_LOGE(TAG, "esp_ota_set_boot_partition failed: %s", esp_err_to_name(err));
		
		httpd_resp_send_500(request);
		return;
	}
	
	len = snprintf(
		buffer,
		sizeof(buffer),
		"firmware update done successfully in %.2f s",
		static_cast<float>(pdTICKS_TO_MS(xTaskGetTickCount() - update_start_ticks)) / 1000
	);
	
	httpd_resp_send(request, buffer, len);
	ESP_LOGI(TAG, "%.*s", len, buffer);
	
	m_main->stop();
}

void Webserver::apiGetMeasurementsHandler(httpd_req_t* request)
{
	auto* object = cJSON_CreateObject();
	
	cJSON_AddItemToObject(
		object,
		"temperature",
		cJSON_CreateNumber(m_main->m_sensor_manager.getAirTemperature())
	);
	
	cJSON_AddItemToObject(
		object,
		"humidity",
		cJSON_CreateNumber(m_main->m_sensor_manager.getAirHumidity())
	);
	
	cJSON_AddItemToObject(
		object,
		"soil_moisture",
		cJSON_CreateNumber(m_main->m_sensor_manager.getSoilMoisture())
	);
	
	cJSON_AddItemToObject(
		object,
		"lightning_enabled",
		cJSON_CreateBool(m_main->m_peripheral_manager.isLightningActive())
	);
	
	// Update when humidifier added
	cJSON_AddItemToObject(
		object,
		"humidifier_enabled",
		cJSON_CreateFalse()
	);
	
	cJSON_AddItemToObject(
		object,
		"timestamp",
		cJSON_CreateNumber(time(nullptr))
	);
	
	char buffer[1024] = "";
	if (!cJSON_PrintPreallocated(object, buffer, sizeof(buffer), false))
	{
		ESP_LOGE(TAG, "buffer size is not enough to serialize response");
		httpd_resp_send_500(request);
		
		cJSON_Delete(object);
		return;
	}
	
	httpd_resp_set_type(request, "application/json");
	httpd_resp_sendstr(request, buffer);
	cJSON_Delete(object);
}

//========================================