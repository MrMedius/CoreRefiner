#include "FieldLayoutEditor.h"
#include "Collision2D.h"
#include "FieldNodeInfoCopy.h"
#include "InputCodex.h"
#include "XMath.h"
#include "Colors.h"
#include "Channels.h"
#include "Graphics.h"
#include "Window.h"

#include <algorithm>
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
}

void FieldLayoutEditor::Begin(
	ModuleField& field,
	ModuleWarehouse& warehouse,
	ScanAssembler& assembler,
	ModuleFieldCanvas& canvas,
	Graphics& gfx,
	Rgph::RenderGraph& rg,
	DirectX::XMFLOAT3 combatOrigin)
{
	combatOrigin_ = combatOrigin;
	warehouse_ = &warehouse;
	editOrigin_ = DirectX::XMFLOAT3{
		static_cast<float>(SCREEN_WIDTH) * 0.32f,
		static_cast<float>(SCREEN_HEIGHT) * 0.5f,
		0.0f
	};
	const DirectX::XMFLOAT3 warehouseOrigin{
		static_cast<float>(SCREEN_WIDTH) * 0.78f,
		static_cast<float>(SCREEN_HEIGHT) * 0.5f,
		0.0f
	};

	assembler.Reset();
	canvas.ClearWaves();
	field.ResetAllCooldowns();
	EnsureRingVisual_(gfx, rg);
	EnsureFieldNodeInfoCopyLoaded_();
	infoPanel_.Ensure(gfx, rg);
	infoPanel_.Hide();

	hover_ = nullptr;
	dragged_ = nullptr;
	hoverSource_ = DragSource_::None;
	dragSource_ = DragSource_::None;
	active_ = true;
	dragStartLocalPos_ = {};
	dragOrigin_ = {};
	ringKind_ = RingKind_::Hover;

	ApplyFieldOrigin_(field, canvas, editOrigin_);
	field.SyncAllVisuals();

	warehouse.SetOrigin(warehouseOrigin);
	warehouse.SyncAllVisuals();

	// warehouse_ already bound; snapshot both Field and Warehouse by instanceId.
	Snapshot_(field);
}

void FieldLayoutEditor::End(ModuleField& field, ModuleFieldCanvas& canvas)
{
	if (!active_)
	{
		return;
	}

	// Keep the current Field/Warehouse layout. Do NOT apply layoutSnapshot_
	// (Esc CancelRestore is the only path that restores Begin poses).
	ClearActiveDrag_(field);
	ClearAllLayoutGhosts_(field);
	infoPanel_.Hide();

	hover_ = nullptr;
	hoverSource_ = DragSource_::None;
	ringKind_ = RingKind_::Hover;
	dragStartLocalPos_ = {};

	if (warehouse_ != nullptr)
	{
		warehouse_->RelayoutSlots();
	}
	warehouse_ = nullptr;
	layoutSnapshot_.clear();
	active_ = false;

	ApplyFieldOrigin_(field, canvas, combatOrigin_);
	field.SyncAllVisuals();
}

