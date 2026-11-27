#include "ObjectBase.h"

#include <algorithm>
#include <cassert>
#include <cmath>

ObjectBase::~ObjectBase()
{
	// Pool/shutdown destruction: unlink without cascading Deactivate (objects may already be tearing down).
	DetachAllChildren_();
	DetachFromParentOnly_();
}

void ObjectBase::Activate()
{
	DeferredDisableQueue::Get().Remove(this);
	pendingDisable_ = false;
	IsUse = true;
	OnEnable();
	EnableComponents();
}

void ObjectBase::Deactivate()
{
	DeferredDisableQueue::Get().Remove(this);
	if (!IsUse && !pendingDisable_)
	{
		return;
	}

	// Cascade: copy first — ClearParent mutates children_.
	const std::vector<ObjectBase*> kids = children_;
	for (ObjectBase* child : kids)
	{
		if (child != nullptr)
		{
			child->Deactivate();
		}
	}
	DetachAllChildren_();
	DetachFromParentOnly_();

	DisableComponents();
	IsUse = false;
	pendingDisable_ = false;
}

void ObjectBase::RequestDisable()
{
	if (!IsUse || pendingDisable_)
	{
		return;
	}

	const std::vector<ObjectBase*> kids = children_;
	for (ObjectBase* child : kids)
	{
		if (child != nullptr)
		{
			child->RequestDisable();
		}
	}

	pendingDisable_ = true;
	DeferredDisableQueue::Get().Enqueue(this);
}

DirectX::XMMATRIX ObjectBase::GetLocalMatrix() const noexcept
{
	return transform_.GetLocalMatrix();
}

DirectX::XMMATRIX ObjectBase::GetWorldMatrix() const noexcept
{
	using namespace DirectX;
	const XMMATRIX local = GetLocalMatrix();
	if (parent_ == nullptr)
	{
		return local;
	}
	// Row vectors: v * local * parentWorld
	return local * parent_->GetWorldMatrix();
}

DirectX::XMFLOAT3 ObjectBase::GetWorldPosition() const noexcept
{
	using namespace DirectX;
	XMFLOAT3 out{};
	XMStoreFloat3(&out, GetWorldMatrix().r[3]);
	return out;
}

namespace
{
	// row 是世界矩阵的行。fallback 在这行长度接近 0 时返回。
	XMFLOAT3 AxisFromWorld_(const DirectX::XMMATRIX& world, int row, XMFLOAT3 fallback) noexcept
	{
		using namespace DirectX;
		const XMVECTOR axis = world.r[row];
		if (XMVectorGetX(XMVector3LengthSq(axis)) < 1e-8f)
		{
			return fallback;
		}
		XMFLOAT3 out{};
		XMStoreFloat3(&out, XMVector3Normalize(axis));
		return out;
	}
}

XMFLOAT3 ObjectBase::GetRight() const noexcept
{
	return AxisFromWorld_(GetWorldMatrix(), 0, { 1.0f, 0.0f, 0.0f });
}

XMFLOAT3 ObjectBase::GetUp() const noexcept
{
	return AxisFromWorld_(GetWorldMatrix(), 1, { 0.0f, 1.0f, 0.0f });
}

XMFLOAT3 ObjectBase::GetForward() const noexcept
{
	return AxisFromWorld_(GetWorldMatrix(), 2, { 0.0f, 0.0f, 1.0f });
}

void ObjectBase::AimRightAt(XMFLOAT3 direction)
{
	// dx、dy、dz 是归一化后的目标方向。
	float dx = direction.x;
	float dy = direction.y;
	float dz = direction.z;
	const float lenSq = dx * dx + dy * dy + dz * dz;
	if (lenSq < 1e-8f)
	{
		SetRotation({ 0.0f, 0.0f, 0.0f });
		return;
	}

	const float invLen = 1.0f / sqrtf(lenSq);
	dx *= invLen;
	dy *= invLen;
	dz *= invLen;
	if (dy > 1.0f)
	{
		dy = 1.0f;
	}
	else if (dy < -1.0f)
	{
		dy = -1.0f;
	}

	// pitch 保持 0。roll 把 +X 抬到方向的高度，yaw 再把它转到水平方向上。
	const float yaw = atan2f(-dz, dx);
	const float roll = asinf(dy);
	SetRotation({ 0.0f, yaw, roll });
}

void ObjectBase::AimForwardAt(XMFLOAT3 direction)
{
	// dx、dy、dz 是归一化后的目标方向。本地 +Z 转到这个方向。
	float dx = direction.x;
	float dy = direction.y;
	float dz = direction.z;
	const float lenSq = dx * dx + dy * dy + dz * dz;
	if (lenSq < 1e-8f)
	{
		SetRotation({ 0.0f, 0.0f, 0.0f });
		return;
	}

	const float invLen = 1.0f / sqrtf(lenSq);
	dx *= invLen;
	dy *= invLen;
	dz *= invLen;
	if (dy > 1.0f)
	{
		dy = 1.0f;
	}
	else if (dy < -1.0f)
	{
		dy = -1.0f;
	}

	// pitch 把 +Z 抬到方向的高度。yaw 把它转到水平方向上。roll 保持 0。
	const float pitch = asinf(-dy);
	float yaw = 0.0f;
	if (cosf(pitch) > 1e-4f)
	{
		yaw = atan2f(dx, dz);
	}
	SetRotation({ pitch, yaw, 0.0f });
}

void ObjectBase::SetParent(ObjectBase* parent)
{
	if (parent == parent_)
	{
		return;
	}
	if (parent == this)
	{
		assert(false && "ObjectBase::SetParent: cannot parent to self");
		return;
	}
	if (parent != nullptr && IsAncestorOf(this, parent))
	{
		assert(false && "ObjectBase::SetParent: cycle detected");
		return;
	}

	DetachFromParentOnly_();
	if (parent == nullptr)
	{
		return;
	}

	parent_ = parent;
	parent_->children_.push_back(this);
}

void ObjectBase::ClearParent()
{
	DetachFromParentOnly_();
}

void ObjectBase::ResetHierarchy()
{
	DetachAllChildren_();
	DetachFromParentOnly_();
}

ObjectBase* ObjectBase::GetChild(size_t index) const noexcept
{
	if (index >= children_.size())
	{
		return nullptr;
	}
	return children_[index];
}

bool ObjectBase::IsAncestorOf(const ObjectBase* ancestor, const ObjectBase* node) noexcept
{
	for (const ObjectBase* cur = node; cur != nullptr; cur = cur->parent_)
	{
		if (cur == ancestor)
		{
			return true;
		}
	}
	return false;
}

void ObjectBase::DetachFromParentOnly_() noexcept
{
	if (parent_ == nullptr)
	{
		return;
	}
	auto& siblings = parent_->children_;
	siblings.erase(std::remove(siblings.begin(), siblings.end(), this), siblings.end());
	parent_ = nullptr;
}

void ObjectBase::DetachAllChildren_() noexcept
{
	const std::vector<ObjectBase*> kids = children_;
	children_.clear();
	for (ObjectBase* child : kids)
	{
		if (child != nullptr && child->parent_ == this)
		{
			child->parent_ = nullptr;
		}
	}
}
