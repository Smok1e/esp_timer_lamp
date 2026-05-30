#pragma once

#include <sh1106.hpp>
#include <render.hpp>

#include <mutex>

//========================================

class Main;

class UIManager
{
public:
	enum class State
	{
		Initialization,
		Running,
		FirmwareUpdate
	};
	
	UIManager() = default;
	
	void init(Main* main);
	void start();
	void stop();
	
	void setState(State state);
	void setInitializationStage(std::string_view stage);
	void setFirmwareUpdateProgress(uint8_t progress);
	
	bool isDisplayOn() const;
	void setDisplayOn(bool on);
	
private:
	void task();
	
	void drawProgress(std::string_view text, uint8_t progress = 0);
	
	void onRenderInitialization();
	void onRenderRunning();
	void onRenderFirmwareUpdate();
	
	Main* m_main = nullptr;
	
	SH1106 m_display {};
	std::mutex m_display_mutex {};
	
	State m_state = State::Initialization;
	std::string_view m_initialization_stage = "Initialization";
	uint8_t m_firmware_update_progress = 0;
	
	bool              m_task_running = true;
	StaticSemaphore_t m_task_semaphore_buffer = {};
	SemaphoreHandle_t m_task_semaphore_handle = 0;
	
};

//========================================