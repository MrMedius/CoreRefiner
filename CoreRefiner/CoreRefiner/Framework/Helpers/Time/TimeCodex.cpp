#include "TimeCodex.h"

TimeCodex& TimeCodex::Get() noexcept
{
    static TimeCodex inst;
    return inst;
}

void TimeCodex::Update(float dt) noexcept
{
    data_.unscaledDeltaTime = dt;
    data_.deltaTime = dt * data_.timeScale;
    data_.totalTime += data_.deltaTime;
}

void TimeCodex::SetTimeScale(float scale) noexcept
{
    data_.timeScale = scale;
}

void TimeCodex::Reset() noexcept
{
    data_ = {};
}