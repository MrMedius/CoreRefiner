#pragma once

#include "ButtonCanvasComponent.h"
#include "Canvas2D.h"
#include "Graphics.h"
#include "IconAtlas.h"
#include "MouseUiInputAdapter.h"
#include "UiRoot.h"

#include <functional>
#include <memory>

namespace Rgph
{
	class RenderGraph;
}

// 奥义重绘界面。遮罩、放大画布、确定、退出。画布不进焦点链。
class UI_UltraRedraw
{
public:
	UI_UltraRedraw(Graphics& gfx, Rgph::RenderGraph& rg);
	~UI_UltraRedraw() = default;

	UI_UltraRedraw(const UI_UltraRedraw&) = delete;
	UI_UltraRedraw& operator=(const UI_UltraRedraw&) = delete;

	// 确定时带上当前图案。界面自己不关闭。
	void SetOnConfirm(std::function<void(const IconAtlas::IconBits& bits)> cb);
	// 退出时不改图案。界面自己不关闭。
	void SetOnExit(std::function<void()> cb);

	// 灌入图案并重画格子。不打开界面。
	void SetBits(const IconAtlas::IconBits& bits);
	// 取出当前图案。亮的格是 1。
	[[nodiscard]] const IconAtlas::IconBits& GetBits() const noexcept;

	// 打开并预填图案。焦点放在确定上。
	void Show(const IconAtlas::IconBits& bits);
	// 关掉界面，图案留着。
	void Hide() noexcept;
	// 界面是否正打开。
	[[nodiscard]] bool IsOpen() const noexcept;

	// 只刷新确定和退出的文案，不改格子。
	void RefreshLabels();

	// 打开时接收画布上的按下和拖动，以及确定和退出。
	void Update(float dt);
	// 打开时才提交遮罩、画布和按钮。
	void Submit();

private:
	// 按 bits_ 把 16×16 画到放大的画布上。
	void Paint_();
	// 逻辑坐标是不是还在整块画布里。缝也算在里面，画布外不算。
	[[nodiscard]] bool InsideBoard_(float logicalX, float logicalY) const noexcept;
	// 逻辑坐标落在哪一格。点在缝上返回 false。
	[[nodiscard]] bool HitCell_(float logicalX, float logicalY, unsigned& cellX, unsigned& cellY) const noexcept;
	// 翻转这一格。x 从左到右，和 IconAtlas 的位序一致。
	void ToggleCell_(unsigned cellX, unsigned cellY) noexcept;
	// 按下落在画布上时开始一笔。清掉上一笔翻过的格子。
	void BeginStroke_() noexcept;
	// 松开或关掉界面时结束这一笔。
	void EndStroke_() noexcept;
	// 这一笔里这格还没翻过才翻转。翻了返回 true。
	[[nodiscard]] bool ApplyToggle_(unsigned cellX, unsigned cellY) noexcept;
	// 从上一格连到这一格，中间跳过的格子也翻。任一格翻了返回 true。
	[[nodiscard]] bool StrokeTo_(unsigned cellX, unsigned cellY) noexcept;

	// 全屏半透明遮罩。
	std::unique_ptr<Canvas2D> bg_;
	// 放大后的格子画布。一格占多像素，不是界面控件。
	std::unique_ptr<Canvas2D> board_;
	// 确定和退出的输入和焦点。画布不注册进来。
	std::unique_ptr<Ui::UiRoot> uiRoot_;
	// 确定按钮。点了把当前图案交给 onConfirm_。
	std::unique_ptr<Ui::ButtonCanvasComponent> btnConfirm_{};
	// 退出按钮。点了只调用 onExit_。
	std::unique_ptr<Ui::ButtonCanvasComponent> btnExit_{};
	// 读画布上的点击。不经过 UiRoot，所以上下左右不会在格子之间换焦点。
	Ui::MouseUiInputAdapter mouse_{};
	// 当前图案。一行一个 uint16，左起是高位。
	IconAtlas::IconBits bits_{};
	// 确定回调。参数是当前图案。
	std::function<void(const IconAtlas::IconBits&)> onConfirm_{};
	// 退出回调。
	std::function<void()> onExit_{};
	// 画布左上角的逻辑坐标。用来把点击换算成格子。
	float boardLeft_{ 0.0f };
	float boardTop_{ 0.0f };
	// 这一笔已经翻过的格子。下标是 y * 16 + x。同一笔再经过不再翻。
	std::uint8_t strokeSeen_[256]{};
	// 上一格的列。hasStrokeCell_ 为 true 时才用。
	unsigned strokeX_{ 0u };
	// 上一格的行。hasStrokeCell_ 为 true 时才用。
	unsigned strokeY_{ 0u };
	// 为 true 时才接收点击并提交。
	bool open_{ false };
	// 正在按住左键画。只有按下时落在画布上才变成 true。
	bool stroking_{ false };
	// 这一笔是否已经有上一格。离开画布后清掉，再进来不把线连过空白。
	bool hasStrokeCell_{ false };
};
