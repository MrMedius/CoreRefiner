#pragma once

#include "Canvas2D.h"
#include "Graphics.h"
#include "RenderGraph.h"

#include <memory>
#include <string>

class ModuleField;

/**
 * @brief 战斗内 UI：迷你 Field + 顶居中波次/倒计时 + 左上角血条。不走 UiRoot。
 * @note 不拥有 Field；文本按整秒/波次变化才重绘；血条按 1% 量化后重绘。
 */
class UI_CombatHud
{
public:
	UI_CombatHud(Graphics& gfx, Rgph::RenderGraph& rg, ModuleField* field = nullptr);
	~UI_CombatHud() = default;

	UI_CombatHud(const UI_CombatHud&) = delete;
	UI_CombatHud& operator=(const UI_CombatHud&) = delete;

	void SetWave(int wave);
	void SetRemain(float remainSec);
	void SetHpRatio(float ratio);
	/** @brief 提交战斗 Field（底板 + 棋子）。 */
	void SubmitField();
	/** @brief 提交波次 / 倒计时 / 血条。 */
	void SubmitHud();
	void Submit();

private:
	void EnsureVisuals_();
	void PaintWave_();
	void PaintTimer_();
	void PaintHpBar_();
	void PaintText_(Canvas2D& canvas, const std::string& text, float fontSize, int& paintedKey, int key);
	void SyncTransforms_() noexcept;

	Graphics& gfx_;
	Rgph::RenderGraph& rg_;
	ModuleField* field_{ nullptr };

	std::unique_ptr<Canvas2D> waveText_;
	std::unique_ptr<Canvas2D> timerText_;
	std::unique_ptr<Canvas2D> hpBar_;

	int wave_{ 0 };
	float remainSec_{ 0.0f };
	float hpRatio_{ 1.0f };

	int paintedWave_{ -1 };
	int paintedRemainSec_{ -1 };
	int paintedHpKey_{ -1 };

	static constexpr unsigned kHpBarW_ = 240u;
	static constexpr unsigned kHpBarH_ = 16u;
	static constexpr float kWaveFontSize_ = 22.0f;
	static constexpr float kTimerFontSize_ = 28.0f;
};
