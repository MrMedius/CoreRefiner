#pragma once
#include <optional>
#include "Keyboard.h"
#include "Mouse.h"
#include "Gamepad.h"

class InputCodex
{
    friend class Window;
public:
    static InputCodex& Get() noexcept
    {
        static InputCodex inst;
        return inst;
    }

    InputCodex(const InputCodex&) = delete;
    InputCodex& operator=(const InputCodex&) = delete;

    void Update() noexcept
    {
        keyboard.Update();
        mouse.Update();
        for (int i = 0; i < kMaxPads; ++i) pads[i].Update();
    }

    // -----------------------------------------------------------
    // Keyboard query
    bool KeyPressed(unsigned char key) const noexcept   { return keyboard.KeyIsPressed(key); }
    bool KeyTriggered(unsigned char key) const noexcept { return keyboard.KeyIsTriggered(key); }
    bool KeyReleased(unsigned char key) const noexcept  { return keyboard.KeyIsReleased(key); }

    std::optional<Keyboard::Event> ReadKey() noexcept   { return keyboard.ReadKey(); }
    std::optional<char> ReadChar() noexcept             { return keyboard.ReadChar(); }
    void FlushKeyboard() noexcept                       { keyboard.Flush(); }

    void EnableKeyAutorepeat() noexcept         { keyboard.EnableAutorepeat(); }
    void DisableKeyAutorepeat() noexcept        { keyboard.DisableAutorepeat(); }
    bool KeyAutorepeatEnabled() const noexcept  { return keyboard.AutorepeatIsEnabled(); }

    bool KeyboardInput() noexcept { return keyboard.AnyKeyPressed(); }


    // -----------------------------------------------------------
    // Mouse query
    std::pair<int, int> MousePos() const noexcept   { return mouse.GetPos(); }
    int MouseX() const noexcept                     { return mouse.GetPosX(); }
    int MouseY() const noexcept                     { return mouse.GetPosY(); }
    bool MouseMovedOrButtons() const noexcept       { return mouse.IsMove() || MouseLeftPressed() || MouseRightPressed(); }
    bool MouseInWindow() const noexcept             { return mouse.IsInWindow(); }

    bool MouseLeftPressed() const noexcept      { return mouse.LeftIsPressed(); }
    bool MouseLeftTriggered() const noexcept    { return mouse.LeftIsTriggered(); }
    bool MouseLeftReleased() const noexcept     { return mouse.LeftIsReleased(); }

    bool MouseRightPressed() const noexcept     { return mouse.RightIsPressed(); }
    bool MouseRightTriggered() const noexcept   { return mouse.RightIsTriggered(); }
    bool MouseRightReleased() const noexcept    { return mouse.RightIsReleased(); }

    std::optional<Mouse::Event> ReadMouse() noexcept { return mouse.Read(); }
    void FlushMouse() noexcept                       { mouse.Flush(); }

    void EnableRawMouse() noexcept  { mouse.EnableRaw(); }
    void DisableRawMouse() noexcept { mouse.DisableRaw(); }
    bool RawMouseEnabled() const noexcept                   { return mouse.RawEnabled(); }
    std::optional<Mouse::RawDelta> ReadRawDelta() noexcept  { return mouse.ReadRawDelta(); }

    bool MouseInput() noexcept { return MouseMovedOrButtons(); }


    // -----------------------------------------------------------
    // Gamepad query (by index)
    bool PadConnected(int idx) const noexcept   { return Pad(idx).IsConnected(); }

    int FindFirstConnectedPad() const noexcept
    {
        for (int i = 0; i < kMaxPads; ++i)
            if (pads[i].IsConnected()) return i;
        return -1;
    }
    int PollJoinRequestPadIndex(WORD b) noexcept
    {
        for (int i = 0; i < kMaxPads; ++i)
            if (pads[i].IsConnected() && pads[i].ButtonTriggered(b))
                return i;
        return -1;
    }

    bool GP_Pressed(int idx, WORD b) const noexcept     { return Pad(idx).ButtonPressed(b); }
    bool GP_Triggered(int idx, WORD b) const noexcept   { return Pad(idx).ButtonTriggered(b); }
    bool GP_Released(int idx, WORD b) const noexcept    { return Pad(idx).ButtonReleased(b); }

