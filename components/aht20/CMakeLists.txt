idf_component_register(
	SRCS
		"src/aht20.cpp"
		
    INCLUDE_DIRS
		"include"
		
	REQUIRES
		esp_driver_i2c
)