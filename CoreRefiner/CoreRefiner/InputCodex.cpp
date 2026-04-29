#include "InputCodex.h"


// Clear States
void InputCodex::ClearAll() noexcept
{
    keyboard.ClearState();
    mouse.Flush();
    keyboard.Flush();
}

// Keyboard injection
void InputCodex::OnKeyDown(unsigned char keycode) noexcept
{
    keyboard.OnKeyPressed(keycode);
}
void InputCodex::OnKeyUp(unsigned char keycode) noexcept
{
    keyboard.OnKeyReleased(keycode);
}
void InputCodex::OnChar(char character) noexcept
{
    keyboard.OnChar(character);
}

// Mouse injection
void InputCodex::OnMouseMove(int x, int y) noexcept
{
    mouse.OnMouseMove(x, y);
}
void InputCodex::OnMouseEnter() noexcept
{
    mouse.OnMouseEnter();
}
void InputCodex::OnMouseLeave() noexcept
{
    mouse.OnMouseLeave();
}

void InputCodex::OnLeftDown(int x, int y) noexcept
{
    mouse.OnLeftPressed(x, y);
}
void InputCodex::OnLeftUp(int x, int y) noexcept
{
    mouse.OnLeftReleased(x, y);
}
void InputCodex::OnRightDown(int x, int y) noexcept
{
    mouse.OnRightPressed(x, y);
}
void InputCodex::OnRightUp(int x, int y) noexcept
{
    mouse.OnRightReleased(x, y);
}

void InputCodex::OnWheelDelta(int x, int y, int delta) noexcept
{
    mouse.OnWheelDelta(x, y, delta);
}

void InputCodex::OnRawDelta(int dx, int dy) noexcept
{
    if (mouse.RawEnabled())
    {
        mouse.OnRawDelta(dx, dy);
    }
}