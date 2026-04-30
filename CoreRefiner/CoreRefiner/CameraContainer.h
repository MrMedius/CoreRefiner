#pragma once
#include "Window.h"
#include <vector>
#include <memory>

class Camera;
class Graphics;
namespace Rgph
{
	class RenderGraph;
}

class CameraContainer
{
public:
	CameraContainer(Graphics& gfx);
	void Bind( Graphics& gfx );
	void AddCamera( std::shared_ptr<Camera> pCam );
	Camera* operator->();
	~CameraContainer();
	void Update(float dt, DirectX::XMFLOAT3 pos, Window* wnd) noexcept;
	void Submit(size_t channels) const;
	void Reset();
	void SpawnWindow(Graphics& gfx);
	void LinkTechniques(Rgph::RenderGraph& rg);
	Camera& GetActiveCamera();
private:
	Camera& GetControlledCamera();
private:
	std::vector<std::shared_ptr<Camera>> cameras;
	int active = 0;
	int controlled = 0;

	// Camera Follow
	float FollowRadius = 50.0f;
	float FollowPitch = DirectX::XM_PI / 4.0f;
	float FollowLerp = 0.1f;
	DirectX::XMFLOAT3 FollowTarget{ 0.0f,40.0f,0.0f };




	// Screen Effects
public:
	// 画面振動をセットする
	void SetScreenShake(int frames, float minRange, float maxRange);
	// フレーム一時停止をセットする
	void SetScreenFroze(int frames);
	// フレーム一時停止をゲットする
	bool GetScreenFroze(void) const { return FrozeScreen; }
private:
	void ScreenShake(void);
	void ScreenFroze(void);
private:
	// Camera Shake
	int ShakeFrames{ 0 };
	float ShakeMinRange{ 0.0f };
	float ShakeMaxRange{ 0.0f };

	// Frame Freeze
	int FrozeFrames{ 0 };
	bool FrozeScreen{ false };
};