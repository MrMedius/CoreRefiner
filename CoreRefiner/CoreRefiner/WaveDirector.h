#pragma once

#include "WaveRules.h"

#include <functional>

/**
 * @brief 一局游戏的相位：战斗、战备、失败、通关。
 */
enum class GamePhase
{
	Combat,
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

	/** @brief 仅 Combat 相扣剩余时间；归零则结束本波。 */
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

	/**
	 * @brief 调试用强制切相：Combat → 结束本波，Prep → 开下一波。
	 */
	void DebugSkipPhase();

	std::function<void(const WaveSpec&)> onWaveStart;
	std::function<void(int)> onWaveEnd;

private:
	void StartWave_(int wave);
	void EndWave_();

	GamePhase phase_{ GamePhase::Prep };
	int waveIndex_{ 0 };
	float remainSec_{ 0.0f };
	WaveSpec currentSpec_{};
};
