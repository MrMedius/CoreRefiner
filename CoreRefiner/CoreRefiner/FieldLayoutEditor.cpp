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

	ApplyFieldOrigin_(field, canvas, editOrigin_);
	field.SyncAllVisuals();
}

void FieldLayoutEditor::End(ModuleField& field, ModuleFieldCanvas& canvas)
{
	if (!active_)
	{
		return;
	}

	hover_ = nullptr;
	dragged_ = nullptr;
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

	// Drop drag first so a held LMB cannot keep a stale dragged_ pointer.
	hover_ = nullptr;
	dragged_ = nullptr;

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
			TrySetLocalPos_(field, *dragged_, mouseLocal);
			hover_ = dragged_;
		}
		if (input.MouseLeftReleased())
		{
			TrySetLocalPos_(field, *dragged_, mouseLocal);
			dragged_ = nullptr;
		}
	}
	else
	{
		hover_ = PickHover_(field, mouseLocal);
		if (input.MouseLeftTriggered() && hover_ != nullptr)
		{
			dragged_ = hover_;
			if (hostWindow != nullptr)
			{
				SnapCursorToNode_(*dragged_, *hostWindow);
			}
			// Keep pose at grab; do not jump to pre-snap mouse offset this frame.
			TrySetLocalPos_(field, *dragged_, dragged_->GetLocalPos());
		}
	}

	IFieldNode* ringTarget = (dragged_ != nullptr) ? dragged_ : hover_;
	if (ringTarget != nullptr)
	{
		SyncRingTransform_(*ringTarget);
	}
}

void FieldLayoutEditor::SubmitOverlay()
{
	if (!active_ || ring_ == nullptr)
	{
		return;
	}
	if (dragged_ == nullptr && hover_ == nullptr)
	{
		return;
	}
	ring_->Submit(Chan::ui);
}

void FieldLayoutEditor::EnsureRingVisual_(Graphics& gfx, Rgph::RenderGraph& rg)
{
	if (ring_ != nullptr)
	{
		return;
	}

	constexpr unsigned kSize = 64u;
	ring_ = std::make_unique<Canvas2D>(gfx, kSize, kSize);
	ring_->Clear(Colors::None);

	const float cx = (static_cast<float>(kSize) - 1.0f) * 0.5f;
	const float cy = cx;
	const float rOuter = static_cast<float>(kSize) * 0.48f;
	const float rInner = rOuter - 3.5f;
	const Color ringColor{ 255u, 230u, 80u, 230u };

	for (unsigned y = 0u; y < kSize; ++y)
	{
		for (unsigned x = 0u; x < kSize; ++x)
		{
			const float dx = static_cast<float>(x) - cx;
			const float dy = static_cast<float>(y) - cy;
			const float d = std::sqrt(dx * dx + dy * dy);
			if (d <= rOuter && d >= rInner)
			{
				ring_->PutPixel(x, y, ringColor);
			}
		}
	}
	ring_->NotifyPixelsChanged();
	ring_->LinkTechniques(rg);
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

bool FieldLayoutEditor::TrySetLocalPos_(
	ModuleField& field,
	IFieldNode& node,
	DirectX::XMFLOAT2 candidate)
{
	candidate = ClampLocalForNode_(candidate, node.GetHitRadius());
	if (WouldOverlapOthers_(field, node, candidate))
	{
		return false;
	}
	node.SetLocalPos(candidate);
	node.SyncVisual();
	return true;
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
	if (ring_ == nullptr)
	{
		return;
	}

	const DirectX::XMFLOAT2 local = node.GetLocalPos();
	const float side = node.GetHitRadius() * 2.0f + kRingPadding_;
	ring_->SetPosition(DirectX::XMFLOAT3{
		editOrigin_.x + local.x,
		editOrigin_.y + local.y,
		0.0f
	});
	ring_->SetScale(DirectX::XMFLOAT3{ side, side, 1.0f });
}
