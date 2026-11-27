#pragma once

#include "IScene.h"

#include <array>
#include <memory>
#include <optional>

class SceneDirector
{
public:
	SceneDirector() = default;
	~SceneDirector() = default;

	SceneDirector(const SceneDirector&) = delete;
	SceneDirector& operator=(const SceneDirector&) = delete;

	void Adopt(SceneId id, std::unique_ptr<IScene> scene);

	void RequestScene(SceneId id);

	void Update(float dt);
	void Submit();

	[[nodiscard]] SceneId GetCurrent() const noexcept { return current_; }

private:
	[[nodiscard]] IScene* Base_(SceneId id) noexcept;
	void ApplyPending_();

	std::array<std::unique_ptr<IScene>, static_cast<std::size_t>(SceneId::Count)> bases_{};

	SceneId current_{ SceneId::Count };
	std::optional<SceneId> pending_{};
};
