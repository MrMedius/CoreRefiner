#include "ZoneLayoutEditor.h"
#include "FieldNodeInfoCopy.h"
#include "InputCodex.h"
#include "Colors.h"
#include "Channels.h"
#include "Graphics.h"
#include "Window.h"

#include <cmath>
#include <utility>

namespace
{
	void EnsureFieldNodeInfoCopyLoaded_()
	{
		if (IsFieldNodeInfoCopyLoaded())
		{
			return;
		}
		if (LoadFieldNodeInfoCopy("FieldNodeInfoCopy.json"))
		{
			return;
		}
		if (LoadFieldNodeInfoCopy("CoreRefiner/FieldNodeInfoCopy.json"))
		{
			return;
		}
		(void)LoadFieldNodeInfoCopy("CoreRefiner/CoreRefiner/FieldNodeInfoCopy.json");
	}

	[[nodiscard]] bool IsBoundZoneId_(ZoneId id) noexcept
	{
		return ToIndex(id) < ZoneCount();
	}
}

void ZoneLayoutEditor::Begin(std::array<IModuleZone*, ZoneCount()> zones, std::array<DirectX::XMFLOAT3, ZoneCount()> origins, Graphics& gfx, Rgph::RenderGraph& rg, DirectX::XMFLOAT3 combatFieldOrigin)
{
	combatOrigin_ = combatFieldOrigin;
	zones_ = zones;

	EnsureRingVisual_(gfx, rg);
	EnsureFieldNodeInfoCopyLoaded_();
	infoPanel_.Ensure(gfx, rg);
	infoPanel_.Hide();

	hover_ = nullptr;
	dragged_ = nullptr;
	hoverSource_ = kNoZone_;
	dragSource_ = kNoZone_;
	active_ = true;
	dragStartLocalPos_ = {};
	dragOrigin_ = {};
	ringKind_ = RingKind_::Hover;

	for (std::size_t i = 0; i < ZoneCount(); ++i)
	{
		IModuleZone* zone = zones_[i];
		if (zone == nullptr)
		{
			continue;
		}
		zone->SetOrigin(origins[i]);
		zone->SyncAllVisuals();
	}

	Snapshot_();
}

void ZoneLayoutEditor::End()
{
	if (!active_)
	{
		return;
	}

	// Keep the current arrangement. Do NOT apply layoutSnapshot_
	// (Esc CancelRestore is the only path that restores Begin poses).
	ClearActiveDrag_();
	ClearAllLayoutGhosts_();
	infoPanel_.Hide();

	hover_ = nullptr;
	hoverSource_ = kNoZone_;
	ringKind_ = RingKind_::Hover;
	dragStartLocalPos_ = {};

	if (IModuleZone* field = ZoneAt_(ZoneId::Field))
	{
		field->SetOrigin(combatOrigin_);
		field->SyncAllVisuals();
	}

	zones_.fill(nullptr);
	layoutSnapshot_.clear();
	active_ = false;
}

