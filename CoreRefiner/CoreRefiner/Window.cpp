#include "Window.h"
#include <sstream>
#include "resource.h"
#include "WindowsThrowMacros.h"
#include "imgui/imgui_impl_win32.h"
#include "InputCodex.h"
#include "SoundCodex.h"
#include "RenderGraph.h"


// Window Class Stuff
Window::WindowClass Window::WindowClass::wndClass;
HCURSOR Window::hCursor = nullptr;

Window::WindowClass::WindowClass() noexcept
	:
	hInst( GetModuleHandle( nullptr ) )
{
	WNDCLASSEX wc = { 0 };
	wc.cbSize = sizeof( wc );
	wc.style = CS_OWNDC;
	wc.lpfnWndProc = HandleMsgSetup;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = GetInstance();
	wc.hIcon = static_cast<HICON>(LoadImage( 
		GetInstance(),MAKEINTRESOURCE( IDI_ICON1 ),
		IMAGE_ICON, 0, 0, LR_DEFAULTSIZE
	));
	hCursor = static_cast<HCURSOR>(LoadImage(
		GetInstance(), MAKEINTRESOURCE(IDC_CURSOR1),
		IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE
	));
	wc.hCursor = hCursor ? hCursor : LoadCursor(nullptr, IDC_ARROW);
	wc.hbrBackground = nullptr;
	wc.lpszMenuName = nullptr;
	wc.lpszClassName = GetName();
	wc.hIconSm = static_cast<HICON>(LoadImage(
		GetInstance(),MAKEINTRESOURCE( IDI_ICON1 ),
		IMAGE_ICON, 16, 16, 0
	));
	RegisterClassEx( &wc );
}

Window::WindowClass::~WindowClass()
{
	UnregisterClass( wndClassName,GetInstance() );
}

const char* Window::WindowClass::GetName() noexcept
{
	return wndClassName;
}

HINSTANCE Window::WindowClass::GetInstance() noexcept
{
	return wndClass.hInst;
}


// Window Stuff
Window::Window( int width,int height,const char* name )
	:
	width( width ),
	height( height )
{
	// calculate window size based on desired client region size
	RECT wr;
	wr.left = 100;
	wr.right = width + wr.left;
	wr.top = 100;
	wr.bottom = height + wr.top;
	if( AdjustWindowRect( &wr,WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU,FALSE ) == 0 )
	{
		throw CHWND_LAST_EXCEPT();
	}
	//AdjustWindowRect(&wr, WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU, FALSE);	
	// create window & get hWnd
	hWnd = CreateWindowEx(
		0, WindowClass::GetName(), name,
		WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU,
		CW_USEDEFAULT, CW_USEDEFAULT, wr.right - wr.left, wr.bottom - wr.top,
		nullptr, nullptr, WindowClass::GetInstance(), this
	);
	// check for error
	if( hWnd == nullptr )
	{
		throw CHWND_LAST_EXCEPT();
	}
	// newly created windows start off as hidden
	ShowWindow( hWnd,SW_SHOWDEFAULT );
	// Init ImGui Win32 Impl
	ImGui_ImplWin32_Init( hWnd );
	// create graphics object
	pGfx = std::make_unique<Graphics>( hWnd,width,height );
	// register mouse raw input device
	RAWINPUTDEVICE rid;
	rid.usUsagePage = 0x01; // mouse page
	rid.usUsage = 0x02; // mouse usage
	rid.dwFlags = 0;
	rid.hwndTarget = nullptr;
	if( RegisterRawInputDevices( &rid,1,sizeof( rid ) ) == FALSE )
	{
		throw CHWND_LAST_EXCEPT();
	}

	// Init SoundCodex
	SoundCodex::Get().Init(hWnd);
}

Window::~Window()
{
	ImGui_ImplWin32_Shutdown();
	DestroyWindow( hWnd );

	// Uninit SoundCodex
	SoundCodex::Get().UnInit();
}

void Window::SetTitle( const std::string& title )
{
	if( SetWindowText( hWnd,title.c_str() ) == 0 )
	{
		throw CHWND_LAST_EXCEPT();
	}
}

void Window::SetDebugTitle(int CountFPS)
{
	// Debug Title
	std::string debugStr =  DEBUG_WINDOW_TITLE + 
							" - FPS: " +
							std::to_string(CountFPS);

	SetWindowText(hWnd, debugStr.c_str());
}

