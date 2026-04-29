#pragma once
#include "Drawable.h"
#include "Provides.h"
#include <DirectXMath.h>

using namespace DirectX;

class Skybox : public Drawable, public IProvides<SkyboxTransitionTag>
{
public:
    Skybox(Graphics& gfx, const std::vector<std::string>& paths);

    DirectX::XMMATRIX GetTransformXM() const noexcept override;

    // transition control
    void Update(float dt);
    void StartExpand(int mode);
    void StartRecover();
    void FinishRecover();
    void ResetMode();

    int GetPlayMode() const { return mode; }

    SkyboxTransitionTag::value_type Provide(SkyboxTransitionTag) const noexcept override;
private:
    enum class State : int
    {
        None = 0,
        Expand = 1,
        Recover = 2,
    };

private:
    // core
    State state = State::None;
    int mode = 0;
    // animation control
    int fromIdx = 0; // cubemap index A
    int toIdx = 0;   // cubemap index B
    float t = 0.0f;  // 0..1 blend
    float duration = 2.0f;
    float interval = 1.0f / duration;
    // radial transition
    XMFLOAT3 center = { 0.0f, 0.0f, 0.0f };
    float radius = 0.0f;
    float maxRadius = 200.0f;
    float softness = 20.0f;
    // noise
    float noiseScale = 0.05f;
    float noiseAmp = 200.0f;
};