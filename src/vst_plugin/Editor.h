#pragma once

#include <memory>
#include <tchar.h>
#include <atomic>
#include <thread>
#include <d3d11.h>

#include "vst2.x/aeffeditor.h"
#include "imgui/imgui.h"
#include "imgui/misc/cpp/imgui_stdlib.h"
#include "imgui/backends/imgui_impl_win32.h"
#include "imgui/backends/imgui_impl_dx11.h"

class Editor : public AEffEditor {
public:
	Editor(AudioEffect* effect = 0);

	~Editor() override;

	bool getRect(ERect** rect) override;

	bool open(void* ptr) override;

	void close() override;

	bool isOpen() override;

	void idle() override;

private:

	void render(HWND hostWindow);

	HWND m_window{ nullptr };
	ERect m_rect;

	std::atomic_bool m_running{ false };
	std::thread m_render_thread;
};
