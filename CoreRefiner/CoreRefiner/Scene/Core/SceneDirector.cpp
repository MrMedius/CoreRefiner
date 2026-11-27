#include "SceneDirector.h"

void SceneDirector::Adopt(SceneId id, std::unique_ptr<IScene> scene)
{
	if (id >= SceneId::Count || scene == nullptr)
	{
		return;
	}
	auto& slot = bases_[static_cast<std::size_t>(id)];
	if (slot != nullptr)
	{
		return;
	}
	slot = std::move(scene);
}

void SceneDirector::RequestScene(SceneId id)
{
	if (id >= SceneId::Count)
	{
		return;
	}
	if (id == current_ && !pending_.has_value())
	{
		return;
	}
	pending_ = id;
	if (current_ == SceneId::Count)
	{
		ApplyPending_();
	}
}

void SceneDirector::Update(float dt)
{
	if (IScene* scene = Base_(current_))
	{
		scene->Update(dt);
	}
	ApplyPending_();
}

void SceneDirector::Submit()
{
	if (IScene* scene = Base_(current_))
	{
		scene->Submit();
	}
}

IScene* SceneDirector::Base_(SceneId id) noexcept
{
	if (id >= SceneId::Count)
	{
		return nullptr;
	}
	return bases_[static_cast<std::size_t>(id)].get();
}

void SceneDirector::ApplyPending_()
{
	if (!pending_.has_value())
	{
		return;
	}
	const SceneId next = *pending_;
	pending_.reset();
	if (next >= SceneId::Count)
	{
		return;
	}
	if (next == current_)
	{
		return;
	}

	if (IScene* leaving = Base_(current_))
	{
		leaving->OnLeave();
	}
	current_ = next;
	if (IScene* entering = Base_(current_))
	{
		entering->OnEnter();
	}
}
