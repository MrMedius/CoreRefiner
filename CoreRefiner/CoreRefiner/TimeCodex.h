#pragma once

struct TimeData
{
    float totalTime = 0.0f;
    float deltaTime = 0.0f;
    float unscaledDeltaTime = 0.0f;
    float timeScale = 1.0f;
};

class TimeCodex
{
public:
    static TimeCodex& Get() noexcept;

    TimeCodex(const TimeCodex&) = delete;
    TimeCodex& operator=(const TimeCodex&) = delete;

    void Update(float dt) noexcept;

    void SetTimeScale(float scale) noexcept;
    [[nodiscard]] float GetTimeScale() const noexcept { return data_.timeScale; }

    void Reset() noexcept;

    [[nodiscard]] float GetTotalTime() const noexcept { return data_.totalTime; }
    [[nodiscard]] float GetDeltaTime() const noexcept { return data_.deltaTime; }
    [[nodiscard]] float GetUnscaledDeltaTime() const noexcept { return data_.unscaledDeltaTime; }

    [[nodiscard]] const TimeData& GetData() const noexcept { return data_; }

private:
    TimeCodex() = default;

    TimeData data_{};
};