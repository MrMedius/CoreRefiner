#include "FieldLayoutEditor.h"
#include "Collision2D.h"
#include "InputCodex.h"
#include "XMath.h"
#include "Colors.h"
#include "Channels.h"
#include "Graphics.h"
#include "Window.h"

#include <algorithm>
#include <cmath>

void FieldLayoutEditor::Begin(
	ModuleField& field,
	ScanAssembler& assembler,
	ModuleFieldCanvas& canvas,
	Graphics& gfx,
	Rgph::RenderGraph& rg,
	DirectX::XMFLOAT3 combatOrigin)
{
	combatOrigin_ = combatOrigin;
	editOrigin_ = DirectX::XMFLOAT3{
		static_cast<float>(SCREEN_WIDTH) * 0.5f,
		static_cast<float>(SCREEN_HEIGHT) * 0.5f,
		0.0f
	};

	assembler.Reset();
	canvas.ClearWaves();
	// Ready all nodes so cooldown masks do not fight layout overlays.
	field.ResetAllCooldowns();
	Snapshot_(field);
	EnsureRingVisual_(gfx, rg);

	hover_ = nullptr;
	dragged_ = nullptr;
	active_ = true;
	dragStartLocalPos_ = {};
	ringKind_ = RingKind_::Hover;

	ApplyFieldOrigin_(field, canvas, editOrigin_);
	field.SyncAllVisuals();
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

	hover_ = nullptr;
	dragged_ = nullptr;
	active_ = false;
	dragStartLocalPos_ = {};

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
	}
	ClearAllLayoutGhosts_(field);

	// Drop drag first so a held LMB cannot keep a stale dragged_ pointer.
	hover_ = nullptr;
	dragged_ = nullptr;
	dragStartLocalPos_ = {};

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
}

void FieldLayoutEditor::Update(float dt, ModuleField& field, Window* hostWindow)
{
	(void)dt;
	if (!active_)
	{
		return;
	}

	auto& input = InputCodex::Get();

	// Esc cancels layout edits (keeps pause / edit origin); skip mouse this frame.
	if (input.KeyTriggered(KK_ESCAPE))
	{
		CancelRestore(field);
		return;
	}

	const DirectX::XMFLOAT2 mouseLocal = MouseToLocal_();

	if (dragged_ != nullptr)
	{
		if (input.MouseLeftPressed())
		{
			SetPreviewLocalPos_(*dragged_, mouseLocal);
			hover_ = dragged_;
		}
		if (input.MouseLeftReleased())
		{
			SetPreviewLocalPos_(*dragged_, mouseLocal);
			CommitOrRevertDrag_(field, *dragged_);
			dragged_->EndLayoutGhost();
			dragged_ = nullptr;
		}
	}
	else
	{
		hover_ = PickHover_(field, mouseLocal);
		if (input.MouseLeftTriggered() && hover_ != nullptr)
		{
			dragged_ = hover_;
			dragStartLocalPos_ = dragged_->GetLocalPos();
			dragged_->BeginLayoutGhost(dragStartLocalPos_);
			if (hostWindow != nullptr)
			{
				SnapCursorToNode_(*dragged_, *hostWindow);
			}
			// Keep pose at grab; do not jump to pre-snap mouse offset this frame.
			SetPreviewLocalPos_(*dragged_, dragged_->GetLocalPos());
		}
	}

	IFieldNode* ringTarget = (dragged_ != nullptr) ? dragged_ : hover_;
	if (dragged_ != nullptr)
	{
		ringKind_ = WouldOverlapOthers_(field, *dragged_, dragged_->GetLocalPos())
			? RingKind_::Overlap
			: RingKind_::Valid;
	}
	else
	{
		ringKind_ = RingKind_::Hover;
	}
	if (ringTarget != nullptr)
	{
		SyncRingTransform_(*ringTarget);
	}
}

void FieldLayoutEditor::SubmitOverlay()
{
	if (!active_)
	{
		return;
	}
	if (dragged_ == nullptr && hover_ == nullptr)
	{
		return;
	}
	Canvas2D* ring = ActiveRing_();
	if (ring == nullptr)
	{
		return;
	}
	ring->Submit(Chan::ui);
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

	// Hover yellow (existing), valid green, overlap red — baked once, switch on Submit.
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
}

DirectX::XMFLOAT2 FieldLayoutEditor::MouseToLocal_() const noexcept
{
	const auto mouse = InputCodex::Get().MousePos();
	return DirectX::XMFLOAT2{
		static_cast<float>(mouse.first) - editOrigin_.x,
		static_cast<float>(mouse.second) - editOrigin_.y
	};
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

void FieldLayoutEditor::SetPreviewLocalPos_(IFieldNode& node, DirectX::XMFLOAT2 candidate)
{
	candidate = ClampLocalForNode_(candidate, node.GetHitRadius());
	node.SetLocalPos(candidate);
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

IFieldNode* FieldLayoutEditor::PickHover_(ModuleField& field, DirectX::XMFLOAT2 mouseLocal) const noexcept
{
	IFieldNode* best = nullptr;
	float bestDistSq = 1.0e9f;
	const Collider2D::PointCollider mousePt{ mouseLocal };

	const std::size_t n = field.GetNodeCount();
	for (std::size_t i = 0; i < n; ++i)
	{
		IFieldNode* node = field.GetNode(i);
		if (node == nullptr)
		{
			continue;
		}

		const Collider2D::CircleCollider hit{ node->GetLocalPos(), node->GetHitRadius() };
		if (!Collider2D::CollisionSystem::IsOverlap(hit, mousePt))
		{
			continue;
		}

		const Vec2 d = V(mouseLocal) - V(node->GetLocalPos());
		const float distSq = d.LengthSq();
		if (distSq < bestDistSq)
		{
			bestDistSq = distSq;
			best = node;
		}
	}
	return best;
}

void FieldLayoutEditor::SnapCursorToNode_(IFieldNode& node, Window& hostWindow) const noexcept
{
	const DirectX::XMFLOAT2 local = node.GetLocalPos();
	const int gameX = static_cast<int>(std::lround(editOrigin_.x + local.x));
	const int gameY = static_cast<int>(std::lround(editOrigin_.y + local.y));

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

void FieldLayoutEditor::SyncRingTransform_(IFieldNode& node)
{
	if (ringHover_ != nullptr)
	{
		SyncOneRingTransform_(*ringHover_, node);
	}
	if (ringValid_ != nullptr)
	{
		SyncOneRingTransform_(*ringValid_, node);
	}
	if (ringOverlap_ != nullptr)
	{
		SyncOneRingTransform_(*ringOverlap_, node);
	}
}

void FieldLayoutEditor::SyncOneRingTransform_(Canvas2D& ring, IFieldNode& node) const
{
	const DirectX::XMFLOAT2 local = node.GetLocalPos();
	const float side = node.GetHitRadius() * 2.0f + kRingPadding_;
	ring.SetPosition(DirectX::XMFLOAT3{
		editOrigin_.x + local.x,
		editOrigin_.y + local.y,
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
