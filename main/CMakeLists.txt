idf_component_register(
	SRCS
		"src/main.cpp"
		"src/sensor_manager.cpp"
		"src/peripheral_manager.cpp"
		"src/ui_manager.cpp"
		"src/utils.cpp"
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
		
		"resources/icons/plant/plant_0.bin"
		"resources/icons/plant/plant_1.bin"
		"resources/icons/plant/plant_2.bin"
		"resources/icons/plant/plant_3.bin"
		"resources/icons/plant/plant_4.bin"
		"resources/icons/plant/plant_5.bin"
		"resources/icons/plant/plant_6.bin"
		"resources/icons/plant/plant_7.bin"
		"resources/icons/plant/plant_8.bin"
		"resources/icons/plant/plant_9.bin"
		"resources/icons/plant/plant_10.bin"
		"resources/icons/plant/plant_11.bin"
		"resources/icons/plant/plant_12.bin"
		"resources/icons/plant/plant_13.bin"
		"resources/icons/plant/plant_14.bin"
		"resources/icons/plant/plant_15.bin"
		
)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-parentheses)