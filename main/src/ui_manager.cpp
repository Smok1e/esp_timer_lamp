#include <ui_manager.hpp>
#include <utils.hpp>
#include <main.hpp>

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
		1'000'000 * CONFIG_DISPLAY_SPI_FREQ_MHZ
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

//======================================== Task loop

void UIManager::task()
{
	ESP_LOGI(TAG, "task started");
	
	while (m_task_running)
	{
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
		
		vTaskDelay(pdMS_TO_TICKS(10));
	}
	
	xSemaphoreGive(m_task_semaphore_handle);
	ESP_LOGI(TAG, "task ended");
	
	vTaskDelete(nullptr);
}

//======================================== Initialization stage rendering

void UIManager::onRenderInitialization()
{
	Vector2u display_size = m_display.getSize();
	for (unsigned x = 0; x < display_size.x; x++)
		for (unsigned y = 0; y < display_size.y; y++)
			if ((y + x) % 10 == 0) m_display.setPixel(x, y, true);
	
	render::Text(
		m_display,
		FONT,
		Vector2i(m_display.getSize()) / 2 - FONT.getTextSize(m_initialization_stage) / 2,
		m_initialization_stage,
		true,
		true
	);
}

//======================================== Runtime rendering

void UIManager::onRenderRunning()
{
	Vector2u display_size = m_display.getSize();
	
	auto text = render::FormatTmp("%5.2f", m_main->m_sensor_manager.getAirTemperature());
	Vector2u text_size(
		ICON_TEMPERATURE.getSize().x + FONT.getTextSize(text).x + ICON_DEGREE.getSize().x,
		FONT.getGlyphSize().y * 3
	);
	
	Vector2u text_pos = Vector2u(0, display_size.y / 2 - text_size.y / 2);
	
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
		text_pos + Vector2i(ICON_TEMPERATURE.getSize().x + 4, y),
		text
	);
	
	render::Icon(
		m_display,
		ICON_DEGREE,
		text_pos + Vector2i(ICON_TEMPERATURE.getSize().x + 4 + FONT.getTextSize(text).x , y)
	);
	
	y += FONT.getGlyphSize().y + 1;
	
	// Humidity
	render::Icon(
		m_display,
		ICON_HUMIDITY,
		text_pos + Vector2i(0, y)
	);
	
	render::Text(
		m_display,
		FONT,
		text_pos + Vector2i(ICON_HUMIDITY.getSize().x + 4, y),
		render::FormatTmp("%5.2f%%", m_main->m_sensor_manager.getAirHumidity())
	);
	
	y += FONT.getGlyphSize().y + 1;
	
	// Soil moisture
	render::Icon(
		m_display,
		ICON_SOIL,
		text_pos + Vector2i(0, y)
	);
	
	render::Text(
		m_display,
		FONT,
		text_pos + Vector2i(ICON_SOIL.getSize().x + 4, y),
		render::FormatTmp("%5.2fV", m_main->m_sensor_manager.getSoilMoisture())
	);
	
	Vector2u cube_offset(text_size.x + (display_size.x - text_size.x) / 2, display_size.y / 2);
	constexpr float cube_max_size = 30;
	
	auto time = static_cast<float>(pdTICKS_TO_MS(xTaskGetTickCount())) / 1000.f;
	auto t = ((sin(time) + 1.f) / 4.f) + .5f;
	auto cube_size = cube_max_size * t;
	
	Vector2f angles(
		.5f * std::numbers::pi * time,
		.3f * std::numbers::pi * time
	);
	
	float sin_phi_x = sin(angles.x);
	float cos_phi_x = cos(angles.x);
	
	float sin_phi_y = sin(angles.y);
	float cos_phi_y = cos(angles.y);
	
	auto transform = [
		&cube_offset,
		&sin_phi_x,
		&cos_phi_x,
		&sin_phi_y,
		&cos_phi_y
	](const Vector3f& point) -> Vector2f
	{
		auto rotated = RotateAroundY(
			RotateAroundX(
				point,
				sin_phi_x,
				cos_phi_x
			),
			sin_phi_y,
			cos_phi_y
		);
		
		return cube_offset + Vector2f(rotated.x, rotated.y);
	};
	
	//     E-----------F
	//    /|          /|
	//   / |         / |
	//  A-----------B  |
	//  |  |        |  |
	//  |  G--------|--H
	//  | /         | /
	//  |/          |/
	//  C-----------D

	auto A = transform(Vector3f(-cube_size / 2, -cube_size / 2, -cube_size / 2));
	auto B = transform(Vector3f( cube_size / 2, -cube_size / 2, -cube_size / 2));
	auto C = transform(Vector3f(-cube_size / 2,  cube_size / 2, -cube_size / 2));
	auto D = transform(Vector3f( cube_size / 2,  cube_size / 2, -cube_size / 2));
	auto E = transform(Vector3f(-cube_size / 2, -cube_size / 2,  cube_size / 2));
	auto F = transform(Vector3f( cube_size / 2, -cube_size / 2,  cube_size / 2));
	auto G = transform(Vector3f(-cube_size / 2,  cube_size / 2,  cube_size / 2));
	auto H = transform(Vector3f( cube_size / 2,  cube_size / 2,  cube_size / 2));

	render::Line(m_display, A, B);
	render::Line(m_display, B, D);
	render::Line(m_display, D, C);
	render::Line(m_display, C, A);
	
	render::Line(m_display, E, F);
	render::Line(m_display, F, H);
	render::Line(m_display, H, G);
	render::Line(m_display, G, E);
	
	render::Line(m_display, A, E);
	render::Line(m_display, B, F);
	render::Line(m_display, C, G);
	render::Line(m_display, D, H);
}

//======================================== Firmware update rendering

void UIManager::onRenderFirmwareUpdate()
{
	Vector2u display_size = m_display.getSize();
	Vector2u progress_bar_size(
		display_size.x - 16,
		16
	);
	
	std::string_view text = "firmware update";
	render::Text(
		m_display,
		FONT,
		Vector2i(display_size.x / 2 - FONT.getTextSize(text).x / 2, 0),
		text
	);
	
	auto progress_bar_position = display_size / 2 - progress_bar_size / 2;
	render::Rectangle(
		m_display,
		progress_bar_position,
		progress_bar_size,
		true,
		false
	);
	
	Vector2u progress_bar_current_size(
		progress_bar_size.x * m_firmware_update_progress / 100,
		progress_bar_size.y
	);
	
	render::Rectangle(
		m_display,
		progress_bar_position,
		progress_bar_current_size
	);
}

//========================================