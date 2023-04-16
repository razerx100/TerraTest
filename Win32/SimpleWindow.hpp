#ifndef SIMPLE_WINDOW_HPP_
#define SIMPLE_WINDOW_HPP_
#include <cstdint>
#include <CleanWin.hpp>

class WindowClass {
public:
	WindowClass() noexcept;
	~WindowClass() noexcept;

	WindowClass(const WindowClass&) = delete;
	WindowClass& operator=(const WindowClass&) = delete;

	const char* GetName() noexcept;
	void Register() noexcept;

	[[nodiscard]]
	HINSTANCE GetHInstance() const noexcept;

private:
	static constexpr const char* wndClassName = "Luna";
	WNDCLASSEX m_wndClass;
};

class SimpleWindow {
public:
	SimpleWindow(std::uint32_t width, std::uint32_t height, const char* name);
	~SimpleWindow() noexcept;

	SimpleWindow(const SimpleWindow&) = delete;
	SimpleWindow& operator=(const SimpleWindow&) = delete;

	[[nodiscard]]
	void* GetWindowHandle() const noexcept;
	[[nodiscard]]
	void* GetModuleInstance() const noexcept;

	static LRESULT CALLBACK HandleMsgSetup(
		HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam
	) noexcept;

private:
	HWND m_hWnd;
	WindowClass m_windowClass;
};
#endif
