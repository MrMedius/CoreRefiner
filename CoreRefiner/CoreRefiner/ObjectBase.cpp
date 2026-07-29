#include "ObjectBase.h"

#include <algorithm>
#include <cassert>

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
