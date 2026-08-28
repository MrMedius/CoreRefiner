#pragma once

enum class SceneId : unsigned char
{
	Title,
	Game,
	Result,
	Count
};

class IScene
{
public:
	virtual ~IScene() = default;

	virtual void OnEnter() {}
	virtual void OnLeave() {}
	virtual void Update(float dt) = 0;
	virtual void Submit() = 0;
};
