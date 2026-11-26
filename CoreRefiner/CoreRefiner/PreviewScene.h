#pragma once
#include "IScene.h"
#include "CameraContainer.h"
#include "PointLight.h"

#include <memory>

class Ball;
class Enemy_T_Shape;
class ExecuteSphere;
class RingYang;
class RingYin;
class RingLesserYang;
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
	std::unique_ptr<RingYin> yin_;
	// 左侧的阳攻击环。播完会再放一次。
	std::unique_ptr<RingYang> yang_;
	// 前方的黑球。播完会再放一次。
	std::unique_ptr<ExecuteSphere> sphere_;
	// 给黑球当来源的测试外形。不提交，不进战斗。
	std::unique_ptr<Enemy_T_Shape> victimSource_;
	// 前方黑球的少阳环。播完会再放一次。
	std::unique_ptr<RingLesserYang> lesserYang_;
};
