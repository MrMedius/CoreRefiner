#include "UI_UltraRedraw.h"

#include "Channels.h"
#include "Colors.h"
#include "RenderGraph.h"
#include "UiCopy.h"

namespace
{
	// 图标边长。和 IconBits 的 16 行一致。
	constexpr unsigned kCells = 16u;
	// 画布上一个图标像素占多少屏幕像素。
	constexpr unsigned kCellPx = 18u;
	// 格子之间的缝，用来看出 16×16。
	constexpr unsigned kGapPx = 2u;
	// 缝和没画的格子。
	constexpr Color kGapColor{ 24u, 24u, 28u, 255u };
	constexpr Color kOffColor{ 46u, 46u, 52u, 255u };
	// 画上的格子。游戏里会再变成彩虹，这里只表示开或关。
	constexpr Color kOnColor{ 255u, 255u, 255u, 255u };
	// 全屏遮罩。
	constexpr Color kDimColor{ 32u, 32u, 36u, 160u };
	// 确定、退出的焦点号。不和更名的 821 起冲突。
	constexpr Ui::FocusHandle kConfirmFocus{ 831u };
	constexpr Ui::FocusHandle kExitFocus{ 832u };

	// 整张放大画布的边长。
	[[nodiscard]] constexpr unsigned BoardPixels() noexcept
	{
		return kCells * kCellPx + (kCells - 1u) * kGapPx;
	}
}

UI_UltraRedraw::UI_UltraRedraw(Graphics& gfx, Rgph::RenderGraph& rg)
{
	const unsigned boardPixels = BoardPixels();
	const float centerX = static_cast<float>(SCREEN_WIDTH / 2u);
	const float centerY = static_cast<float>(SCREEN_HEIGHT / 2u);
	const unsigned btnWidth = SCREEN_WIDTH / 8u;
	const unsigned btnHeight = SCREEN_HEIGHT / 15u;
	// 画布略往上，给下面的确定和退出留位置。
	const float boardCenterY = centerY - static_cast<float>(btnHeight);
	boardLeft_ = centerX - static_cast<float>(boardPixels) * 0.5f;
	boardTop_ = boardCenterY - static_cast<float>(boardPixels) * 0.5f;
	const float buttonY = boardTop_ + static_cast<float>(boardPixels) + static_cast<float>(btnHeight);

	{
		bg_ = std::make_unique<Canvas2D>(gfx, SCREEN_WIDTH, SCREEN_HEIGHT);
		bg_->Clear(kDimColor);
		bg_->SetPosition(DirectX::XMFLOAT3{ centerX, centerY, 0.0f });
		bg_->SetScale(DirectX::XMFLOAT3{
			static_cast<float>(SCREEN_WIDTH),
			static_cast<float>(SCREEN_HEIGHT),
			1.0f
			});
		bg_->LinkTechniques(rg);
	}

	board_ = std::make_unique<Canvas2D>(gfx, boardPixels, boardPixels);
	board_->SetPosition(DirectX::XMFLOAT3{ centerX, boardCenterY, 0.0f });
	board_->SetScale(DirectX::XMFLOAT3{
		static_cast<float>(boardPixels),
		static_cast<float>(boardPixels),
		1.0f
		});
	board_->LinkTechniques(rg);
	Paint_();

	Ui::ButtonCanvasStyle buttonStyle{};
	buttonStyle.primaryFont = Text::FontSource::System(L"Microsoft YaHei UI");

	uiRoot_ = std::make_unique<Ui::UiRoot>();
	uiRoot_->Clear();

	{
		btnConfirm_ = std::make_unique<Ui::ButtonCanvasComponent>(
			gfx, kConfirmFocus,
			centerX - static_cast<float>(btnWidth) * 0.7f,
			buttonY,
			static_cast<float>(btnWidth),
			static_cast<float>(btnHeight),
			buttonStyle);
		btnConfirm_->Button().SetOnClick([this] {
			if (!onConfirm_)
			{
				return;
			}
			onConfirm_(bits_);
			});
		btnConfirm_->RegisterTo(*uiRoot_);
	}

	{
		btnExit_ = std::make_unique<Ui::ButtonCanvasComponent>(
			gfx, kExitFocus,
			centerX + static_cast<float>(btnWidth) * 0.7f,
			buttonY,
			static_cast<float>(btnWidth),
			static_cast<float>(btnHeight),
			buttonStyle);
		btnExit_->Button().SetOnClick([this] {
			if (onExit_)
			{
				onExit_();
			}
			});
		btnExit_->RegisterTo(*uiRoot_);
	}

	uiRoot_->RebuildTabOrder();
	uiRoot_->InitLinkTechniques(rg);
	RefreshLabels();
}

