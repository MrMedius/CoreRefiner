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

	// 和 UI_UltraRedraw::strokeSeen_ 的长度一致。边长改了这里会先报错。
	static_assert(kCells * kCells == 256u);
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
	EndStroke_();
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
	EndStroke_();
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
	if (!frame.pointer.primaryDown)
	{
		if (stroking_)
		{
			EndStroke_();
		}
		return;
	}

	const float pointerX = frame.pointer.logicalX;
	const float pointerY = frame.pointer.logicalY;
	const bool onBoard = frame.pointer.insideLogicalSurface && InsideBoard_(pointerX, pointerY);
	if (!stroking_)
	{
		// 只有这一下是在画布上按下的，才开始画。从按钮拖进来不算。
		if (!frame.pointer.primaryPressed || !onBoard)
		{
			return;
		}
		BeginStroke_();
	}
	else if (!onBoard)
	{
		hasStrokeCell_ = false;
		return;
	}

	unsigned cellX = 0u;
	unsigned cellY = 0u;
	if (!HitCell_(pointerX, pointerY, cellX, cellY))
	{
		return;
	}
	if (StrokeTo_(cellX, cellY))
	{
		Paint_();
	}
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

bool UI_UltraRedraw::InsideBoard_(const float logicalX, const float logicalY) const noexcept
{
	const float localX = logicalX - boardLeft_;
	const float localY = logicalY - boardTop_;
	const float boardPixels = static_cast<float>(BoardPixels());
	return localX >= 0.0f && localY >= 0.0f && localX < boardPixels && localY < boardPixels;
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

void UI_UltraRedraw::BeginStroke_() noexcept
{
	stroking_ = true;
	hasStrokeCell_ = false;
	for (std::uint8_t& seen : strokeSeen_)
	{
		seen = 0u;
	}
}

void UI_UltraRedraw::EndStroke_() noexcept
{
	stroking_ = false;
	hasStrokeCell_ = false;
}

bool UI_UltraRedraw::ApplyToggle_(const unsigned cellX, const unsigned cellY) noexcept
{
	if (cellX >= kCells || cellY >= kCells)
	{
		return false;
	}
	// index 是这一笔的记录下标。翻过的格子再经过就停住。
	const unsigned index = cellY * kCells + cellX;
	if (strokeSeen_[index] != 0u)
	{
		return false;
	}
	strokeSeen_[index] = 1u;
	ToggleCell_(cellX, cellY);
	return true;
}

bool UI_UltraRedraw::StrokeTo_(const unsigned cellX, const unsigned cellY) noexcept
{
	if (!hasStrokeCell_)
	{
		const bool flipped = ApplyToggle_(cellX, cellY);
		strokeX_ = cellX;
		strokeY_ = cellY;
		hasStrokeCell_ = true;
		return flipped;
	}

	int x0 = static_cast<int>(strokeX_);
	int y0 = static_cast<int>(strokeY_);
	const int x1 = static_cast<int>(cellX);
	const int y1 = static_cast<int>(cellY);
	const int dx = x1 > x0 ? x1 - x0 : x0 - x1;
	const int sx = x0 < x1 ? 1 : -1;
	const int dy = y1 > y0 ? y0 - y1 : y1 - y0;
	const int sy = y0 < y1 ? 1 : -1;
	int err = dx + dy;
	bool flipped = false;
	while (true)
	{
		flipped = ApplyToggle_(static_cast<unsigned>(x0), static_cast<unsigned>(y0)) || flipped;
		if (x0 == x1 && y0 == y1)
		{
			break;
		}
		const int e2 = err * 2;
		if (e2 >= dy)
		{
			err += dy;
			x0 += sx;
		}
		if (e2 <= dx)
		{
			err += dx;
			y0 += sy;
		}
	}
	strokeX_ = cellX;
	strokeY_ = cellY;
	return flipped;
}