    float GP_LeftX(int idx)  const noexcept { return Pad(idx).LeftX(); }
    float GP_LeftY(int idx)  const noexcept { return Pad(idx).LeftY(); }
    float GP_RightX(int idx) const noexcept { return Pad(idx).RightX(); }
    float GP_RightY(int idx) const noexcept { return Pad(idx).RightY(); }

    float GP_LT(int idx) const noexcept { return Pad(idx).LT_Axis(); }
    float GP_RT(int idx) const noexcept { return Pad(idx).RT_Axis(); }

    bool GP_LT_Triggered(int idx) const noexcept { return Pad(idx).LT_Triggered(); }
    bool GP_RT_Triggered(int idx) const noexcept { return Pad(idx).RT_Triggered(); }

    bool GamepadInput(int idx) const noexcept { return Pad(idx).HasAnyInput(); }

    bool AnyGamepadInput() const noexcept
    {
        for (int i = 0; i < kMaxPads; ++i)
            if (pads[i].HasAnyInput()) return true;
        return false;
    }

    void GP_SetVibration(int idx, float leftMotor, float rightMotor) noexcept
    {
        if(GamepadInput(idx)) Pad(idx).SetVibration(leftMotor, rightMotor);
    }
    void GP_SetVibrationPulse(int idx, float leftMotor, float rightMotor, int frames) noexcept
    {
        if (GamepadInput(idx)) Pad(idx).SetVibrationPulse(leftMotor, rightMotor, frames);
    }
    void GP_SetVibrationAll(float leftMotor, float rightMotor) noexcept
    {
        for (int i = 0; i < kMaxPads; ++i)
            if (pads[i].IsConnected())
                pads[i].SetVibration(leftMotor, rightMotor);
    }
    void GP_SetVibrationPulseAll(float leftMotor, float rightMotor, int frames) noexcept
    {
        for (int i = 0; i < kMaxPads; ++i)
            if (pads[i].IsConnected())
                pads[i].SetVibrationPulse(leftMotor, rightMotor, frames);
    }


private:
    // Injection API (called from Window/WndProc)
    void ClearAll() noexcept;

    void OnKeyDown(unsigned char keycode) noexcept;
    void OnKeyUp(unsigned char keycode) noexcept;
    void OnChar(char character) noexcept;

    void OnMouseMove(int x, int y) noexcept;
    void OnMouseEnter() noexcept;
    void OnMouseLeave() noexcept;

    void OnLeftDown(int x, int y) noexcept;
    void OnLeftUp(int x, int y) noexcept;
    void OnRightDown(int x, int y) noexcept;
    void OnRightUp(int x, int y) noexcept;

    void OnWheelDelta(int x, int y, int delta) noexcept;
    void OnRawDelta(int dx, int dy) noexcept;

    // Direct access (optional)
    // Keyboard
    Keyboard& Kbd() noexcept { return keyboard; }
    const Keyboard& Kbd() const noexcept { return keyboard; }
    // Mouse
    Mouse& Mse() noexcept { return mouse; }
    const Mouse& Mse() const noexcept { return mouse; }
    // Gamepad
    static constexpr bool IsValidPadIndex(int idx) noexcept { return idx >= 0 && idx < kMaxPads; }
    Gamepad& Pad(int idx) noexcept
    {
#ifdef _DEBUG
        if (!IsValidPadIndex(idx)) __debugbreak();
#endif
        if (idx < 0) idx = 0;
        if (idx >= kMaxPads) idx = kMaxPads - 1;
        return pads[idx];
    }
    const Gamepad& Pad(int idx) const noexcept
    {
        if (idx < 0) idx = 0;
        if (idx >= kMaxPads) idx = kMaxPads - 1;
        return pads[idx];
    }

private:
    InputCodex() = default;
    ~InputCodex() = default;

private:
    Keyboard keyboard;
    Mouse mouse;
    static constexpr int kMaxPads = 4;
    Gamepad pads[kMaxPads] = { Gamepad(0), Gamepad(1), Gamepad(2), Gamepad(3) };
};
