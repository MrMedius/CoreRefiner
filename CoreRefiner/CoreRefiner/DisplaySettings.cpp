#include "DisplaySettings.h"

#include "GameStatsCodex.h"
#include "Window.h"

void ApplyFullscreen(Window& wnd, bool enable)
{
	wnd.SetFullscreen(enable);
	GameStatsCodex::SetFullscreen(wnd.IsFullscreen());
}

void ApplyWindowSizeIndex(Window& wnd, int index)
{
	GameStatsCodex::SetWindowSizeIndex(index);
	switch (GameStatsCodex::GetWindowSizeIndex())
	{
	case 1:
		wnd.SetWindowedClientSize(1600, 900);
		break;
	case 2:
		wnd.SetWindowedClientSize(1920, 1080);
		break;
	default:
		wnd.SetWindowedClientSize(1280, 720);
		break;
	}
}