void ZoneLayoutEditor::CancelRestore()
{
	if (!active_)
	{
		return;
	}

	ClearActiveDrag_();
	ClearAllLayoutGhosts_();
	infoPanel_.Hide();

	hover_ = nullptr;
	hoverSource_ = kNoZone_;
	ringKind_ = RingKind_::Hover;
	dragStartLocalPos_ = {};

	struct PendingMigrate_
	{
		std::unique_ptr<IFieldNode> node;
		ZoneId target{ ZoneId::Field };
	};
	std::vector<PendingMigrate_> pending;
	pending.reserve(layoutSnapshot_.size());

	// Pass 1: pull every zone-mismatched node out so capacity is free before re-adopt.
	for (const LayoutSnapshotEntry_& entry : layoutSnapshot_)
	{
		ZoneId curZone = kNoZone_;
		IFieldNode* node = FindNodeById_(entry.id, curZone);
		if (node == nullptr || curZone == entry.zone)
		{
			continue;
		}

		IModuleZone* source = ZoneAt_(curZone);
		if (source == nullptr)
		{
			continue;
		}
		std::unique_ptr<IFieldNode> taken = source->TakeNode(node);
		if (taken == nullptr)
		{
			continue;
		}
		pending.push_back(PendingMigrate_{ std::move(taken), entry.zone });
	}

	auto adoptOne_ = [&](PendingMigrate_& p) -> IFieldNode*
	{
		if (p.node == nullptr)
		{
			return nullptr;
		}
		IFieldNode* raw = p.node.get();
		const DirectX::XMFLOAT2 local = raw->GetLocalPos();
		if (IModuleZone* target = ZoneAt_(p.target))
		{
			if (target->TryAcceptDrop(p.node, local))
			{
				return raw;
			}
		}
		// Full / Core / missing target: keep ownership on Field so the node is not dropped.
		if (IModuleZone* field = ZoneAt_(ZoneId::Field))
		{
			const DirectX::XMFLOAT2 fallbackLocal =
				(raw != nullptr) ? raw->GetLocalPos() : DirectX::XMFLOAT2{};
			if (!field->TryAcceptDrop(p.node, fallbackLocal))
			{
				return nullptr;
			}
			return raw;
		}
		return nullptr;
	};

	// Pass 2: re-adopt in ZoneId order so Field is filled first (frees warehouse slots).
	for (std::size_t zi = 0; zi < ZoneCount(); ++zi)
	{
		const ZoneId targetId = static_cast<ZoneId>(zi);
		for (PendingMigrate_& p : pending)
		{
			if (p.target == targetId)
			{
				(void)adoptOne_(p);
			}
		}
	}

	// Pass 3: restore Begin localPos/origin (also repairs Take/TryAcceptDrop RelayoutSlots side effects).
	for (const LayoutSnapshotEntry_& entry : layoutSnapshot_)
	{
		ZoneId curZone = kNoZone_;
		IFieldNode* node = FindNodeById_(entry.id, curZone);
		if (node == nullptr || curZone != entry.zone)
		{
			continue;
		}

		node->SetLocalPos(entry.localPos);
		if (IModuleZone* zone = ZoneAt_(entry.zone))
		{
			node->SetFieldOrigin(zone->GetOrigin());
		}
		if (node->IsLayoutGhostActive())
		{
			node->EndLayoutGhost();
		}
		node->SyncVisual();
	}

	for (IModuleZone* zone : zones_)
	{
		if (zone != nullptr)
		{
			zone->SyncAllVisuals();
		}
	}
}

void ZoneLayoutEditor::Update(float dt, Window* hostWindow)
{
	(void)dt;
	if (!active_)
	{
		return;
	}

	auto& input = InputCodex::Get();

	if (input.KeyTriggered(KK_ESCAPE))
	{
		CancelRestore();
		return;
	}

	const DirectX::XMFLOAT2 mouseGame = MouseGame_();

	if (dragged_ != nullptr)
	{
		if (input.MouseLeftPressed())
		{
			SetFreePreview_(*dragged_, mouseGame);
			hover_ = dragged_;
			hoverSource_ = dragSource_;
		}
		if (input.MouseLeftReleased())
		{
			SetFreePreview_(*dragged_, mouseGame);
			ResolveRelease_();
			dragged_ = nullptr;
			dragSource_ = kNoZone_;
			dragOrigin_ = {};
			hover_ = PickHover_(mouseGame, hoverSource_);
		}
	}
	else
	{
		hover_ = PickHover_(mouseGame, hoverSource_);
		if (input.MouseLeftTriggered() && hover_ != nullptr)
		{
			dragged_ = hover_;
			dragSource_ = hoverSource_;
			dragOrigin_ = OriginForSource_(dragSource_);
			dragStartLocalPos_ = dragged_->GetLocalPos();
			dragged_->BeginLayoutGhost(dragStartLocalPos_);
			if (hostWindow != nullptr)
			{
				SnapCursorToNode_(*dragged_, dragOrigin_, *hostWindow);
			}
			SetFreePreview_(*dragged_, DirectX::XMFLOAT2{
				dragOrigin_.x + dragged_->GetLocalPos().x,
				dragOrigin_.y + dragged_->GetLocalPos().y
			});
		}
	}

	IFieldNode* ringTarget = (dragged_ != nullptr) ? dragged_ : hover_;
	const ZoneId ringSource = (dragged_ != nullptr) ? dragSource_ : hoverSource_;
	if (dragged_ != nullptr)
	{
		const DropEval_ eval = EvalDrop_(*dragged_);
		ringKind_ = eval.placeable ? RingKind_::Valid : RingKind_::Overlap;
	}
	else
	{
		ringKind_ = RingKind_::Hover;
	}
	if (ringTarget != nullptr)
	{
		SyncRingTransform_(*ringTarget, OriginForSource_(ringSource));
	}

	if (dragged_ != nullptr || hover_ == nullptr)
	{
		infoPanel_.Hide();
	}
	else
	{
		const DirectX::XMFLOAT3 origin = OriginForSource_(hoverSource_);
		const DirectX::XMFLOAT2 local = hover_->GetLocalPos();
		infoPanel_.ShowFor(
			hover_->GetAttackNodeLabel(),
			DirectX::XMFLOAT2{ origin.x + local.x, origin.y + local.y });
	}
}

