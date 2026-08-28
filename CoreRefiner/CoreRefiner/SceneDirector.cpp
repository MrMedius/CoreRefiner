#include "SceneDirector.h"

SceneDirector::SceneDirector(Window& wnd, Rgph::GameRenderGraph& gameRG, Rgph::UserInterfaceRenderGraph& uiRG)
	:
	wnd_(wnd),
	gameRG_(gameRG),
	uiRG_(uiRG)
{
	// Title / Game / Result 在对应场景类落地后于此处构造。
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
