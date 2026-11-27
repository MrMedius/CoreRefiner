#pragma once
#include <Windows.h>
#include <XInput.h>
#include <cmath>

#pragma comment(lib, "xinput.lib")

class Gamepad
{
public:
    Gamepad() = default;
    explicit Gamepad(DWORD playerIndex) noexcept { SetPlayerIndex(playerIndex); }

    Gamepad(const Gamepad&) = delete;
    Gamepad& operator=(const Gamepad&) = delete;

    void Update() noexcept;

    // total 0..3
    void SetPlayerIndex(DWORD idx) noexcept { playerIndex = (idx > 3) ? 0 : idx; }
    DWORD GetPlayerIndex() const noexcept   { return playerIndex; }

    bool IsConnected() const noexcept { return connected; }

    // Stick Axis: -1..1
    float LeftX()  const noexcept { return leftX; }
    float LeftY()  const noexcept { return leftY; }
    float RightX() const noexcept { return rightX; }
    float RightY() const noexcept { return rightY; }

    // Trigger Axis: 0..1
    float LT_Axis() const noexcept { return ltAxis; }
    float RT_Axis() const noexcept { return rtAxis; }

    // Buttons
    bool ButtonPressed(WORD btn) const noexcept;
    bool ButtonTriggered(WORD btn) const noexcept;
    bool ButtonReleased(WORD btn) const noexcept;

    // Trigger as Button
    bool LT_Pressed() const noexcept    { return ltDown; }
    bool LT_Triggered() const noexcept  { return ltDown && !ltDownOld; }
    bool LT_Released() const noexcept   { return !ltDown && ltDownOld; }

    bool RT_Pressed() const noexcept    { return rtDown; }
    bool RT_Triggered() const noexcept  { return rtDown && !rtDownOld; }
    bool RT_Released() const noexcept   { return !rtDown && rtDownOld; }

    bool HasAnyInput() const noexcept
    {
        if (!connected) return false;
        return state.Gamepad.wButtons != 0
            || ltDown || rtDown   
            || abs(leftX)  > 0.0f 
            || abs(leftY)  > 0.0f 
            || abs(rightX) > 0.0f 
            || abs(rightY) > 0.0f;
    }

    // ---- Common button constants
    static constexpr WORD GP_A          = XINPUT_GAMEPAD_A;
    static constexpr WORD GP_B          = XINPUT_GAMEPAD_B;
    static constexpr WORD GP_X          = XINPUT_GAMEPAD_X;
    static constexpr WORD GP_Y          = XINPUT_GAMEPAD_Y;
    static constexpr WORD GP_LB         = XINPUT_GAMEPAD_LEFT_SHOULDER;
    static constexpr WORD GP_RB         = XINPUT_GAMEPAD_RIGHT_SHOULDER;
    static constexpr WORD GP_BACK       = XINPUT_GAMEPAD_BACK;
    static constexpr WORD GP_START      = XINPUT_GAMEPAD_START;
    static constexpr WORD GP_LS         = XINPUT_GAMEPAD_LEFT_THUMB;
    static constexpr WORD GP_RS         = XINPUT_GAMEPAD_RIGHT_THUMB;
    static constexpr WORD GP_DPAD_UP    = XINPUT_GAMEPAD_DPAD_UP;
    static constexpr WORD GP_DPAD_DOWN  = XINPUT_GAMEPAD_DPAD_DOWN;
    static constexpr WORD GP_DPAD_LEFT  = XINPUT_GAMEPAD_DPAD_LEFT;
    static constexpr WORD GP_DPAD_RIGHT = XINPUT_GAMEPAD_DPAD_RIGHT;

    // Tuning
    void SetLeftDeadZone(float dz) noexcept     { deadZoneLeft = Clamp01_(dz); }
    void SetRightDeadZone(float dz) noexcept    { deadZoneRight = Clamp01_(dz); }
    void SetTriggerThreshold(float th) noexcept { triggerThreshold = Clamp01_(th); }

    float GetLeftDeadZone() const noexcept      { return deadZoneLeft; }
    float GetRightDeadZone() const noexcept     { return deadZoneRight; }
    float GetTriggerThreshold() const noexcept  { return triggerThreshold; }

    // Vibration
    void SetVibration(float leftMotor, float rightMotor) noexcept; // maintain
    void SetVibrationPulse(float leftMotor, float rightMotor, int frames) noexcept; // pulse

private:
    static float Clamp01_(float v) noexcept
    {
        if (v < 0.0f) return 0.0f;
        if (v > 1.0f) return 1.0f;
        return v;
    }

    static float NormalizeThumb_(SHORT v) noexcept; // -1..1
    static void ApplyDeadZone_(float& x, float& y, float deadZone) noexcept;

private:
    DWORD playerIndex{ 0 };

    bool connected{ false };
    XINPUT_STATE state{};
    XINPUT_STATE stateOld{};

    // axes
    float leftX{ 0.0f }, leftY{ 0.0f };
    float rightX{ 0.0f }, rightY{ 0.0f };
    float ltAxis{ 0.0f }, rtAxis{ 0.0f };

    // trigger-as-button
    bool ltDown{ false }, ltDownOld{ false };
    bool rtDown{ false }, rtDownOld{ false };

    // tuning
    float deadZoneLeft{ 0.2f };
    float deadZoneRight{ 0.2f };
    float triggerThreshold{ 0.25f };

    // pulse vibration
    int   vibFramesLeft{ 0 };
    float vibL{ 0.0f };
    float vibR{ 0.0f };
};
