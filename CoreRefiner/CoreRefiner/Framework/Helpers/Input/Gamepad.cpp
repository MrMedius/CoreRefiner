#include "Gamepad.h"
#include <cmath>

float Gamepad::NormalizeThumb_(SHORT v) noexcept
{
    // SHORT: -32768..32767 -> -1..1inote : negative half-side has a larger rangej
    if (v >= 0) return static_cast<float>(v) / 32767.0f;
    return static_cast<float>(v) / 32768.0f;
}

void Gamepad::ApplyDeadZone_(float& x, float& y, float deadZone) noexcept
{
    const float len = std::sqrt(x * x + y * y);
    if (len <= deadZone)
    {
        x = 0.0f; y = 0.0f;
        return;
    }

    // Linear scaling outside the dead zone ensures the edge remains 1.
    const float scale = (len - deadZone) / (1.0f - deadZone);
    const float s = (scale > 1.0f) ? 1.0f : scale;

    const float nx = x / len;
    const float ny = y / len;
    x = nx * s;
    y = ny * s;
}

void Gamepad::Update() noexcept
{
    // save the previous frame
    stateOld = state;
    ltDownOld = ltDown;
    rtDownOld = rtDown;

    const DWORD result = XInputGetState(playerIndex, &state);
    connected = (result == ERROR_SUCCESS);

    if (!connected)
    {
        leftX = leftY = 0.0f;
        rightX = rightY = 0.0f;
        ltAxis = rtAxis = 0.0f;
        ltDown = rtDown = false;
        vibFramesLeft = 0;
        return;
    }

    // sticks
    float lx = NormalizeThumb_(state.Gamepad.sThumbLX);
    float ly = NormalizeThumb_(state.Gamepad.sThumbLY);
    ApplyDeadZone_(lx, ly, deadZoneLeft);
    leftX = lx; leftY = ly;

    float rx = NormalizeThumb_(state.Gamepad.sThumbRX);
    float ry = NormalizeThumb_(state.Gamepad.sThumbRY);
    ApplyDeadZone_(rx, ry, deadZoneRight);
    rightX = rx; rightY = ry;

    // triggers 0..255 -> 0..1
    ltAxis = static_cast<float>(state.Gamepad.bLeftTrigger) / 255.0f;
    rtAxis = static_cast<float>(state.Gamepad.bRightTrigger) / 255.0f;

    // trigger as button
    ltDown = (ltAxis >= triggerThreshold);
    rtDown = (rtAxis >= triggerThreshold);

    // pulse vibration countdown
    if (vibFramesLeft > 0)
    {
        --vibFramesLeft;
        if (vibFramesLeft == 0)
        {
            SetVibration(0.0f, 0.0f);
        }
    }
}

bool Gamepad::ButtonPressed(WORD btn) const noexcept
{
    if (!connected) return false;
    return (state.Gamepad.wButtons & btn) != 0;
}

bool Gamepad::ButtonTriggered(WORD btn) const noexcept
{
    if (!connected) return false;
    const bool nowDown = (state.Gamepad.wButtons & btn) != 0;
    const bool oldDown = (stateOld.Gamepad.wButtons & btn) != 0;
    return nowDown && !oldDown;
}

bool Gamepad::ButtonReleased(WORD btn) const noexcept
{
    if (!connected) return false;
    const bool nowDown = (state.Gamepad.wButtons & btn) != 0;
    const bool oldDown = (stateOld.Gamepad.wButtons & btn) != 0;
    return !nowDown && oldDown;
}

void Gamepad::SetVibration(float leftMotor, float rightMotor) noexcept
{
    if (!connected) return;

    leftMotor = Clamp01_(leftMotor);
    rightMotor = Clamp01_(rightMotor);

    XINPUT_VIBRATION vib{};
    vib.wLeftMotorSpeed = static_cast<WORD>(leftMotor * 65535.0f);
    vib.wRightMotorSpeed = static_cast<WORD>(rightMotor * 65535.0f);
    XInputSetState(playerIndex, &vib);
}

void Gamepad::SetVibrationPulse(float leftMotor, float rightMotor, int frames) noexcept
{
    vibL = leftMotor;
    vibR = rightMotor;
    vibFramesLeft = (frames > 0) ? frames : 0;

    if (connected && vibFramesLeft > 0)
    {
        SetVibration(vibL, vibR);
    }
}
