idf_component_register(
	SRCS
		"src/sh1106.cpp"
		
    INCLUDE_DIRS
		"include"
		
	REQUIRES
		esp_driver_spi
		esp_driver_gpio
		
)