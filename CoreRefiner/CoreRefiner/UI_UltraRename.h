#pragma once

#include "ButtonCanvasComponent.h"
#include "Canvas2D.h"
#include "Graphics.h"
#include "TextFieldCanvasComponent.h"
#include "UiRoot.h"

#include <functional>
#include <memory>
#include <string>

namespace Rgph
{
	class RenderGraph;
}

class UI_UltraRename
{
public:
	UI_UltraRename(Graphics& gfx, Rgph::RenderGraph& rg);
	~UI_UltraRename() = default;

	UI_UltraRename(const UI_UltraRename&) = delete;
	UI_UltraRename& operator=(const UI_UltraRename&) = delete;

	// 确定时带上输入栏里的名字，允许为空。界面自己不关闭。
	void SetOnConfirm(std::function<void(std::string name)> cb);
	// 退出时不改名字。界面自己不关闭。
	void SetOnExit(std::function<void()> cb);

	// 打开并预填当前名字，光标放在名字末尾，焦点放在输入栏。
	void Show(std::string currentName);
	// 关掉界面，不清输入栏。
	void Hide() noexcept;
	// 界面是否正打开。
	[[nodiscard]] bool IsOpen() const noexcept;

	// 只刷新确定和退出的文案，不改输入栏里的名字。
	void RefreshLabels();

	// 打开时才接收点击和输入。
	void Update(float dt);
	// 打开时才提交遮罩和控件。
	void Submit();

private:
	// 全屏半透明遮罩。
	std::unique_ptr<Canvas2D> bg_;
	// 输入栏、确定、退出的输入和焦点。
	std::unique_ptr<Ui::UiRoot> uiRoot_;
	// 名字输入栏。上限是奥义名字的码点数。
	std::unique_ptr<Ui::TextFieldCanvasComponent> field_{};
	// 确定按钮。点了把当前文字交给 onConfirm_。
	std::unique_ptr<Ui::ButtonCanvasComponent> btnConfirm_{};
	// 退出按钮。点了只调用 onExit_。
	std::unique_ptr<Ui::ButtonCanvasComponent> btnExit_{};

	// 确定回调。参数是输入栏里的名字。
	std::function<void(std::string)> onConfirm_{};
	// 退出回调。
	std::function<void()> onExit_{};

	// 为 true 时 Update 和 Submit 才工作。
	bool open_{ false };
};