void UI_UltraRedraw::SetOnConfirm(std::function<void(const IconAtlas::IconBits& bits)> cb)
{
	onConfirm_ = std::move(cb);
}

void UI_UltraRedraw::SetOnExit(std::function<void()> cb)
{
	onExit_ = std::move(cb);
}

void UI_UltraRedraw::SetBits(const IconAtlas::IconBits& bits)
{
	bits_ = bits;
	Paint_();
}

const IconAtlas::IconBits& UI_UltraRedraw::GetBits() const noexcept
{
	return bits_;
}

void UI_UltraRedraw::Show(const IconAtlas::IconBits& bits)
{
	SetBits(bits);
	open_ = true;
	if (uiRoot_ != nullptr)
	{
		//uiRoot_->Focus().RequestFocus(kConfirmFocus);
		uiRoot_->Focus().ClearFocus();
	}
	RefreshLabels();
}

void UI_UltraRedraw::Hide() noexcept
{
	open_ = false;
}

bool UI_UltraRedraw::IsOpen() const noexcept
{
	return open_;
}

void UI_UltraRedraw::RefreshLabels()
{
	if (btnConfirm_ != nullptr)
	{
		btnConfirm_->Button().SetLabel(GetUiCopy("ui.confirm"));
	}
	if (btnExit_ != nullptr)
	{
		btnExit_->Button().SetLabel(GetUiCopy("ui.exit"));
	}
}

void UI_UltraRedraw::Update(float dt)
{
	(void)dt;
	if (!open_ || uiRoot_ == nullptr || board_ == nullptr)
	{
		return;
	}

	uiRoot_->UpdateAfterInput();

	const Ui::UiInputFrame frame = mouse_.BuildFrame();
	if (!frame.pointer.primaryPressed || !frame.pointer.insideLogicalSurface)
	{
		return;
	}

	unsigned cellX = 0u;
	unsigned cellY = 0u;
	if (!HitCell_(frame.pointer.logicalX, frame.pointer.logicalY, cellX, cellY))
	{
		return;
	}
	ToggleCell_(cellX, cellY);
	Paint_();
}

void UI_UltraRedraw::Submit()
{
	if (!open_)
	{
		return;
	}
	if (bg_ != nullptr)
	{
		bg_->Submit(Chan::ui);
	}
	if (board_ != nullptr)
	{
		board_->Submit(Chan::ui);
	}
	if (uiRoot_ != nullptr)
	{
		uiRoot_->Submit(Chan::ui);
	}
}

void UI_UltraRedraw::Paint_()
{
	if (board_ == nullptr)
	{
		return;
	}

	Canvas& canvas = *board_;
	canvas.Clear(kGapColor);
	const unsigned pitch = kCellPx + kGapPx;
	for (unsigned y = 0u; y < kCells; ++y)
	{
		const std::uint16_t row = bits_[y];
		const unsigned top = y * pitch;
		for (unsigned x = 0u; x < kCells; ++x)
		{
			const bool on = (row & static_cast<std::uint16_t>(1u << (15u - x))) != 0u;
			const Color color = on ? kOnColor : kOffColor;
			const unsigned left = x * pitch;
			for (unsigned py = 0u; py < kCellPx; ++py)
			{
				for (unsigned px = 0u; px < kCellPx; ++px)
				{
					canvas.PutPixel(left + px, top + py, color);
				}
			}
		}
	}
	canvas.NotifyPixelsChanged();
}

bool UI_UltraRedraw::HitCell_(
	const float logicalX,
	const float logicalY,
	unsigned& cellX,
	unsigned& cellY) const noexcept
{
	const float localX = logicalX - boardLeft_;
	const float localY = logicalY - boardTop_;
	if (localX < 0.0f || localY < 0.0f)
	{
		return false;
	}

	const unsigned pitch = kCellPx + kGapPx;
	const unsigned ix = static_cast<unsigned>(localX);
	const unsigned iy = static_cast<unsigned>(localY);
	const unsigned x = ix / pitch;
	const unsigned y = iy / pitch;
	if (x >= kCells || y >= kCells)
	{
		return false;
	}
	// 落在格子之间的缝上不算点中。
	if ((ix % pitch) >= kCellPx || (iy % pitch) >= kCellPx)
	{
		return false;
	}
	cellX = x;
	cellY = y;
	return true;
}

void UI_UltraRedraw::ToggleCell_(const unsigned cellX, const unsigned cellY) noexcept
{
	if (cellX >= kCells || cellY >= kCells)
	{
		return;
	}
	const std::uint16_t mask = static_cast<std::uint16_t>(1u << (15u - cellX));
	bits_[cellY] = static_cast<std::uint16_t>(bits_[cellY] ^ mask);
}
