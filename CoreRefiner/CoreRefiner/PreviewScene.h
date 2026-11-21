#pragma once
#include "IScene.h"
#include "CameraContainer.h"
#include "PointLight.h"

#include <memory>

class Ball;
class Ring_Yang;
class Ring_Yin;
class SceneDirector;
class Window;

namespace Rgph
{
	class GameRenderGraph;
}

// 不进战斗的着色器预览。球停在原点，镜头和战斗一样用 F2 自由看。
class PreviewScene : public IScene
{
public:
	PreviewScene(Window& wnd, SceneDirector& director, Rgph::GameRenderGraph& gameRG);
	~PreviewScene() override;

	PreviewScene(const PreviewScene&) = delete;
	PreviewScene& operator=(const PreviewScene&) = delete;

	void OnEnter() override;
	void OnLeave() override;
	void Update(float dt) override;
	void Submit() override;

private:
	Window& wnd_;
	// 场景切换。Esc 用它回到标题。
	SceneDirector& director_;
	// 和战斗共用的局内渲染图。Attack 着色器画在这张图上。
	Rgph::GameRenderGraph& gameRG_;
	// 预览自己的灯。不替换战斗已经绑上的阴影摄像机。
	PointLight light_;
	// 和战斗同一套摄像机。主摄像机看原点，Observer 给 F2 自由移动。
	CameraContainer cameras_;
	// 停在原点的测试球。不调用 Update，所以不飞行、不计寿命。
	std::unique_ptr<Ball> ball_;
	// 右侧的阴环。播完会再放一次。
	std::unique_ptr<Ring_Yin> yin_;
	// 左侧的阳攻击环。播完会再放一次。
	std::unique_ptr<Ring_Yang> yang_;
};
