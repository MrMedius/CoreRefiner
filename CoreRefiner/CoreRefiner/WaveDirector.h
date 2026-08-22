#pragma once

#include "WaveRules.h"

#include <functional>

/**
 * @brief 一局游戏的相位：战斗、收尾吸取、战备、失败、通关。
 */
enum class GamePhase
{
	Combat,
	Vacuum,
	Prep,
	Defeat,
	Victory
};

/**
 * @brief 回合导演：倒计时、切相、波次回调。不含渲染与敌人生成。
 */
class WaveDirector
{
public:
	WaveDirector() = default;

	WaveDirector(const WaveDirector&) = delete;
	WaveDirector& operator=(const WaveDirector&) = delete;

	/** @brief 回到战备、波次 0；不触发回调。 */
	void Reset() noexcept;

	/** @brief Combat 扣剩余时间，归零进入 Vacuum；Vacuum 由外部 FinishWave。 */
	void Update(float dt);

	[[nodiscard]] GamePhase GetPhase() const noexcept { return phase_; }
	/** @brief 当前或刚结束的波次（1-based）；Reset 后为 0。 */
	[[nodiscard]] int GetWaveIndex() const noexcept { return waveIndex_; }
	/** @brief 下一波编号；已通关时仍返回 kTotalWaves。 */
	[[nodiscard]] int GetNextWaveIndex() const noexcept;
	[[nodiscard]] float GetRemainSec() const noexcept { return remainSec_; }
	[[nodiscard]] const WaveSpec& GetCurrentSpec() const noexcept { return currentSpec_; }

	/** @brief Prep 相请求开下一波；Combat / Defeat / Victory 时忽略。 */
	void RequestStartWave();
	/** @brief 玩家死亡，进入 Defeat；不视为波次成功结束。 */
	void NotifyPlayerDead();
	/** @brief Vacuum 相结束本波，进入 Prep 或 Victory。 */
	void FinishWave();

	/**
	 * @brief 调试用强制切相：Combat → Vacuum，Vacuum → 结束本波，Prep → 开下一波。
	 */
	void DebugSkipPhase();

	std::function<void(const WaveSpec&)> onWaveStart;
	/** @brief 倒计时结束、进入吸取收尾（尚未切 Prep）。 */
	std::function<void(int)> onWaveExpire;
	std::function<void(int)> onWaveEnd;

private:
	void StartWave_(int wave);
	void ExpireWave_();
	void EndWave_();

	GamePhase phase_{ GamePhase::Prep };
	int waveIndex_{ 0 };
	float remainSec_{ 0.0f };
	WaveSpec currentSpec_{};
};