void FieldLayoutEditor::CancelRestore(ModuleField& field)
{
	if (!active_)
	{
		return;
	}

	ClearActiveDrag_(field);
	ClearAllLayoutGhosts_(field);
	infoPanel_.Hide();

	hover_ = nullptr;
	hoverSource_ = DragSource_::None;
	ringKind_ = RingKind_::Hover;
	dragStartLocalPos_ = {};

	struct PendingMigrate_
	{
		std::unique_ptr<IFieldNode> node;
		LayoutZone_ target{ LayoutZone_::Field };
	};
	std::vector<PendingMigrate_> pending;
	pending.reserve(layoutSnapshot_.size());

	// Pass 1: pull every zone-mismatched node out so capacity is free before re-adopt.
	for (const LayoutSnapshotEntry_& entry : layoutSnapshot_)
	{
		LayoutZone_ curZone = LayoutZone_::Field;
		IFieldNode* node = FindNodeById_(field, entry.id, curZone);
		if (node == nullptr || curZone == entry.zone)
		{
			continue;
		}

		std::unique_ptr<IFieldNode> taken;
		if (curZone == LayoutZone_::Field)
		{
			taken = field.TakeNode(node);
		}
		else if (warehouse_ != nullptr)
		{
			taken = warehouse_->TakeNode(node);
		}
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
		if (p.target == LayoutZone_::Field)
		{
			return field.AdoptNode(std::move(p.node));
		}
		if (warehouse_ == nullptr)
		{
			return field.AdoptNode(std::move(p.node));
		}
		IFieldNode* raw = warehouse_->TryAdopt(p.node);
		if (raw != nullptr)
		{
			return raw;
		}
		// Full / Core: keep ownership on Field so the node is not dropped.
		return field.AdoptNode(std::move(p.node));
	};

	// Pass 2a: return Field-bound nodes first (frees warehouse slots).
	for (PendingMigrate_& p : pending)
	{
		if (p.target == LayoutZone_::Field)
		{
			(void)adoptOne_(p);
		}
	}
	// Pass 2b: warehouse targets.
	for (PendingMigrate_& p : pending)
	{
		if (p.target == LayoutZone_::Warehouse)
		{
			(void)adoptOne_(p);
		}
	}

	// Pass 3: restore Begin localPos/origin (also repairs Take/TryAdopt RelayoutSlots side effects).
	for (const LayoutSnapshotEntry_& entry : layoutSnapshot_)
	{
		LayoutZone_ curZone = LayoutZone_::Field;
		IFieldNode* node = FindNodeById_(field, entry.id, curZone);
		if (node == nullptr || curZone != entry.zone)
		{
			continue;
		}

		node->SetLocalPos(entry.localPos);
		if (entry.zone == LayoutZone_::Field)
		{
			node->SetFieldOrigin(editOrigin_);
		}
		else if (warehouse_ != nullptr)
		{
			node->SetFieldOrigin(warehouse_->GetOrigin());
		}
		if (node->IsLayoutGhostActive())
		{
			node->EndLayoutGhost();
		}
		node->SyncVisual();
	}

	field.SyncAllVisuals();
	if (warehouse_ != nullptr)
	{
		warehouse_->SyncAllVisuals();
	}
}

