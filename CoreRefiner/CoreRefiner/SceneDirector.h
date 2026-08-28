#pragma once

#include "IScene.h"

#include <array>
#include <memory>
#include <optional>

class Window;

namespace Rgph
{
	class GameRenderGraph;
	class UserInterfaceRenderGraph;
}

/**
 * @brief 三个互斥底座（Title / Game / Result）。只对当前底座调 Update / Submit。
 * @note 切底座在本帧当前场景 Update 之后生效，避免在场景 Update 中途 OnLeave。
 * @note 三个场景指针在后续步骤于构造函数里 make_unique；本步只搭调度。
 */
class SceneDirector
{
public:
	SceneDirector(Window& wnd, Rgph::GameRenderGraph& gameRG, Rgph::UserInterfaceRenderGraph& uiRG);
	~SceneDirector() = default;

	SceneDirector(const SceneDirector&) = delete;
	SceneDirector& operator=(const SceneDirector&) = delete;

	/** @brief 请求切换底座。帧内当前场景 Update 结束后再 OnLeave / OnEnter。 */
	void RequestScene(SceneId id);

	void Update(float dt);
	void Submit();

	[[nodiscard]] SceneId GetCurrent() const noexcept { return current_; }

private:
	[[nodiscard]] IScene* Base_(SceneId id) noexcept;
	void ApplyPending_();

	Window& wnd_;
	Rgph::GameRenderGraph& gameRG_;
	Rgph::UserInterfaceRenderGraph& uiRG_;

	std::array<std::unique_ptr<IScene>, static_cast<std::size_t>(SceneId::Count)> bases_{};

	SceneId current_{ SceneId::Count };
	std::optional<SceneId> pending_{};
};