// ------------------------------------------------------------------------
// cursor related
void Window::EnableCursor() noexcept
{
	cursorEnabled = true;
	ShowCursor();
	EnableImGuiMouse();
	FreeCursor();

	cursorIdleTimer.Mark();
	cursorHiddenByIdle = false;
}

void Window::DisableCursor() noexcept
{
	cursorEnabled = false;
	HideCursor();
	DisableImGuiMouse();
	ConfineCursor();

	cursorHiddenByIdle = false;
	cursorIdleTimer.Mark();
}

bool Window::CursorEnabled() const noexcept
{
	return cursorEnabled;
}

HCURSOR Window::GetCursor() noexcept 
{
	return hCursor; 
}

void Window::SetCursorHandle(HCURSOR cur) noexcept 
{
	hCursor = cur; 
}

void Window::TickCursorAutoHide() noexcept
{
	if (!autoHideCursor) return;

	// when first-person or locked mode, no participate in automatic hiding
	if (!cursorEnabled) return;

	// Timeout -> Hide
	if (!cursorHiddenByIdle && cursorIdleTimer.Peek() >= cursorHideSeconds)
	{
		HideCursor();
		cursorHiddenByIdle = true;
	}
}

// ------------------------------------------------------------------------
// screen related
// helper
static void GetClientSize(HWND hWnd, UINT& outW, UINT& outH) noexcept
{
	RECT rc{};
	GetClientRect(hWnd, &rc);
	outW = (UINT)(rc.right - rc.left);
	outH = (UINT)(rc.bottom - rc.top);
}
void Window::ToggleFullscreen() noexcept
{
	if( !isFullscreen )
	{
		// Save current window position and size
		GetWindowRect( hWnd, &windowedRect );
		windowedStyle = GetWindowLong( hWnd, GWL_STYLE );
		windowedExStyle = GetWindowLong( hWnd, GWL_EXSTYLE );

		// Get main display information
		MONITORINFO mi = { sizeof( MONITORINFO ) };
		GetMonitorInfo( MonitorFromWindow( hWnd, MONITOR_DEFAULTTONEAREST ), &mi );

		// Set borderless full-screen style
		SetWindowLong( hWnd, GWL_STYLE, windowedStyle & ~( WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU ) );
		SetWindowLong( hWnd, GWL_EXSTYLE, windowedExStyle & ~( WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE ) );

		// Set window position and size to full screen
		SetWindowPos( hWnd, HWND_TOP,
			mi.rcMonitor.left,
			mi.rcMonitor.top,
			mi.rcMonitor.right - mi.rcMonitor.left,
			mi.rcMonitor.bottom - mi.rcMonitor.top,
			SWP_FRAMECHANGED | SWP_NOZORDER | SWP_NOACTIVATE );

		// Update the size of the Graphics (using the original resolution; the Graphics internally handles letterboxing)
		UINT cw, ch;
		GetClientSize(hWnd, cw, ch);
		pGfx->OnWindowResize(cw, ch);

		isFullscreen = true;
	}
	else
	{
		// Restore window style
		SetWindowLong( hWnd, GWL_STYLE, windowedStyle );
		SetWindowLong( hWnd, GWL_EXSTYLE, windowedExStyle );

		// Restore window position and size
		SetWindowPos( hWnd, HWND_NOTOPMOST,
			windowedRect.left,
			windowedRect.top,
			windowedRect.right - windowedRect.left,
			windowedRect.bottom - windowedRect.top,
			SWP_FRAMECHANGED | SWP_NOZORDER | SWP_NOACTIVATE );

		// Restore Graphics Size
		UINT cw, ch;
		GetClientSize(hWnd, cw, ch);
		pGfx->OnWindowResize(cw, ch);

		isFullscreen = false;
	}
}

bool Window::IsFullscreen() const noexcept
{
	return isFullscreen;
}

std::optional<int> Window::ProcessMessages() noexcept
{
	MSG msg;
	// while queue has messages, remove and dispatch them (but do not block on empty queue)
	while( PeekMessage( &msg,nullptr,0,0,PM_REMOVE ) )
	{
		// check for quit because peekmessage does not signal this via return val
		if( msg.message == WM_QUIT )
		{
			// return optional wrapping int (arg to PostQuitMessage is in wparam) signals quit
			return (int)msg.wParam;
		}

		// TranslateMessage will post auxilliary WM_CHAR messages from key msgs
		TranslateMessage( &msg );
		DispatchMessage( &msg );
	}

	// return empty optional when not quitting app
	return {};
}

