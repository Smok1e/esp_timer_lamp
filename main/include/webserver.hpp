#pragma once

#include <esp_http_server.h>
#include <esp_partition.h>
#include <esp_ota_ops.h>

//========================================

class Main;

class Webserver
{
public:
	Webserver() = default;
	~Webserver();
	
	void init(Main* main);
	
private:
	Main* m_main = nullptr;

	httpd_handle_t m_httpd_handle = 0;
	
	template<auto Handler, httpd_method_t Method = HTTP_GET>
	void registerEndpoint(const char* uri);
	
	void indexHandler              (httpd_req_t* request);
	void apiUpdateFirmwareHandler  (httpd_req_t* request);
	void apiGetMeasurementsHandler (httpd_req_t* request);
	
};

//========================================

template<auto Handler, httpd_method_t Method /*= HTTP_GET*/>
void Webserver::registerEndpoint(const char* uri)
{
	httpd_uri_t config = {};
	config.uri = uri;
	config.method = Method;
	config.user_ctx = this;
	config.handler = [](httpd_req_t* request) -> esp_err_t {
		(reinterpret_cast<Webserver*>(request->user_ctx)->*Handler)(request);
		
		return ESP_OK;
	};
	
	ESP_ERROR_CHECK(httpd_register_uri_handler(m_httpd_handle, &config));
}

//========================================