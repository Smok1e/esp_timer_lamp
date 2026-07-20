#include <main.hpp>
#include <webserver.hpp>

#include <cJSON.h>

//========================================

constexpr const char* TAG = "webserver";

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
	
	registerEndpoint<&Webserver::apiUpdateFirmwareHandler,   HTTP_POST>("/api/update_firmware"   );
	registerEndpoint<&Webserver::apiGetStatusHandler,        HTTP_GET> ("/api/get_status"        );
	registerEndpoint<&Webserver::apiSetDisplayOnHandler,     HTTP_GET> ("/api/set_display_on"    );
	registerEndpoint<&Webserver::apiSetLightningModeHandler, HTTP_GET> ("/api/set_lightning_mode");
}

void Webserver::stop()
{
	httpd_stop(m_httpd_handle);
	ESP_LOGI(TAG, "webserver stopped");
}

//========================================

void Webserver::apiUpdateFirmwareHandler(httpd_req_t* request)
{
	char buffer[1024] = "";
	int len = 0;
	
	httpd_resp_set_type(request, "text/plain");
	
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

void Webserver::apiGetStatusHandler(httpd_req_t* request)
{
	auto* object = cJSON_CreateObject();
	
	if (m_main->m_sensor_manager.isAirSensorAvailable())
	{
		cJSON_AddNumberToObject(
			object,
			"temperature",
			m_main->m_sensor_manager.getAirTemperature()
		);
		
		cJSON_AddNumberToObject(
			object,
			"humidity",
			m_main->m_sensor_manager.getAirHumidity()
		);
	}
	
	else
	{
		cJSON_AddNullToObject(object, "temperature");
		cJSON_AddNullToObject(object, "humidity");
	}
	
	cJSON_AddNumberToObject(
		object,
		"soil_moisture",
		m_main->m_sensor_manager.getSoilMoisture()
	);
	
	cJSON_AddBoolToObject(
		object,
		"lightning_active",
		m_main->m_peripheral_manager.isLightningActive()
	);
	
	cJSON_AddStringToObject(
		object,
		"lightning_mode",
		LightningModeToStr(m_main->m_peripheral_manager.getLightningMode())
	);
	
	cJSON_AddBoolToObject(
		object,
		"humidifier_active",
		m_main->m_peripheral_manager.isHumidifierActive()
	);
	
	cJSON_AddBoolToObject(
		object,
		"display_on",
		m_main->m_ui_manager.isDisplayOn()
	);
	
	cJSON_AddNumberToObject(
		object,
		"timestamp",
		time(nullptr)
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

void Webserver::apiSetDisplayOnHandler(httpd_req_t* request)
{
	httpd_resp_set_type(request, "text/plain");
	
	char query[128] = "";
	if (esp_err_t err; (err = httpd_req_get_url_query_str(request, query, sizeof(query))) != ESP_OK)
	{
		ESP_LOGE(TAG, "unable to get query string: %s", esp_err_to_name(err));
		
		httpd_resp_set_status(request, "400");
		httpd_resp_sendstr(request, "invalid query");
		return;
	}
	
	char value[8] = "";
	if (esp_err_t err; (err = httpd_query_key_value(query, "on", value, sizeof(value))) != ESP_OK)
	{
		ESP_LOGE(TAG, "unable to get query value: %s", esp_err_to_name(err));
		
		httpd_resp_set_status(request, "400");
		httpd_resp_sendstr(request, "invalid query value");
		return;
	}

	bool on = (strcmp(value, "1") == 0);
	m_main->m_ui_manager.setDisplayOn(on);
	
	httpd_resp_sendstr(
		request,
		on
			? "set display on"
			: "set display off"
	);
}

void Webserver::apiSetLightningModeHandler(httpd_req_t* request)
{
	httpd_resp_set_type(request, "text/plain");
	
	char query[128] = "";
	if (esp_err_t err; (err = httpd_req_get_url_query_str(request, query, sizeof(query))) != ESP_OK)
	{
		ESP_LOGE(TAG, "unable to get query string: %s", esp_err_to_name(err));
		
		httpd_resp_set_status(request, "400");
		httpd_resp_sendstr(request, "invalid query");
		return;
	}
	
	char value[8] = "";
	if (esp_err_t err; (err = httpd_query_key_value(query, "mode", value, sizeof(value))) != ESP_OK)
	{
		ESP_LOGE(TAG, "unable to get query value: %s", esp_err_to_name(err));
		
		httpd_resp_set_status(request, "400");
		httpd_resp_sendstr(request, "invalid query value");
		return;
	}

	auto mode = StrToLightningMode(value);
	if (!mode)
	{
		ESP_LOGE(TAG, "invalid lightning mode string: %s", value);
		
		httpd_resp_set_status(request, "400");
		httpd_resp_sendstr(request, "invalid mode string");
	}
	
	m_main->m_peripheral_manager.setLightningMode(mode.value());
	
	httpd_resp_sendstr(
		request,
		"lightning mode updated"
	);
}

//========================================