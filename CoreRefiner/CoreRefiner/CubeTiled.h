#pragma once
#include "Drawable.h"
#include "Bindable.h"
#include "Transformation.h"
#include "Provides.h"

using namespace DirectX;

class CubeTiled : public Drawable, public IProvides<FieldTransitionTag>
{
public:
	CubeTiled(Graphics& gfx, XMFLOAT3 size, XMFLOAT2 numTiled);
	void SetPosition(XMFLOAT3 pos) noexcept;
	void SetRotation(float roll, float pitch, float yaw) noexcept;
	void SetScale(XMFLOAT3 size) noexcept;
	XMMATRIX GetTransformXM() const noexcept override;
    // transition related
    void Update(float dt, const XMFLOAT3& centerWorld);
    void StartExpand(const XMFLOAT3& centerWorld, int mode);
    void StartRecover(int mode);
    void FinishRecover();
    int GetPlayMode() { return FT.mode; }
	void GameStartExpend();
    FieldTransitionTag::value_type Provide(FieldTransitionTag) const noexcept override 
    {
        return {
            static_cast<int>(FT.state),FT.mode,
            FT.radius,FT.softness,FT.center,FT.opacity,
            FT.colorFrom,0.0f,FT.colorTo,0.0f,
            FT.noiseScale,FT.noiseAmp,{0.0f,0.0f}
        };
    }
private:
	Transformation trans;
    // transition related
    enum class FieldTransitionState : int
    {
        None = 0,
        Expand = 1,
        Recover = 2,
    };
    struct FieldTransition
    {
        // core
        FieldTransitionState state = FieldTransitionState::None;
        int mode = 0;
        // animation control
        XMFLOAT3 center = { 0,0,0 };
        float duration = 2.0f;           // total transition time (second)
        float interval = 1.0f / duration; // every step of transition progress
        float t = 0.0f;                   // transition progress
        float maxRadius = 20.0f;          // each increase in radius
        float effectRange = 200.0f;
        // shader params
        float radius = 0.0f;
        float softness = 5.0f;
        float opacity = 0.5f;
        XMFLOAT3 colorBase = { 1.0f,1.0f,1.0f };
        XMFLOAT3 colorFrom = colorBase;
        XMFLOAT3 colorTo = colorBase;
        // noise
        float noiseScale = 0.1f;
        float noiseAmp = 200.0f;
    } FT;
};