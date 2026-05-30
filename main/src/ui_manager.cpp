#include <ui_manager.hpp>
#include <utils.hpp>
#include <main.hpp>

#include <algorithm>

//======================================== Constants

constexpr const char* TAG = "ui";

constexpr auto DISPLAY_PIN_DC    = static_cast<gpio_num_t>(CONFIG_DISPLAY_PIN_DC   );
constexpr auto DISPLAY_PIN_RESET = static_cast<gpio_num_t>(CONFIG_DISPLAY_PIN_RESET);

//======================================== Resources

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

const Font FONT             { FONT_BEGIN,             FONT_END             };
const Icon ICON_TEMPERATURE { ICON_TEMPERATURE_BEGIN, ICON_TEMPERATURE_END };
const Icon ICON_HUMIDITY    { ICON_HUMIDITY_BEGIN,    ICON_HUMIDITY_END    };
const Icon ICON_SOIL        { ICON_SOIL_BEGIN,        ICON_SOIL_END        };
const Icon ICON_DEGREE      { ICON_DEGREE_BEGIN,      ICON_DEGREE_END      };

//========================================

extern const uint8_t ICON_PLANT_0_BEGIN [] asm("_binary_plant_0_bin_start"    );
extern const uint8_t ICON_PLANT_0_END   [] asm("_binary_plant_0_bin_end"      );

extern const uint8_t ICON_PLANT_1_BEGIN [] asm("_binary_plant_1_bin_start"    );
extern const uint8_t ICON_PLANT_1_END   [] asm("_binary_plant_1_bin_end"      );

extern const uint8_t ICON_PLANT_2_BEGIN [] asm("_binary_plant_2_bin_start"    );
extern const uint8_t ICON_PLANT_2_END   [] asm("_binary_plant_2_bin_end"      );

extern const uint8_t ICON_PLANT_3_BEGIN [] asm("_binary_plant_3_bin_start"    );
extern const uint8_t ICON_PLANT_3_END   [] asm("_binary_plant_3_bin_end"      );

extern const uint8_t ICON_PLANT_4_BEGIN [] asm("_binary_plant_4_bin_start"    );
extern const uint8_t ICON_PLANT_4_END   [] asm("_binary_plant_4_bin_end"      );

extern const uint8_t ICON_PLANT_5_BEGIN [] asm("_binary_plant_5_bin_start"    );
extern const uint8_t ICON_PLANT_5_END   [] asm("_binary_plant_5_bin_end"      );

extern const uint8_t ICON_PLANT_6_BEGIN [] asm("_binary_plant_6_bin_start"    );
extern const uint8_t ICON_PLANT_6_END   [] asm("_binary_plant_6_bin_end"      );

extern const uint8_t ICON_PLANT_7_BEGIN [] asm("_binary_plant_7_bin_start"    );
extern const uint8_t ICON_PLANT_7_END   [] asm("_binary_plant_7_bin_end"      );

extern const uint8_t ICON_PLANT_8_BEGIN [] asm("_binary_plant_8_bin_start"    );
extern const uint8_t ICON_PLANT_8_END   [] asm("_binary_plant_8_bin_end"      );

extern const uint8_t ICON_PLANT_9_BEGIN [] asm("_binary_plant_9_bin_start"    );
extern const uint8_t ICON_PLANT_9_END   [] asm("_binary_plant_9_bin_end"      );

extern const uint8_t ICON_PLANT_10_BEGIN[] asm("_binary_plant_10_bin_start"    );
extern const uint8_t ICON_PLANT_10_END  [] asm("_binary_plant_10_bin_end"      );

extern const uint8_t ICON_PLANT_11_BEGIN[] asm("_binary_plant_11_bin_start"    );
extern const uint8_t ICON_PLANT_11_END  [] asm("_binary_plant_11_bin_end"      );

extern const uint8_t ICON_PLANT_12_BEGIN[] asm("_binary_plant_12_bin_start"    );
extern const uint8_t ICON_PLANT_12_END  [] asm("_binary_plant_12_bin_end"      );

extern const uint8_t ICON_PLANT_13_BEGIN[] asm("_binary_plant_13_bin_start"    );
extern const uint8_t ICON_PLANT_13_END  [] asm("_binary_plant_13_bin_end"      );

extern const uint8_t ICON_PLANT_14_BEGIN[] asm("_binary_plant_14_bin_start"    );
extern const uint8_t ICON_PLANT_14_END  [] asm("_binary_plant_14_bin_end"      );

extern const uint8_t ICON_PLANT_15_BEGIN[] asm("_binary_plant_15_bin_start"    );
extern const uint8_t ICON_PLANT_15_END  [] asm("_binary_plant_15_bin_end"      );

