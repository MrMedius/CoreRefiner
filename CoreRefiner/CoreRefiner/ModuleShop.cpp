#include "ModuleShop.h"
#include "Channels.h"
#include "Collision2D.h"
#include "Colors.h"
#include "ModuleNodeFactory.h"
#include "ModuleNodeLabel.h"
#include "ModuleNodePrice.h"
#include "RenderGraph.h"

#include <algorithm>
#include <cmath>
#include <utility>

static_assert(ModuleShop::kSlotCount <= ModuleNodeLabelCount());

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

float ModuleShop::HalfSpanX_() noexcept
{
	return (static_cast<float>(kColumns - 1) * 0.5f) * kSlotPitch;
}

float ModuleShop::HalfSpanY_() noexcept
{
	return (static_cast<float>(kRows - 1) * 0.5f) * kSlotPitch;
}

DirectX::XMFLOAT2 ModuleShop::SlotLocalPos_(std::size_t index) noexcept
{
	const int col = static_cast<int>(index % static_cast<std::size_t>(kColumns));
	const int row = static_cast<int>(index / static_cast<std::size_t>(kColumns));
	const float x = (static_cast<float>(col) - (static_cast<float>(kColumns - 1) * 0.5f)) * kSlotPitch;
	const float y = static_cast<float>(row) * kSlotPitch;
	return DirectX::XMFLOAT2{ x, y };
}

std::size_t ModuleShop::FindSlotIndex_(const IModuleNode* node) const noexcept
{
	if (node == nullptr)
	{
		return kSlotCount;
	}
	for (std::size_t i = 0; i < kSlotCount; ++i)
	{
		if (slots_[i].node.get() == node)
		{
			return i;
		}
	}
	return kSlotCount;
}

ModuleShop::BoundsWorld ModuleShop::GetBoundsWorld() const noexcept
{
	const float halfSpanX = HalfSpanX_();
	const float halfSpanY = HalfSpanY_();
	const float localCenterY = halfSpanY;

	BoundsWorld b{};
	b.center = DirectX::XMFLOAT2{
		origin_.x,
		origin_.y + localCenterY
	};
	b.half = DirectX::XMFLOAT2{
		halfSpanX + kBoundsPad,
		halfSpanY + kBoundsPad
	};
	return b;
}

bool ModuleShop::ContainsCircle(DirectX::XMFLOAT2 worldCenter, float radius) const noexcept
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

void ModuleShop::SetOrigin(DirectX::XMFLOAT3 origin) noexcept
{
	origin_ = origin;
	RelayoutSlots_();
	SyncPanelTransform_();
}

void ModuleShop::FillStock()
{
	for (std::size_t i = 0; i < kSlotCount; ++i)
	{
		const ModuleNodeLabel label = static_cast<ModuleNodeLabel>(i);
		slots_[i].node = ModuleNodeFactory::MakeModuleNode(label, SlotLocalPos_(i));
		slots_[i].sold = false;
		slots_[i].price = (slots_[i].node != nullptr)
			? ModuleNodePrice::GetBuyPrice(label)
			: 0;
	}
	RelayoutSlots_();
}

void ModuleShop::MarkSold(std::size_t index)
{
	if (index >= kSlotCount)
	{
		return;
	}
	slots_[index].sold = true;
	slots_[index].node.reset();
}

void ModuleShop::RelayoutSlots_()
{
	for (std::size_t i = 0; i < kSlotCount; ++i)
	{
		IModuleNode* node = slots_[i].node.get();
		if (node == nullptr)
		{
			continue;
		}
		node->SetLocalPos(SlotLocalPos_(i));
		node->SetZoneOrigin(origin_);
		node->SyncVisual();
	}
}

bool ModuleShop::TryAcceptDrop(std::unique_ptr<IModuleNode>& node, DirectX::XMFLOAT2 localPos)
{
	(void)node;
	(void)localPos;
	// Sell adopt lands in a later step; inbound drops are refused for now.
	return false;
}

DropResult ModuleShop::EvalDrop(const IModuleNode& node, DirectX::XMFLOAT2 worldPos, ZoneId from) const noexcept
{
	DropResult result{};
	if (!ContainsCircle(worldPos, node.GetHitRadius()))
	{
		result.verdict = DropVerdict::OutOfBounds;
		return result;
	}

	if (from == ZoneId::Shop)
	{
		result.verdict = DropVerdict::Accept;
		const std::size_t i = FindSlotIndex_(&node);
		if (i < kSlotCount)
		{
			result.localPos = SlotLocalPos_(i);
		}
		return result;
	}

	result.verdict = DropVerdict::Forbidden;
	return result;
}

std::unique_ptr<IModuleNode> ModuleShop::TakeNode(IModuleNode* node)
{
	const std::size_t i = FindSlotIndex_(node);
	if (i >= kSlotCount)
	{
		return nullptr;
	}
	return std::move(slots_[i].node);
}

void ModuleShop::EnsurePanelVisual_(Graphics& gfx, Rgph::RenderGraph& rg)
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

void ModuleShop::PaintPanel_()
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

	for (std::size_t i = 0; i < kSlotCount; ++i)
	{
		const DirectX::XMFLOAT2 slot = SlotLocalPos_(i);
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

void ModuleShop::SyncPanelTransform_() noexcept
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

void ModuleShop::InitAllVisuals(Graphics& gfx, Rgph::RenderGraph& rg, DirectX::XMFLOAT3 origin)
{
	origin_ = origin;
	FillStock();
	EnsurePanelVisual_(gfx, rg);
	for (Slot& slot : slots_)
	{
		if (slot.node != nullptr)
		{
			slot.node->InitVisual(gfx, rg, origin_);
		}
	}
	RelayoutSlots_();
	SyncPanelTransform_();
}

void ModuleShop::SyncAllVisuals()
{
	SyncPanelTransform_();
	for (Slot& slot : slots_)
	{
		if (slot.node != nullptr)
		{
			slot.node->SyncVisual();
		}
	}
}

void ModuleShop::SubmitBackground()
{
	if (panel_ != nullptr)
	{
		panel_->Submit(Chan::ui);
	}
}

void ModuleShop::SubmitNodes()
{
	for (Slot& slot : slots_)
	{
		if (slot.node != nullptr)
		{
			slot.node->SubmitVisual();
		}
	}
}

void ModuleShop::SubmitAllVisuals()
{
	SubmitBackground();
	SubmitNodes();
}

IModuleNode* ModuleShop::PickAt(DirectX::XMFLOAT2 worldPos, float& outDistSq) noexcept
{
	IModuleNode* best = nullptr;
	float bestDistSq = 1.0e9f;

	for (Slot& slot : slots_)
	{
		IModuleNode* node = slot.node.get();
		if (node == nullptr)
		{
			continue;
		}
		const DirectX::XMFLOAT2 local = node->GetLocalPos();
		const DirectX::XMFLOAT2 world{
			origin_.x + local.x,
			origin_.y + local.y
		};
		const Collider2D::CircleCollider hit{ world, node->GetHitRadius() };
		const Collider2D::PointCollider pt{ worldPos };
		if (!Collider2D::CollisionSystem::IsOverlap(hit, pt))
		{
			continue;
		}
		const float dx = worldPos.x - world.x;
		const float dy = worldPos.y - world.y;
		const float distSq = dx * dx + dy * dy;
		if (distSq < bestDistSq)
		{
			bestDistSq = distSq;
			best = node;
		}
	}

	if (best != nullptr)
	{
		outDistSq = bestDistSq;
	}
	return best;
}