Graphics& Window::Gfx()
{
	if( !pGfx )
	{
		throw CHWND_NOGFX_EXCEPT();
	}
	return *pGfx;
}

void Window::ConfineCursor() noexcept
{
	RECT rect; 
	GetClientRect( hWnd,&rect );
	MapWindowPoints( hWnd,nullptr,reinterpret_cast<POINT*>(&rect),2 );
	ClipCursor( &rect );
}

void Window::FreeCursor() noexcept
{
	ClipCursor( nullptr );
}

void Window::HideCursor() noexcept
{
	while( ::ShowCursor( FALSE ) >= 0 );
}

void Window::ShowCursor() noexcept
{
	while( ::ShowCursor( TRUE ) < 0 );
}

void Window::EnableImGuiMouse() noexcept
{
	ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
}

void Window::DisableImGuiMouse() noexcept
{
	ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouse;
}

bool Window::MapClientToGame(int cx, int cy, int& outX, int& outY) noexcept
{
	if (!pGfx) return false;

	const D3D11_VIEWPORT vp = pGfx->GetMainViewport();

	const float vx = (float)cx - vp.TopLeftX;
	const float vy = (float)cy - vp.TopLeftY;

	// Black border area: deemed invalid
	if (vx < 0.0f || vy < 0.0f || vx >= vp.Width || vy >= vp.Height)
		return false;

	const float nx = vx / vp.Width;   // 0..1
	const float ny = vy / vp.Height;  // 0..1

	outX = (int)(nx * (float)Graphics::LogicalCanvasWidth());
	outY = (int)(ny * (float)Graphics::LogicalCanvasHeight());
	return true;
}

LRESULT CALLBACK Window::HandleMsgSetup( HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam ) noexcept
{
	// use create parameter passed in from CreateWindow() to store window class pointer at WinAPI side
	if( msg == WM_NCCREATE )
	{
		// extract ptr to window class from creation data
		const CREATESTRUCTW* const pCreate = reinterpret_cast<CREATESTRUCTW*>(lParam);
		Window* const pWnd = static_cast<Window*>(pCreate->lpCreateParams);
		// set WinAPI-managed user data to store ptr to window instance
		SetWindowLongPtr( hWnd,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(pWnd) );
		// set message proc to normal (non-setup) handler now that setup is finished
		SetWindowLongPtr( hWnd,GWLP_WNDPROC,reinterpret_cast<LONG_PTR>(&Window::HandleMsgThunk) );
		// forward message to window instance handler
		return pWnd->HandleMsg( hWnd,msg,wParam,lParam );
	}
	// if we get a message before the WM_NCCREATE message, handle with default handler
	return DefWindowProc( hWnd,msg,wParam,lParam );
}

LRESULT CALLBACK Window::HandleMsgThunk( HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam ) noexcept
{
	// retrieve ptr to window instance
	Window* const pWnd = reinterpret_cast<Window*>(GetWindowLongPtr( hWnd,GWLP_USERDATA ));
	// forward message to window instance handler
	return pWnd->HandleMsg( hWnd,msg,wParam,lParam );
}

