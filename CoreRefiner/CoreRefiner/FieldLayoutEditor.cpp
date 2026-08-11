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
	// Field left, warehouse right — room for pause layout interaction.
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
	Snapshot_(field);
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
}

void FieldLayoutEditor::End(ModuleField& field, ModuleFieldCanvas& canvas)
{
	if (!active_)
	{
		return;
	}

	if (dragged_ != nullptr)
	{
		dragged_->EndLayoutGhost();
	}
	ClearAllLayoutGhosts_(field);
	infoPanel_.Hide();

	hover_ = nullptr;
	dragged_ = nullptr;
	hoverSource_ = DragSource_::None;
	dragSource_ = DragSource_::None;
	warehouse_ = nullptr;
	active_ = false;
	dragStartLocalPos_ = {};
	dragOrigin_ = {};

	ApplyFieldOrigin_(field, canvas, combatOrigin_);
	field.SyncAllVisuals();
}

void FieldLayoutEditor::CancelRestore(ModuleField& field)
{
	if (!active_)
	{
		return;
	}

	if (dragged_ != nullptr)
	{
		dragged_->EndLayoutGhost();
		if (dragSource_ == DragSource_::Warehouse && warehouse_ != nullptr)
		{
			warehouse_->RelayoutSlots();
		}
		else if (dragSource_ == DragSource_::Field)
		{
			dragged_->SetLocalPos(dragStartLocalPos_);
			dragged_->SyncVisual();
		}
	}
	ClearAllLayoutGhosts_(field);
	infoPanel_.Hide();

	hover_ = nullptr;
	dragged_ = nullptr;
	hoverSource_ = DragSource_::None;
	dragSource_ = DragSource_::None;
	dragStartLocalPos_ = {};
	dragOrigin_ = {};

	const std::size_t n = (std::min)(field.GetNodeCount(), snapshotLocalPos_.size());
	for (std::size_t i = 0; i < n; ++i)
	{
		IFieldNode* node = field.GetNode(i);
		if (node != nullptr)
		{
			node->SetLocalPos(snapshotLocalPos_[i]);
		}
	}
	field.SyncAllVisuals();
	if (warehouse_ != nullptr)
	{
		warehouse_->RelayoutSlots();
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
			if (dragSource_ == DragSource_::Field)
			{
				CommitOrRevertDrag_(field, *dragged_);
			}
			else if (dragSource_ == DragSource_::Warehouse && warehouse_ != nullptr)
			{
				// Step 7: no cross-zone transfer yet — snap warehouse back to grid.
				warehouse_->RelayoutSlots();
			}
			dragged_->EndLayoutGhost();
			dragged_ = nullptr;
			dragSource_ = DragSource_::None;
			dragOrigin_ = {};
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
		if (dragSource_ == DragSource_::Field)
		{
			ringKind_ = WouldOverlapOthers_(field, *dragged_, dragged_->GetLocalPos())
				? RingKind_::Overlap
				: RingKind_::Valid;
		}
		else
		{
			ringKind_ = RingKind_::Valid;
		}
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
	const std::size_t n = field.GetNodeCount();
	snapshotLocalPos_.resize(n);
	for (std::size_t i = 0; i < n; ++i)
	{
		const IFieldNode* node = field.GetNode(i);
		snapshotLocalPos_[i] = (node != nullptr)
			? node->GetLocalPos()
			: DirectX::XMFLOAT2{ 0.0f, 0.0f };
	}
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
	DirectX::XMFLOAT2 candidate) const noexcept
{
	const Collider2D::CircleCollider moving{ candidate, self.GetHitRadius() };
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

void FieldLayoutEditor::CommitOrRevertDrag_(ModuleField& field, IFieldNode& node)
{
	const DirectX::XMFLOAT2 pose = node.GetLocalPos();
	if (WouldOverlapOthers_(field, node, pose))
	{
		node.SetLocalPos(dragStartLocalPos_);
		node.SyncVisual();
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
		const DirectX::XMFLOAT2 world{
			origin.x + node.GetLocalPos().x,
			origin.y + node.GetLocalPos().y
		};
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