void ZoneLayoutEditor::SubmitOverlay()
{
	if (!active_)
	{
		return;
	}
	if (dragged_ != nullptr || hover_ != nullptr)
	{
		if (Canvas2D* ring = ActiveRing_())
		{
			ring->Submit(Chan::ui);
		}
	}
	infoPanel_.Submit();
}

std::unique_ptr<Canvas2D> ZoneLayoutEditor::MakeRingCanvas_(Graphics& gfx, Rgph::RenderGraph& rg, Color ringColor)
{
	constexpr unsigned kSize = 64u;
	auto ring = std::make_unique<Canvas2D>(gfx, kSize, kSize);
	ring->Clear(Colors::None);

	const float cx = (static_cast<float>(kSize) - 1.0f) * 0.5f;
	const float cy = cx;
	const float rOuter = static_cast<float>(kSize) * 0.48f;
	const float rInner = rOuter - 3.5f;

	for (unsigned y = 0u; y < kSize; ++y)
	{
		for (unsigned x = 0u; x < kSize; ++x)
		{
			const float dx = static_cast<float>(x) - cx;
			const float dy = static_cast<float>(y) - cy;
			const float d = std::sqrt(dx * dx + dy * dy);
			if (d <= rOuter && d >= rInner)
			{
				ring->PutPixel(x, y, ringColor);
			}
		}
	}
	ring->NotifyPixelsChanged();
	ring->LinkTechniques(rg);
	return ring;
}

void ZoneLayoutEditor::EnsureRingVisual_(Graphics& gfx, Rgph::RenderGraph& rg)
{
	if (ringHover_ != nullptr)
	{
		return;
	}

	ringHover_ = MakeRingCanvas_(gfx, rg, Color{ 255u, 230u, 80u, 230u });
	ringValid_ = MakeRingCanvas_(gfx, rg, Color{ 80u, 220u, 120u, 230u });
	ringOverlap_ = MakeRingCanvas_(gfx, rg, Color{ 230u, 80u, 80u, 230u });
}

void ZoneLayoutEditor::Snapshot_()
{
	layoutSnapshot_.clear();

	std::size_t total = 0u;
	for (IModuleZone* zone : zones_)
	{
		if (zone != nullptr)
		{
			total += zone->GetNodeCount();
		}
	}
	layoutSnapshot_.reserve(total);

	for (IModuleZone* zone : zones_)
	{
		if (zone == nullptr)
		{
			continue;
		}
		for (std::size_t i = 0; i < zone->GetNodeCount(); ++i)
		{
			const IFieldNode* node = zone->GetNode(i);
			if (node == nullptr)
			{
				continue;
			}
			layoutSnapshot_.push_back(LayoutSnapshotEntry_{
				node->GetInstanceId(),
				zone->GetZoneId(),
				node->GetLocalPos()
			});
		}
	}
}

