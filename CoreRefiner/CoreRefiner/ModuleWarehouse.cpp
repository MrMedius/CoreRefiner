#include "ModuleWarehouse.h"
#include "Channels.h"
#include "Colors.h"
#include "RenderGraph.h"

#include <algorithm>
#include <cmath>

namespace
{
	void PutPixelClamped(Canvas2D& canvas, int x, int y, Color c)
	{
		const int w = static_cast<int>(canvas.GetCanvasWidth());
		const int h = static_cast<int>(canvas.GetCanvasHeight());
		if (x < 0 || y < 0 || x >= w || y >= h)
		{
			return;
		}
		canvas.PutPixel(static_cast<unsigned>(x), static_cast<unsigned>(y), c);
	}

	void DrawHLine(Canvas2D& canvas, int x0, int x1, int y, Color c)
	{
		if (x1 < x0)
		{
			std::swap(x0, x1);
		}
		for (int x = x0; x <= x1; ++x)
		{
			PutPixelClamped(canvas, x, y, c);
		}
	}

	void DrawVLine(Canvas2D& canvas, int x, int y0, int y1, Color c)
	{
		if (y1 < y0)
		{
			std::swap(y0, y1);
		}
		for (int y = y0; y <= y1; ++y)
		{
			PutPixelClamped(canvas, x, y, c);
		}
	}

	void DrawRectOutline(Canvas2D& canvas, int x0, int y0, int x1, int y1, Color c)
	{
		DrawHLine(canvas, x0, x1, y0, c);
		DrawHLine(canvas, x0, x1, y1, c);
		DrawVLine(canvas, x0, y0, y1, c);
		DrawVLine(canvas, x1, y0, y1, c);
	}
}

float ModuleWarehouse::HalfSpanX_() noexcept
{
	return (static_cast<float>(kColumns - 1) * 0.5f) * kSlotPitch;
}

float ModuleWarehouse::HalfSpanY_() noexcept
{
	return (static_cast<float>(kMaxRows - 1) * 0.5f) * kSlotPitch;
}

DirectX::XMFLOAT2 ModuleWarehouse::SlotLocalPos_(std::size_t index) noexcept
{
	const int col = static_cast<int>(index % static_cast<std::size_t>(kColumns));
	const int row = static_cast<int>(index / static_cast<std::size_t>(kColumns));
	const float x = (static_cast<float>(col) - (static_cast<float>(kColumns - 1) * 0.5f)) * kSlotPitch;
	const float y = static_cast<float>(row) * kSlotPitch;
	return DirectX::XMFLOAT2{ x, y };
}

ModuleWarehouse::BoundsWorld ModuleWarehouse::GetBoundsWorld() const noexcept
{
	// Slot centers span: x in [-(cols-1)/2, +(cols-1)/2]*pitch, y in [0, (rows-1)*pitch].
	const float halfSpanX = HalfSpanX_();
	const float halfSpanY = HalfSpanY_();
	const float localCenterY = halfSpanY;

	BoundsWorld b{};
	b.center = DirectX::XMFLOAT2{
		warehouseOrigin_.x,
		warehouseOrigin_.y + localCenterY
	};
	b.half = DirectX::XMFLOAT2{
		halfSpanX + kBoundsPad,
		halfSpanY + kBoundsPad
	};
	return b;
}

bool ModuleWarehouse::ContainsCircle(DirectX::XMFLOAT2 worldCenter, float radius) const noexcept
{
	const BoundsWorld b = GetBoundsWorld();
	const float r = (std::max)(radius, 0.0f);
	const float minX = b.center.x - b.half.x + r;
	const float maxX = b.center.x + b.half.x - r;
	const float minY = b.center.y - b.half.y + r;
	const float maxY = b.center.y + b.half.y - r;
	if (minX > maxX || minY > maxY)
	{
		return false;
	}
	return worldCenter.x >= minX && worldCenter.x <= maxX
		&& worldCenter.y >= minY && worldCenter.y <= maxY;
}

void ModuleWarehouse::SetOrigin(DirectX::XMFLOAT3 origin) noexcept
{
	warehouseOrigin_ = origin;
	RelayoutSlots();
	SyncPanelTransform_();
}

void ModuleWarehouse::RelayoutSlots()
{
	for (std::size_t i = 0; i < nodes_.size(); ++i)
	{
		IFieldNode* node = nodes_[i].get();
		if (node == nullptr)
		{
			continue;
		}
		node->SetLocalPos(SlotLocalPos_(i));
		node->SetFieldOrigin(warehouseOrigin_);
		node->SyncVisual();
	}
}