const Icon ICON_PLANT[] = {
	{ ICON_PLANT_0_BEGIN,  ICON_PLANT_0_BEGIN  },
	{ ICON_PLANT_1_BEGIN,  ICON_PLANT_1_BEGIN  },
	{ ICON_PLANT_2_BEGIN,  ICON_PLANT_2_BEGIN  },
	{ ICON_PLANT_3_BEGIN,  ICON_PLANT_3_BEGIN  },
	{ ICON_PLANT_4_BEGIN,  ICON_PLANT_4_BEGIN  },
	{ ICON_PLANT_5_BEGIN,  ICON_PLANT_5_BEGIN  },
	{ ICON_PLANT_6_BEGIN,  ICON_PLANT_6_BEGIN  },
	{ ICON_PLANT_7_BEGIN,  ICON_PLANT_7_BEGIN  },
	{ ICON_PLANT_8_BEGIN,  ICON_PLANT_8_BEGIN  },
	{ ICON_PLANT_9_BEGIN,  ICON_PLANT_9_BEGIN  },
	{ ICON_PLANT_10_BEGIN, ICON_PLANT_10_BEGIN },
	{ ICON_PLANT_11_BEGIN, ICON_PLANT_11_BEGIN },
	{ ICON_PLANT_12_BEGIN, ICON_PLANT_12_BEGIN },
	{ ICON_PLANT_13_BEGIN, ICON_PLANT_13_BEGIN },
	{ ICON_PLANT_14_BEGIN, ICON_PLANT_14_BEGIN },
	{ ICON_PLANT_15_BEGIN, ICON_PLANT_15_BEGIN }
};

//======================================== Lifecycle

void UIManager::init(Main* main)
{
	m_main = main;

	m_task_semaphore_handle = xSemaphoreCreateBinaryStatic(&m_task_semaphore_buffer);
	
	spi_bus_config_t spi_bus_config = {};
	spi_bus_config.miso_io_num = -1;
	spi_bus_config.mosi_io_num = CONFIG_DISPLAY_PIN_MOSI;
	spi_bus_config.sclk_io_num = CONFIG_DISPLAY_PIN_SCK;
	spi_bus_config.quadwp_io_num = -1;
	spi_bus_config.quadhd_io_num = -1;
	spi_bus_config.max_transfer_sz = 0xFFFF;

	auto result = spi_bus_initialize(SPI2_HOST, &spi_bus_config, SPI_DMA_CH_AUTO);
	if (result != ESP_OK && result != ESP_ERR_INVALID_STATE)
		ESP_ERROR_CHECK(result);
	
	ESP_LOGI(TAG, "SPI bus initialized");
	
	m_display.init(
		SPI2_HOST,
		DISPLAY_PIN_DC,
		DISPLAY_PIN_RESET,
		1000
	);
	
	ESP_LOGI(TAG, "display initialized");
	ESP_LOGI(TAG, "UI initialized");
}

void UIManager::start()
{
	xTaskCreate(
		[](void* arg) -> void
		{
			reinterpret_cast<UIManager*>(arg)->task();
		},
		"UIManager",
		4096,
		this,
		5,
		nullptr
	);
}

void UIManager::stop()
{
	m_task_running = false;
	xSemaphoreTake(m_task_semaphore_handle, portMAX_DELAY);
}

//======================================== Setters

void UIManager::setState(UIManager::State state)
{
	m_state = state;
}

void UIManager::setInitializationStage(std::string_view stage)
{
	m_initialization_stage = stage;
}

void UIManager::setFirmwareUpdateProgress(uint8_t progress)
{
	m_firmware_update_progress = progress;
}

bool UIManager::isDisplayOn() const
{
	return m_display.isDisplayOn();
}

void UIManager::setDisplayOn(bool on)
{
	if (m_display.isDisplayOn() == on)
		return;
	
	std::lock_guard<std::mutex> lock(m_display_mutex);
	
	ESP_LOGI(TAG, "set display %s", on? "on": "off");
	m_display.setDisplayOn(on);
}

//======================================== Task loop

void UIManager::task()
{
	ESP_LOGI(TAG, "task started");
	
	while (m_task_running)
	{
		if (m_display.isDisplayOn())
		{
			std::lock_guard<std::mutex> lock(m_display_mutex);
			m_display.clear();
			
			switch (m_state)
			{
				case State::Initialization:
					onRenderInitialization();
					break;
					
				case State::Running:
					onRenderRunning();
					break;
					
				case State::FirmwareUpdate:
					onRenderFirmwareUpdate();
					break;
					
			}
			
			m_display.flush();
		}
		
		vTaskDelay(pdMS_TO_TICKS(10));
	}
	
	xSemaphoreGive(m_task_semaphore_handle);
	ESP_LOGI(TAG, "task ended");
	
	vTaskDelete(nullptr);
}

//======================================== Progress bar