IFieldNode* ZoneLayoutEditor::FindNodeById_(std::uint32_t id, ZoneId& outZone) const noexcept
{
	if (id == 0u)
	{
		return nullptr;
	}

	for (IModuleZone* zone : zones_)
	{
		if (zone == nullptr)
		{
			continue;
		}
		for (std::size_t i = 0; i < zone->GetNodeCount(); ++i)
		{
			IFieldNode* node = zone->GetNode(i);
			if (node != nullptr && node->GetInstanceId() == id)
			{
				outZone = zone->GetZoneId();
				return node;
			}
		}
	}

	return nullptr;
}

void ZoneLayoutEditor::ClearAllLayoutGhosts_()
{
	for (IModuleZone* zone : zones_)
	{
		if (zone != nullptr)
		{
			zone->ClearLayoutGhosts();
		}
	}
}

DirectX::XMFLOAT2 ZoneLayoutEditor::MouseGame_() const noexcept
{
	const auto mouse = InputCodex::Get().MousePos();
	return DirectX::XMFLOAT2{
		static_cast<float>(mouse.first),
		static_cast<float>(mouse.second)
	};
}

DirectX::XMFLOAT3 ZoneLayoutEditor::OriginForSource_(ZoneId source) const noexcept
{
	if (IModuleZone* zone = ZoneAt_(source))
	{
		return zone->GetOrigin();
	}
	return DirectX::XMFLOAT3{};
}

DirectX::XMFLOAT2 ZoneLayoutEditor::WorldPosOf_(const IFieldNode& node, DirectX::XMFLOAT3 origin) const noexcept
{
	return DirectX::XMFLOAT2{
		origin.x + node.GetLocalPos().x,
		origin.y + node.GetLocalPos().y
	};
}

IModuleZone* ZoneLayoutEditor::ZoneAt_(ZoneId id) const noexcept
{
	if (!IsBoundZoneId_(id))
	{
		return nullptr;
	}
	return zones_[ToIndex(id)];
}

void ZoneLayoutEditor::SetFreePreview_(IFieldNode& node, DirectX::XMFLOAT2 mouseGame)
{
	node.SetLocalPos(DirectX::XMFLOAT2{
		mouseGame.x - dragOrigin_.x,
		mouseGame.y - dragOrigin_.y
	});
	node.SyncVisual();
}

ZoneLayoutEditor::DropEval_ ZoneLayoutEditor::EvalDrop_(const IFieldNode& node) const noexcept
{
	DropEval_ eval{};
	const DirectX::XMFLOAT2 world = WorldPosOf_(node, dragOrigin_);
	const float radius = node.GetHitRadius();
	const ZoneId from = dragSource_;

	for (std::size_t i = 0; i < ZoneCount(); ++i)
	{
		IModuleZone* zone = zones_[i];
		if (zone == nullptr || !zone->ContainsCircle(world, radius))
		{
			continue;
		}
		const DropResult drop = zone->EvalDrop(node, world, from);
		if (!IsDropAccepted(drop.verdict))
		{
			return eval;
		}
		eval.placeable = true;
		eval.target = static_cast<ZoneId>(i);
		return eval;
	}
	return eval;
}

void ZoneLayoutEditor::RevertDrag_()
{
	if (dragged_ == nullptr)
	{
		return;
	}

	dragged_->SetLocalPos(dragStartLocalPos_);
	if (IModuleZone* source = ZoneAt_(dragSource_))
	{
		dragged_->SetFieldOrigin(source->GetOrigin());
	}
	dragged_->EndLayoutGhost();
	dragged_->SyncVisual();
	if (IModuleZone* source = ZoneAt_(dragSource_))
	{
		source->SyncAllVisuals();
	}
}

void ZoneLayoutEditor::ClearActiveDrag_()
{
	if (dragged_ == nullptr)
	{
		return;
	}
	RevertDrag_();
	dragged_ = nullptr;
	dragSource_ = kNoZone_;
	dragOrigin_ = {};
}

