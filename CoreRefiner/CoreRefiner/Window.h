#pragma once
#include "Win.h"
#include "ExceptionBase.h"
#include "Graphics.h"
#include <optional>
#include <memory>
#include "Timer.h"

namespace Rgph { class RenderGraph; }

class Window
{
public:
	class Exception : public ExceptionBase
	{
		using ExceptionBase::ExceptionBase;
	public:
		static std::string TranslateErrorCode( HRESULT hr ) noexcept;
	};
	class HrException : public Exception
	{
	public:
		HrException( int line,const char* file,HRESULT hr ) noexcept;
		const char* what() const noexcept override;
		const char* GetType() const noexcept override;
		HRESULT GetErrorCode() const noexcept;
		std::string GetErrorDescription() const noexcept;
	private:
		HRESULT hr;
	};
	class NoGfxException : public Exception
	{
	public:
		using Exception::Exception;
		const char* GetType() const noexcept override;
	};
private:
	// singleton manages registration/cleanup of window class
	class WindowClass
	{
	public:
		static const char* GetName() noexcept;
		static HINSTANCE GetInstance() noexcept;
	private:
		WindowClass() noexcept;
		~WindowClass();
		WindowClass( const WindowClass& ) = delete;
		WindowClass& operator=( const WindowClass& ) = delete;
		static constexpr const char* wndClassName = "Direct3D Engine Window";
		static WindowClass wndClass;
		HINSTANCE hInst;
	};
public:
	Window( int width,int height,const char* name );
	~Window();
	Window( const Window& ) = delete;
	Window& operator=( const Window& ) = delete;
	void SetTitle( const std::string& title );
	void SetDebugTitle(int CountFPS);
	// cursor related
	void EnableCursor() noexcept;
	void DisableCursor() noexcept;
	bool CursorEnabled() const noexcept;
	[[nodiscard]] HWND GetHwnd() const noexcept { return hWnd; }
	[[nodiscard]] bool MapGameToClient(int gx, int gy, int& outX, int& outY) const noexcept;
	static HCURSOR GetCursor() noexcept;
	static void SetCursorHandle(HCURSOR cur) noexcept;
	void TickCursorAutoHide() noexcept;
	// screen related
	void ToggleFullscreen() noexcept;
	void SetFullscreen(bool enable) noexcept;
	void SetWindowedClientSize(int clientWidth, int clientHeight) noexcept;
	bool IsFullscreen() const noexcept;
	static std::optional<int> ProcessMessages() noexcept;
	Graphics& Gfx();
private:
	// cursor related
	void ConfineCursor() noexcept;
	void FreeCursor() noexcept;
	void ShowCursor() noexcept;
	void HideCursor() noexcept;
	void EnableImGuiMouse() noexcept;
	void DisableImGuiMouse() noexcept;
	bool MapClientToGame(int cx, int cy, int& outX, int& outY) noexcept;
	void RememberWindowedClientSize_(int clientWidth, int clientHeight) noexcept;
	void ApplyWindowedClientSize_(int clientWidth, int clientHeight) noexcept;
	static LRESULT CALLBACK HandleMsgSetup( HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam ) noexcept;
	static LRESULT CALLBACK HandleMsgThunk( HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam ) noexcept;
	LRESULT HandleMsg( HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam ) noexcept;
private:
	// cursor related
	bool cursorEnabled = true;
	static HCURSOR hCursor;
	Timer cursorIdleTimer;
	bool autoHideCursor = true;
	bool cursorHiddenByIdle = false;
	float cursorHideSeconds = 2.0f;
	// screen related
	bool isFullscreen = false;
	RECT windowedRect;
	DWORD windowedStyle;
	DWORD windowedExStyle;
	int width;
	int height;
	HWND hWnd;
	std::unique_ptr<Graphics> pGfx;
	std::vector<BYTE> rawBuffer;
	std::string commandLine;
	const std::string DEBUG_WINDOW_TITLE = "GameWindow";
	const std::string RELEASE_WINDOW_TITLE = "NeoFramework";
};