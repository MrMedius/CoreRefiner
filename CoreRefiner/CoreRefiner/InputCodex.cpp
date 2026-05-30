#include "InputCodex.h"


// Clear States
void InputCodex::ClearAll() noexcept
{
    keyboard.ClearState();
    mouse.Flush();
    keyboard.Flush();
    ClearTextInput();
    textCaptureRequested_ = false;
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

void InputCodex::OnTextCommitUtf8(std::string utf8)
{
    if (utf8.empty())
        return;
    pendingTextCommits_.push_back(std::move(utf8));
}

void InputCodex::OnImeComposition(std::string utf8, const bool active)
{
    imeCompositionUtf8_ = std::move(utf8);
    imeCompositionActive_ = active;
    if (!active)
        imeCompositionUtf8_.clear();
}

void InputCodex::DrainTextFrame(TextInputFrame& out)
{
    out.commitUtf8.insert(
        out.commitUtf8.end(),
        pendingTextCommits_.begin(),
        pendingTextCommits_.end());
    pendingTextCommits_.clear();

    out.imeCompositionUtf8 = imeCompositionUtf8_;
    out.imeCompositionActive = imeCompositionActive_;
}

void InputCodex::ClearTextInput() noexcept
{
    pendingTextCommits_.clear();
    imeCompositionUtf8_.clear();
    imeCompositionActive_ = false;
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