void UIManager::drawProgress(std::string_view text, uint8_t progress)
{
	constexpr size_t gap = 4;
	
	Vector2u display_size = m_display.getSize();
	
	Vector2i text_size = FONT.getTextSize(text);
	Vector2u progress_bar_size(
		display_size.x - 16,
		7
	);
	
	size_t progress_bar_fill_width = progress_bar_size.x / 3;
	size_t progress_bar_fill_space = progress_bar_size.x * 2;
	
	render::Text(
		m_display,
		FONT,
		Vector2i(
			(display_size.x - text_size.x) / 2,
			(display_size.y - text_size.y - gap - progress_bar_size.y) / 2
		),
		text,
		true,
		true
	);
	
	Vector2i progress_bar_pos(
		(display_size.x - progress_bar_size.x) / 2,
		(display_size.y + text_size.y) / 2 + gap
	);
	
	render::RoundedRectangle(
		m_display,
		progress_bar_pos,
		progress_bar_size,
		1,
		false,
		render::RoundedRectangleStyle::Default | render::RoundedRectangleStyle::Outline
	);
	
	m_display.setScissor(
		progress_bar_pos.x + 1,
		progress_bar_pos.y + 1,
		progress_bar_size.x - 2,
		progress_bar_size.y - 2
	);
	
	if (progress)
	{
		render::Rectangle(
			m_display,
			progress_bar_pos,
			Vector2i(progress * progress_bar_size.x / 100, progress_bar_size.y)
		);
	}
	
	else
	{
		auto milliseconds = pdTICKS_TO_MS(xTaskGetTickCount());
		
		Vector2i progress_bar_fill_pos(
			progress_bar_pos.x + (
				(progress_bar_size.x * milliseconds / 750) % progress_bar_fill_space
			) - progress_bar_fill_space / 2,
			progress_bar_pos.y
		);
		
		render::Rectangle(
			m_display,
			progress_bar_fill_pos,
			Vector2i(
				progress_bar_fill_width,
				progress_bar_size.y
			)
		);
	}
	
	m_display.setScissor();
}

//======================================== Rendering

void UIManager::onRenderInitialization()
{
	drawProgress(m_initialization_stage);
}

void UIManager::onRenderRunning()
{
	constexpr size_t gap = 3;
	
	Vector2u display_size = m_display.getSize();
	
	auto text = m_main->m_sensor_manager.isAirSensorAvailable()
		? render::FormatTmp("%5.2f", m_main->m_sensor_manager.getAirTemperature())
		: "N/A    ";
	
	Vector2u text_size(
		ICON_TEMPERATURE.getSize().x + (text.length() + 1) * FONT.getGlyphSize().x,
		(FONT.getGlyphSize().y + gap) * 3
	);
	
	Vector2i text_pos(0, display_size.y / 2 - text_size.y / 2);
	
	uint8_t y = 0;
	
	// Temperature
	render::Icon(
		m_display,
		ICON_TEMPERATURE,
		text_pos
	);
	
	render::Text(
		m_display,
		FONT,
		text_pos + Vector2i(ICON_TEMPERATURE.getSize().x + gap, y),
		text
	);
	
	if (m_main->m_sensor_manager.isAirSensorAvailable())
		render::Icon(
			m_display,
			ICON_DEGREE,
			text_pos + Vector2i(ICON_TEMPERATURE.getSize().x + gap + FONT.getTextSize(text).x , y)
		);
	
	y += FONT.getGlyphSize().y + gap;
	
	// Humidity
	render::Icon(
		m_display,
		ICON_HUMIDITY,
		text_pos + Vector2i(0, y)
	);
	
	render::Text(
		m_display,
		FONT,
		text_pos + Vector2i(ICON_HUMIDITY.getSize().x + gap, y),
		m_main->m_sensor_manager.isAirSensorAvailable()
			? render::FormatTmp("%5.2f%%", m_main->m_sensor_manager.getAirHumidity())
			: "N/A"
	);
	
	y += FONT.getGlyphSize().y + gap;
	
	// Soil moisture
	render::Icon(
		m_display,
		ICON_SOIL,
		text_pos + Vector2i(0, y)
	);
	
	render::Text(
		m_display,
		FONT,
		text_pos + Vector2i(ICON_SOIL.getSize().x + gap, y),
		render::FormatTmp("%5.2fV", m_main->m_sensor_manager.getSoilMoisture())
	);
	
	constexpr auto frame_count = std::size(ICON_PLANT);
	auto index = std::min<size_t>(
		frame_count - 1,
		(pdTICKS_TO_MS(xTaskGetTickCount()) / 150) % (frame_count + 10)
	);
	
	const auto& frame = ICON_PLANT[index];
	
	render::Icon(
		m_display,
		frame,
		Vector2i(
			text_size.x + (display_size.x - text_size.x - frame.getSize().x) / 2,
			(display_size.y - frame.getSize().y) / 2
		)
	);
}

void UIManager::onRenderFirmwareUpdate()
{
	drawProgress("Updating", m_firmware_update_progress);
}

//========================================