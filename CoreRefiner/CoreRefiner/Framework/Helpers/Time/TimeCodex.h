#pragma once

struct TimeData
{
	float totalTime = 0.0f;         // 累计时间，单位秒。每帧累加 deltaTime * timeScale。
	float deltaTime = 0.0f;         // 本帧时间增量，单位秒。每帧更新为 unscaledDeltaTime * timeScale。
	float unscaledDeltaTime = 0.0f; // 本帧时间增量，单位秒。每帧更新为上一帧的真实时间增量。
	float timeScale = 1.0f;         // 时间缩放系数。1.0 正常，0.5 半速，2.0 双倍速。可在运行时修改。
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