void ZoneLayoutEditor::ResolveRelease_()
{
	if (dragged_ == nullptr)
	{
		return;
	}

	const DropEval_ eval = EvalDrop_(*dragged_);
	IModuleZone* target = ZoneAt_(eval.target);
	IModuleZone* source = ZoneAt_(dragSource_);
	if (!eval.placeable || target == nullptr || source == nullptr)
	{
		RevertDrag_();
		return;
	}

	IFieldNode* node = dragged_;
	const DirectX::XMFLOAT2 world = WorldPosOf_(*node, dragOrigin_);
	const DropResult drop = target->EvalDrop(*node, world, dragSource_);
	if (!IsDropAccepted(drop.verdict))
	{
		RevertDrag_();
		return;
	}

	if (dragSource_ == eval.target)
	{
		node->SetLocalPos(drop.localPos);
		node->SetFieldOrigin(target->GetOrigin());
		node->EndLayoutGhost();
		node->SyncVisual();
		target->SyncAllVisuals();
		return;
	}

	node->EndLayoutGhost();
	std::unique_ptr<IFieldNode> taken = source->TakeNode(node);
	if (!target->TryAcceptDrop(taken, drop.localPos))
	{
		if (taken != nullptr)
		{
			(void)source->TryAcceptDrop(taken, dragStartLocalPos_);
		}
		RevertDrag_();
	}
}

IFieldNode* ZoneLayoutEditor::PickHover_(DirectX::XMFLOAT2 mouseGame, ZoneId& outSource) const noexcept
{
	outSource = kNoZone_;
	IFieldNode* best = nullptr;
	float bestDistSq = 1.0e9f;

	for (IModuleZone* zone : zones_)
	{
		if (zone == nullptr)
		{
			continue;
		}
		float distSq = 1.0e9f;
		IFieldNode* hit = zone->PickAt(mouseGame, distSq);
		if (hit == nullptr)
		{
			continue;
		}
		if (best == nullptr || distSq < bestDistSq)
		{
			best = hit;
			bestDistSq = distSq;
			outSource = zone->GetZoneId();
		}
	}
	return best;
}

void ZoneLayoutEditor::SnapCursorToNode_(IFieldNode& node, DirectX::XMFLOAT3 origin, Window& hostWindow) const noexcept
{
	const DirectX::XMFLOAT2 local = node.GetLocalPos();
	const int gameX = static_cast<int>(std::lround(origin.x + local.x));
	const int gameY = static_cast<int>(std::lround(origin.y + local.y));

	int clientX = 0;
	int clientY = 0;
	if (!hostWindow.MapGameToClient(gameX, gameY, clientX, clientY))
	{
		return;
	}

	POINT pt{ clientX, clientY };
	::ClientToScreen(hostWindow.GetHwnd(), &pt);
	::SetCursorPos(pt.x, pt.y);
}

void ZoneLayoutEditor::SyncRingTransform_(IFieldNode& node, DirectX::XMFLOAT3 origin)
{
	if (ringHover_ != nullptr)
	{
		SyncOneRingTransform_(*ringHover_, node, origin);
	}
	if (ringValid_ != nullptr)
	{
		SyncOneRingTransform_(*ringValid_, node, origin);
	}
	if (ringOverlap_ != nullptr)
	{
		SyncOneRingTransform_(*ringOverlap_, node, origin);
	}
}

void ZoneLayoutEditor::SyncOneRingTransform_(Canvas2D& ring, IFieldNode& node, DirectX::XMFLOAT3 origin) const
{
	const DirectX::XMFLOAT2 local = node.GetLocalPos();
	const float side = node.GetHitRadius() * 2.0f + kRingPadding_;
	ring.SetPosition(DirectX::XMFLOAT3{
		origin.x + local.x,
		origin.y + local.y,
		0.0f
	});
	ring.SetScale(DirectX::XMFLOAT3{ side, side, 1.0f });
}

Canvas2D* ZoneLayoutEditor::ActiveRing_() const noexcept
{
	switch (ringKind_)
	{
	case RingKind_::Valid:
		return ringValid_.get();
	case RingKind_::Overlap:
		return ringOverlap_.get();
	case RingKind_::Hover:
	default:
		return ringHover_.get();
	}
}