LRESULT Window::HandleMsg( HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam ) noexcept
{
	// deal cursor
	if (msg == WM_SETCURSOR)
	{
		if (LOWORD(lParam) == HTCLIENT)
		{
			if (!cursorHiddenByIdle)
				::SetCursor(GetCursor());
			return TRUE;
		}
	}
	auto WakeCursor = [&]()
		{
			cursorIdleTimer.Mark();
			if (cursorHiddenByIdle)
			{
				ShowCursor();
				cursorHiddenByIdle = false;
				::SetCursor(GetCursor());
			}
		};

	// deal imgui
	if( ImGui_ImplWin32_WndProcHandler( hWnd,msg,wParam,lParam ) )
	{
		return true;
	}
	const auto& imio = ImGui::GetIO();

	auto& input = InputCodex::Get();
	switch (msg)
	{
		// we don't want the DefProc to handle this message because
		// we want our destructor to destroy the window, so return 0 instead of break
	case WM_CLOSE:
		PostQuitMessage(0);
		return 0;
		// clear keystate when window loses focus to prevent input getting "stuck"
	case WM_KILLFOCUS:
		input.ClearAll();
		break;
	case WM_ACTIVATE:
		// confine/free cursor on window to foreground/background if cursor disabled
		if (!cursorEnabled)
		{
			if (wParam & WA_ACTIVE)
			{
				ConfineCursor();
				HideCursor();
			}
			else
			{
				FreeCursor();
				ShowCursor();
			}
		}
		break;

		/*********** KEYBOARD MESSAGES ***********/
	case WM_KEYDOWN:
		// syskey commands need to be handled to track ALT key (VK_MENU) and F10
	case WM_SYSKEYDOWN:
		// stifle this keyboard message if imgui wants to capture
		if (imio.WantCaptureKeyboard)
		{
			break;
		}
		if (!(lParam & 0x40000000) || input.KeyAutorepeatEnabled()) // filter autorepeat
		{
			input.OnKeyDown(static_cast<unsigned char>(wParam));
		}
		break;
	case WM_KEYUP:
	case WM_SYSKEYUP:
		// stifle this keyboard message if imgui wants to capture
		if (imio.WantCaptureKeyboard)
		{
			break;
		}
		input.OnKeyUp(static_cast<unsigned char>(wParam));
		break;
	case WM_CHAR:
		// stifle this keyboard message if imgui wants to capture
		if (imio.WantCaptureKeyboard)
		{
			break;
		}
		input.OnChar(static_cast<unsigned char>(wParam));
		break;
		/*********** END KEYBOARD MESSAGES ***********/

		/************* MOUSE MESSAGES ****************/
	case WM_MOUSEMOVE:
	{
		WakeCursor();

		const POINTS pt = MAKEPOINTS(lParam);
		// cursorless exclusive gets first dibs
		if (!cursorEnabled)
		{
			if (!input.MouseInWindow())
			{
				SetCapture(hWnd);
				input.OnMouseEnter();
				HideCursor();
			}
			break;
		}
		// stifle this mouse message if imgui wants to capture
		if (imio.WantCaptureMouse) break;

		// in client region -> log move, and log enter + capture mouse (if not previously in window)
		int gx, gy;
		if (MapClientToGame(pt.x, pt.y, gx, gy))
		{
			input.OnMouseMove(gx, gy);
			if (!input.MouseInWindow())
			{
				SetCapture(hWnd);
				input.OnMouseEnter();
			}
		}
		// not in client -> log move / maintain capture if button down
		else
		{
			if (wParam & (MK_LBUTTON | MK_RBUTTON))
			{
				input.OnMouseMove(pt.x, pt.y);
			}
			// button up -> release capture / log event for leaving
			else
			{
				ReleaseCapture();
				input.OnMouseLeave();
			}
		}
		break;
	}
	case WM_LBUTTONDOWN:
	{
		WakeCursor();

		SetForegroundWindow(hWnd);
		if (!cursorEnabled)
		{
			ConfineCursor();
			HideCursor();
		}
		// stifle this mouse message if imgui wants to capture
		if (imio.WantCaptureMouse) break;

		const POINTS pt = MAKEPOINTS(lParam);
		int gx, gy;
		if (!MapClientToGame(pt.x, pt.y, gx, gy)) break;
		input.OnLeftDown(gx, gy);
		break;
	}
	case WM_RBUTTONDOWN:
	{
		WakeCursor();

		// stifle this mouse message if imgui wants to capture
		if (imio.WantCaptureMouse) break;

		const POINTS pt = MAKEPOINTS(lParam);
		int gx, gy;
		if (!MapClientToGame(pt.x, pt.y, gx, gy)) break;
		input.OnRightDown(gx, gy);
		break;
	}
	case WM_LBUTTONUP:
	{
		WakeCursor();

		// stifle this mouse message if imgui wants to capture
		if (imio.WantCaptureMouse) break;

		const POINTS pt = MAKEPOINTS(lParam);
		int gx, gy;
		// release mouse if outside of window
		if (!MapClientToGame(pt.x, pt.y, gx, gy))
		{
			input.OnLeftUp(0, 0);
			ReleaseCapture();
			input.OnMouseLeave();
			break;
		}
		input.OnLeftUp(gx, gy);
		break;
	}
	case WM_RBUTTONUP:
	{
		WakeCursor();

		// stifle this mouse message if imgui wants to capture
		if (imio.WantCaptureMouse) break;

		const POINTS pt = MAKEPOINTS(lParam);
		int gx, gy;
		// release mouse if outside of window
		if (!MapClientToGame(pt.x, pt.y, gx, gy))
		{
			input.OnRightUp(0, 0);
			ReleaseCapture();
			input.OnMouseLeave();
			break;
		}
		input.OnRightUp(gx, gy);
		break;
	}
	case WM_MOUSEWHEEL:
	{
		WakeCursor();

		// stifle this mouse message if imgui wants to capture
		if (imio.WantCaptureMouse) break;

		const POINTS pt = MAKEPOINTS(lParam);
		const int delta = GET_WHEEL_DELTA_WPARAM(wParam);

		int gx, gy;
		if (!MapClientToGame(pt.x, pt.y, gx, gy)) break;
		input.OnWheelDelta(gx, gy, delta);
		break;
	}
	/************** END MOUSE MESSAGES **************/

	/************** RAW MOUSE MESSAGES **************/
	case WM_INPUT:
	{
		if (!input.RawMouseEnabled())
		{
			break;
		}
		UINT size;
		// first get the size of the input data
		if (GetRawInputData(
			reinterpret_cast<HRAWINPUT>(lParam),
			RID_INPUT,
			nullptr,
			&size,
			sizeof(RAWINPUTHEADER)) == -1)
		{
			// bail msg processing if error
			break;
		}
		rawBuffer.resize(size);
		// read in the input data
		if (GetRawInputData(
			reinterpret_cast<HRAWINPUT>(lParam),
			RID_INPUT,
			rawBuffer.data(),
			&size,
			sizeof(RAWINPUTHEADER)) != size)
		{
			// bail msg processing if error
			break;
		}
		// process the raw input data
		auto& ri = reinterpret_cast<const RAWINPUT&>(*rawBuffer.data());
		if (ri.header.dwType == RIM_TYPEMOUSE &&
			(ri.data.mouse.lLastX != 0 || ri.data.mouse.lLastY != 0))
		{
			WakeCursor();
			input.OnRawDelta(ri.data.mouse.lLastX, ri.data.mouse.lLastY);
		}
		break;
	}
	/************** END RAW MOUSE MESSAGES **************/
	}

	return DefWindowProc( hWnd,msg,wParam,lParam );
}


 //Window Exception Stuff
