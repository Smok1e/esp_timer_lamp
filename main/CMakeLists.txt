idf_component_register(
	SRCS
		"src/main.cpp"
		"src/render.cpp"
		"src/font.cpp"
		"src/icon.cpp"
		"src/webserver.cpp"
		
	INCLUDE_DIRS
		"include"
		
	REQUIRES
		esp_wifi
		esp_driver_gpio
		esp_driver_i2c
		esp_http_server
		esp_adc
		esp-tls
		app_update
		nvs_flash
		cjson
		aht20
		sh1106
		
	EMBED_FILES
		"resources/icons/font.bin"
		"resources/icons/temperature.bin"
		"resources/icons/humidity.bin"
		"resources/icons/soil.bin"
		"resources/icons/degree.bin"
		
		"resources/web/index.html"
		
)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-parentheses)