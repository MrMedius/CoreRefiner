#pragma once

#include "ButtonCanvasView.h"
#include "FocusManager.h"
#include "MouseUiInputAdapter.h"
#include "UiButton.h"

#include <memory>

namespace Rgph
{
	class RenderGraph;
}

class Graphics;

/**
 * @class UiStep6VerifyHarness
 * @brief 步骤 1–6 自检：语义输入、`FocusManager`、双 `UiButton`、`ButtonCanvasView`。
 *
 * @par 用法
 * - `Game` 构造函数末尾 `Init(gfx, uiRg)`。
 * - **`RunGame` 内在 `InputCodex::Update()` 之后** 调用 `TickAfterInput(titleActive)`。
 * - **`Draw` TITLE 分支在 `UIRG.Execute` 之前** 调用 `SubmitTitleUi(titleActive)`。
 */
class UiStep6VerifyHarness
{
public:
	UiStep6VerifyHarness() = default;
	~UiStep6VerifyHarness();

	UiStep6VerifyHarness(const UiStep6VerifyHarness&) = delete;
	UiStep6VerifyHarness& operator=(const UiStep6VerifyHarness&) = delete;

	void Init(Graphics& gfx, Rgph::RenderGraph& uiRg);
	void Shutdown();

	/** @brief TITLE 场景为 true；仅此时更新/提交自检 UI。 */
	void TickAfterInput(bool titleSceneActive);
	void SubmitTitleUi(bool titleSceneActive);

private:
	bool initialized_ = false;

	Ui::FocusManager focus_{};
	Ui::MouseUiInputAdapter mouseAdapter_{};
	std::unique_ptr<Ui::UiButton> btnA_{};
	std::unique_ptr<Ui::UiButton> btnB_{};
	std::unique_ptr<Ui::ButtonCanvasView> viewA_{};
	std::unique_ptr<Ui::ButtonCanvasView> viewB_{};

	unsigned clicksA_ = 0u;
	unsigned clicksB_ = 0u;
};