std::string Window::Exception::TranslateErrorCode( HRESULT hr ) noexcept
{
	char* pMsgBuf = nullptr;
	// windows will allocate memory for err string and make our pointer point to it
	const DWORD nMsgLen = FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		nullptr,hr,MAKELANGID( LANG_NEUTRAL,SUBLANG_DEFAULT ),
		reinterpret_cast<LPSTR>(&pMsgBuf),0,nullptr
	);
	// 0 string length returned indicates a failure
	if( nMsgLen == 0 )
	{
		return "Unidentified error code";
	}
	// copy error string from windows-allocated buffer to std::string
	std::string errorString = pMsgBuf;
	// free windows buffer
	LocalFree( pMsgBuf );
	return errorString;
}


Window::HrException::HrException( int line,const char* file,HRESULT hr ) noexcept
	:
	Exception( line,file ),
	hr( hr )
{}

const char* Window::HrException::what() const noexcept
{
	std::ostringstream oss;
	oss << GetType() << std::endl
		<< "[Error Code] 0x" << std::hex << std::uppercase << GetErrorCode()
		<< std::dec << " (" << (unsigned long)GetErrorCode() << ")" << std::endl
		<< "[Description] " << GetErrorDescription() << std::endl
		<< GetOriginString();
	whatBuffer = oss.str();
	return whatBuffer.c_str();
}

const char* Window::HrException::GetType() const noexcept
{
	return "Window Exception";
}

HRESULT Window::HrException::GetErrorCode() const noexcept
{
	return hr;
}

std::string Window::HrException::GetErrorDescription() const noexcept
{
	return Exception::TranslateErrorCode( hr );
}


const char* Window::NoGfxException::GetType() const noexcept
{
	return "Window Exception [No Graphics]";
}