void FieldLayoutEditor::Update(float dt, ModuleField& field, Window* hostWindow)
{
	(void)dt;
	if (!active_)
	{
		return;
	}

	auto& input = InputCodex::Get();

	if (input.KeyTriggered(KK_ESCAPE))
	{
		CancelRestore(field);
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
			ResolveRelease_(field);
			dragged_ = nullptr;
			dragSource_ = DragSource_::None;
			dragOrigin_ = {};
			hover_ = PickHover_(field, mouseGame, hoverSource_);
		}
	}
	else
	{
		hover_ = PickHover_(field, mouseGame, hoverSource_);
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
	const DragSource_ ringSource = (dragged_ != nullptr) ? dragSource_ : hoverSource_;
	if (dragged_ != nullptr)
	{
		const DropEval_ eval = EvalDrop_(field, *dragged_);
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

void FieldLayoutEditor::SubmitOverlay()
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

std::unique_ptr<Canvas2D> FieldLayoutEditor::MakeRingCanvas_(
	Graphics& gfx,
	Rgph::RenderGraph& rg,
	Color ringColor)
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

void FieldLayoutEditor::EnsureRingVisual_(Graphics& gfx, Rgph::RenderGraph& rg)
{
	if (ringHover_ != nullptr)
	{
		return;
	}

	ringHover_ = MakeRingCanvas_(gfx, rg, Color{ 255u, 230u, 80u, 230u });
	ringValid_ = MakeRingCanvas_(gfx, rg, Color{ 80u, 220u, 120u, 230u });
	ringOverlap_ = MakeRingCanvas_(gfx, rg, Color{ 230u, 80u, 80u, 230u });
}

void FieldLayoutEditor::Snapshot_(const ModuleField& field)
{
	layoutSnapshot_.clear();
	layoutSnapshot_.reserve(field.GetNodeCount()
		+ ((warehouse_ != nullptr) ? warehouse_->GetNodeCount() : 0u));

	for (std::size_t i = 0; i < field.GetNodeCount(); ++i)
	{
		const IFieldNode* node = field.GetNode(i);
		if (node == nullptr)
		{
			continue;
		}
		layoutSnapshot_.push_back(LayoutSnapshotEntry_{
			node->GetInstanceId(),
			LayoutZone_::Field,
			node->GetLocalPos()
		});
	}

	if (warehouse_ == nullptr)
	{
		return;
	}
	for (std::size_t i = 0; i < warehouse_->GetNodeCount(); ++i)
	{
		const IFieldNode* node = warehouse_->GetNode(i);
		if (node == nullptr)
		{
			continue;
		}
		layoutSnapshot_.push_back(LayoutSnapshotEntry_{
			node->GetInstanceId(),
			LayoutZone_::Warehouse,
			node->GetLocalPos()
		});
	}
}

IFieldNode* FieldLayoutEditor::FindNodeById_(
	ModuleField& field,
	std::uint32_t id,
	LayoutZone_& outZone) const noexcept
{
	if (id == 0u)
	{
		return nullptr;
	}

	for (std::size_t i = 0; i < field.GetNodeCount(); ++i)
	{
		IFieldNode* node = field.GetNode(i);
		if (node != nullptr && node->GetInstanceId() == id)
		{
			outZone = LayoutZone_::Field;
			return node;
		}
	}

	if (warehouse_ != nullptr)
	{
		for (std::size_t i = 0; i < warehouse_->GetNodeCount(); ++i)
		{
			IFieldNode* node = warehouse_->GetNode(i);
			if (node != nullptr && node->GetInstanceId() == id)
			{
				outZone = LayoutZone_::Warehouse;
				return node;
			}
		}
	}

	return nullptr;
}

void FieldLayoutEditor::ApplyFieldOrigin_(
	ModuleField& field,
	ModuleFieldCanvas& canvas,
	DirectX::XMFLOAT3 origin)
{
	field.SetFieldOrigin(origin);
	canvas.SetPosition(origin);
}

void FieldLayoutEditor::ClearAllLayoutGhosts_(ModuleField& field)
{
	field.ForEach([](IFieldNode& node)
	{
		if (node.IsLayoutGhostActive())
		{
			node.EndLayoutGhost();
		}
	});
	if (warehouse_ != nullptr)
	{
		warehouse_->ForEach([](IFieldNode& node)
		{
			if (node.IsLayoutGhostActive())
			{
				node.EndLayoutGhost();
			}
		});
	}
}

DirectX::XMFLOAT2 FieldLayoutEditor::MouseGame_() const noexcept
{
	const auto mouse = InputCodex::Get().MousePos();
	return DirectX::XMFLOAT2{
		static_cast<float>(mouse.first),
		static_cast<float>(mouse.second)
	};
}

DirectX::XMFLOAT3 FieldLayoutEditor::OriginForSource_(DragSource_ source) const noexcept
{
	if (source == DragSource_::Warehouse && warehouse_ != nullptr)
	{
		return warehouse_->GetOrigin();
	}
	return editOrigin_;
}

DirectX::XMFLOAT2 FieldLayoutEditor::WorldPosOf_(
	const IFieldNode& node,
	DirectX::XMFLOAT3 origin) const noexcept
{
	return DirectX::XMFLOAT2{
		origin.x + node.GetLocalPos().x,
		origin.y + node.GetLocalPos().y
	};
}

bool FieldLayoutEditor::FieldContainsCircle_(
	DirectX::XMFLOAT2 worldCenter,
	float radius) const noexcept
{
	const float r = (std::max)(radius, 0.0f);
	const float usable = ModuleField::kHalfExtent - r;
	if (usable < 0.0f)
	{
		return false;
	}
	const float lx = worldCenter.x - editOrigin_.x;
	const float ly = worldCenter.y - editOrigin_.y;
	return std::fabs(lx) <= usable && std::fabs(ly) <= usable;
}

DirectX::XMFLOAT2 FieldLayoutEditor::ClampLocalForNode_(
	DirectX::XMFLOAT2 p,
	float hitRadius) const noexcept
{
	const float half = (std::max)(ModuleField::kHalfExtent - hitRadius, 0.0f);
	const Collider2D::BoxCollider bounds = Collider2D::BoxCollider::MakeCenteredSquare(half);
	return Collider2D::ClampPointToBox(p, bounds);
}

bool FieldLayoutEditor::WouldOverlapOthers_(
	const ModuleField& field,
	const IFieldNode& self,
	DirectX::XMFLOAT2 fieldLocal) const noexcept
{
	const Collider2D::CircleCollider moving{ fieldLocal, self.GetHitRadius() };
	const std::size_t n = field.GetNodeCount();
	for (std::size_t i = 0; i < n; ++i)
	{
		const IFieldNode* other = field.GetNode(i);
		if (other == nullptr || other == &self)
		{
			continue;
		}
		const Collider2D::CircleCollider solid{ other->GetLocalPos(), other->GetHitRadius() };
		if (Collider2D::CollisionSystem::IsOverlap(moving, solid))
		{
			return true;
		}
	}
	return false;
}

void FieldLayoutEditor::SetFreePreview_(IFieldNode& node, DirectX::XMFLOAT2 mouseGame)
{
	node.SetLocalPos(DirectX::XMFLOAT2{
		mouseGame.x - dragOrigin_.x,
		mouseGame.y - dragOrigin_.y
	});
	node.SyncVisual();
}

FieldLayoutEditor::DropEval_ FieldLayoutEditor::EvalDrop_(
	const ModuleField& field,
	const IFieldNode& node) const noexcept
{
	DropEval_ eval{};
	const DirectX::XMFLOAT2 world = WorldPosOf_(node, dragOrigin_);
	const float radius = node.GetHitRadius();

	const bool inField = FieldContainsCircle_(world, radius);
	const bool inWarehouse = (warehouse_ != nullptr)
		&& warehouse_->ContainsCircle(world, radius);

	if (!inField && !inWarehouse)
	{
		return eval;
	}

	if (inField)
	{
		const DirectX::XMFLOAT2 fieldLocal{
			world.x - editOrigin_.x,
			world.y - editOrigin_.y
		};
		if (WouldOverlapOthers_(field, node, fieldLocal))
		{
			return eval;
		}
		eval.placeable = true;
		eval.target = DropTarget_::Field;
		return eval;
	}

	// Warehouse zone only.
	if (dragSource_ == DragSource_::Warehouse)
	{
		eval.placeable = true;
		eval.target = DropTarget_::Warehouse;
		return eval;
	}

	// Field → Warehouse: reject Core / full stock.
	if (node.IsCore() || warehouse_ == nullptr || !warehouse_->HasFreeSlot())
	{
		return eval;
	}
	eval.placeable = true;
	eval.target = DropTarget_::Warehouse;
	return eval;
}

void FieldLayoutEditor::RevertDrag_(ModuleField& field)
{
	(void)field;
	if (dragged_ == nullptr)
	{
		return;
	}

	if (dragSource_ == DragSource_::Warehouse && warehouse_ != nullptr)
	{
		dragged_->EndLayoutGhost();
		warehouse_->RelayoutSlots();
		return;
	}

	dragged_->SetLocalPos(dragStartLocalPos_);
	dragged_->SetFieldOrigin(editOrigin_);
	dragged_->EndLayoutGhost();
	dragged_->SyncVisual();
}

void FieldLayoutEditor::ClearActiveDrag_(ModuleField& field)
{
	if (dragged_ == nullptr)
	{
		return;
	}
	RevertDrag_(field);
	dragged_ = nullptr;
	dragSource_ = DragSource_::None;
	dragOrigin_ = {};
}

void FieldLayoutEditor::ResolveRelease_(ModuleField& field)
{
	if (dragged_ == nullptr)
	{
		return;
	}

	const DropEval_ eval = EvalDrop_(field, *dragged_);
	if (!eval.placeable || eval.target == DropTarget_::None)
	{
		RevertDrag_(field);
		return;
	}

	IFieldNode* node = dragged_;
	const DirectX::XMFLOAT2 world = WorldPosOf_(*node, dragOrigin_);

	if (eval.target == DropTarget_::Field)
	{
		DirectX::XMFLOAT2 fieldLocal{
			world.x - editOrigin_.x,
			world.y - editOrigin_.y
		};
		fieldLocal = ClampLocalForNode_(fieldLocal, node->GetHitRadius());

		if (dragSource_ == DragSource_::Warehouse && warehouse_ != nullptr)
		{
			node->EndLayoutGhost();
			std::unique_ptr<IFieldNode> taken = warehouse_->TakeNode(node);
			IFieldNode* adopted = field.AdoptNode(std::move(taken));
			if (adopted == nullptr)
			{
				RevertDrag_(field);
				return;
			}
			adopted->SetLocalPos(fieldLocal);
			adopted->SetFieldOrigin(editOrigin_);
			adopted->SyncVisual();
			return;
		}

		// Same-zone Field drop.
		node->SetLocalPos(fieldLocal);
		node->SetFieldOrigin(editOrigin_);
		node->EndLayoutGhost();
		node->SyncVisual();
		return;
	}

	if (eval.target == DropTarget_::Warehouse && warehouse_ != nullptr)
	{
		if (dragSource_ == DragSource_::Field)
		{
			node->EndLayoutGhost();
			std::unique_ptr<IFieldNode> taken = field.TakeNode(node);
			if (warehouse_->TryAdopt(taken) == nullptr)
			{
				IFieldNode* back = field.AdoptNode(std::move(taken));
				if (back != nullptr)
				{
					back->SetLocalPos(dragStartLocalPos_);
					back->SetFieldOrigin(editOrigin_);
					back->SyncVisual();
				}
				return;
			}
			return;
		}

		// Same-zone Warehouse drop.
		node->EndLayoutGhost();
		warehouse_->RelayoutSlots();
	}
}

IFieldNode* FieldLayoutEditor::PickHover_(
	ModuleField& field,
	DirectX::XMFLOAT2 mouseGame,
	DragSource_& outSource) const noexcept
{
	IFieldNode* best = nullptr;
	float bestDistSq = 1.0e9f;
	outSource = DragSource_::None;

	auto consider = [&](IFieldNode& node, DirectX::XMFLOAT3 origin, DragSource_ source)
	{
		const DirectX::XMFLOAT2 world = WorldPosOf_(node, origin);
		const Collider2D::CircleCollider hit{ world, node.GetHitRadius() };
		const Collider2D::PointCollider mousePt{ mouseGame };
		if (!Collider2D::CollisionSystem::IsOverlap(hit, mousePt))
		{
			return;
		}
		const Vec2 d = V(mouseGame) - V(world);
		const float distSq = d.LengthSq();
		if (distSq < bestDistSq)
		{
			bestDistSq = distSq;
			best = &node;
			outSource = source;
		}
	};

	const std::size_t fieldN = field.GetNodeCount();
	for (std::size_t i = 0; i < fieldN; ++i)
	{
		IFieldNode* node = field.GetNode(i);
		if (node != nullptr)
		{
			consider(*node, editOrigin_, DragSource_::Field);
		}
	}

	if (warehouse_ != nullptr)
	{
		const DirectX::XMFLOAT3 whOrigin = warehouse_->GetOrigin();
		const std::size_t whN = warehouse_->GetNodeCount();
		for (std::size_t i = 0; i < whN; ++i)
		{
			IFieldNode* node = warehouse_->GetNode(i);
			if (node != nullptr)
			{
				consider(*node, whOrigin, DragSource_::Warehouse);
			}
		}
	}

	return best;
}

void FieldLayoutEditor::SnapCursorToNode_(
	IFieldNode& node,
	DirectX::XMFLOAT3 origin,
	Window& hostWindow) const noexcept
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

void FieldLayoutEditor::SyncRingTransform_(IFieldNode& node, DirectX::XMFLOAT3 origin)
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

void FieldLayoutEditor::SyncOneRingTransform_(
	Canvas2D& ring,
	IFieldNode& node,
	DirectX::XMFLOAT3 origin) const
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

Canvas2D* FieldLayoutEditor::ActiveRing_() const noexcept
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