IFieldNode* ModuleWarehouse::TryAdopt(std::unique_ptr<IFieldNode>& node)
{
	if (node == nullptr || node->IsCore() || IsFull())
	{
		return nullptr;
	}
	IFieldNode* raw = node.get();
	nodes_.push_back(std::move(node));
	RelayoutSlots();
	return raw;
}

std::unique_ptr<IFieldNode> ModuleWarehouse::TakeNode(std::size_t index)
{
	if (index >= nodes_.size())
	{
		return nullptr;
	}
	std::unique_ptr<IFieldNode> out = std::move(nodes_[index]);
	nodes_.erase(nodes_.begin() + static_cast<std::ptrdiff_t>(index));
	RelayoutSlots();
	return out;
}

std::unique_ptr<IFieldNode> ModuleWarehouse::TakeNode(IFieldNode* node)
{
	if (node == nullptr)
	{
		return nullptr;
	}
	for (std::size_t i = 0; i < nodes_.size(); ++i)
	{
		if (nodes_[i].get() == node)
		{
			return TakeNode(i);
		}
	}
	return nullptr;
}

void ModuleWarehouse::EnsurePanelVisual_(Graphics& gfx, Rgph::RenderGraph& rg)
{
	if (panel_ != nullptr)
	{
		return;
	}

	const float halfX = HalfSpanX_() + kBoundsPad;
	const float halfY = HalfSpanY_() + kBoundsPad;
	const unsigned w = static_cast<unsigned>(std::lround(halfX * 2.0f));
	const unsigned h = static_cast<unsigned>(std::lround(halfY * 2.0f));

	panel_ = std::make_unique<Canvas2D>(gfx, w, h);
	PaintPanel_();
	panel_->LinkTechniques(rg);
	SyncPanelTransform_();
}

void ModuleWarehouse::PaintPanel_()
{
	if (panel_ == nullptr)
	{
		return;
	}

	constexpr Color kBg{ 36u, 48u, 68u, 150u };
	constexpr Color kGrid{ 170u, 190u, 210u, 110u };

	panel_->Clear(kBg);

	const float halfX = HalfSpanX_() + kBoundsPad;
	const float halfY = HalfSpanY_() + kBoundsPad;
	const float localCenterY = HalfSpanY_();
	const float halfPitch = kSlotPitch * 0.5f;

	for (std::size_t i = 0; i < kMaxSlots; ++i)
	{
		const DirectX::XMFLOAT2 slot = SlotLocalPos_(i);
		// Warehouse-local → panel pixel (bounds center is local (0, localCenterY)).
		const float cx = slot.x + halfX;
		const float cy = slot.y - localCenterY + halfY;
		const int x0 = static_cast<int>(std::lround(cx - halfPitch));
		const int y0 = static_cast<int>(std::lround(cy - halfPitch));
		const int x1 = static_cast<int>(std::lround(cx + halfPitch));
		const int y1 = static_cast<int>(std::lround(cy + halfPitch));
		DrawRectOutline(*panel_, x0, y0, x1, y1, kGrid);
	}

	panel_->NotifyPixelsChanged();
}

void ModuleWarehouse::SyncPanelTransform_() noexcept
{
	if (panel_ == nullptr)
	{
		return;
	}

	const BoundsWorld b = GetBoundsWorld();
	panel_->SetPosition(DirectX::XMFLOAT3{ b.center.x, b.center.y, 0.0f });
	panel_->SetScale(DirectX::XMFLOAT3{
		b.half.x * 2.0f,
		b.half.y * 2.0f,
		1.0f
	});
}

void ModuleWarehouse::InitAllVisuals(Graphics& gfx, Rgph::RenderGraph& rg, DirectX::XMFLOAT3 origin)
{
	warehouseOrigin_ = origin;
	EnsurePanelVisual_(gfx, rg);
	for (auto& n : nodes_)
	{
		if (n != nullptr)
		{
			n->InitVisual(gfx, rg, warehouseOrigin_);
		}
	}
	RelayoutSlots();
	SyncPanelTransform_();
}

void ModuleWarehouse::SyncAllVisuals()
{
	SyncPanelTransform_();
	for (auto& n : nodes_)
	{
		if (n != nullptr)
		{
			n->SyncVisual();
		}
	}
}

void ModuleWarehouse::SubmitBackground()
{
	if (panel_ != nullptr)
	{
		panel_->Submit(Chan::ui);
	}
}

void ModuleWarehouse::SubmitNodes()
{
	for (auto& n : nodes_)
	{
		if (n != nullptr)
		{
			n->SubmitVisual();
		}
	}
}

void ModuleWarehouse::SubmitAllVisuals()
{
	SubmitBackground();
	SubmitNodes();
}
