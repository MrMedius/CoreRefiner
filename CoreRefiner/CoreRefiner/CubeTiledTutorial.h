#pragma once
#include "Drawable.h"
#include "Transformation.h"
#include "Provides.h"
using namespace DirectX;

class CubeTiledTutorial : public Drawable, public IProvides<TutorialTransitionTag>
{
public:
    CubeTiledTutorial(Graphics& gfx, XMFLOAT3 size, XMFLOAT2 numTiled);
    void SetPosition(XMFLOAT3 pos) noexcept;
    void SetRotation(float roll, float pitch, float yaw) noexcept;
    void SetScale(XMFLOAT3 size) noexcept;
    XMMATRIX GetTransformXM() const noexcept override;

    // Collapse
    void Update(float dt);
    void StartCollapse(void);
    bool IsCollapseFinished() const { return m_collapseFinished; }
    bool IsCollapsing()       const { return m_isCollapsing; }
    bool IsInsideCollapseHole(XMFLOAT3 worldPos) const;
    float GetCurrentCollapseRadius() const { return m_currentCollapseRadiusWorld; }

    void Reset();

    TutorialTransitionTag::value_type Provide(TutorialTransitionTag) const noexcept override;

private:
    Transformation trans;

    TutorialTransitionTag::value_type FT{};

    // time count
    float m_gridTimeAccumulator = 0.0f;

    // collapse
    bool  m_isCollapsing = false;
    bool  m_collapseFinished = false;
    float m_collapseSpeed = 0.5f;
    float m_collapseElapsedTime = 0.0f;
    float m_collapseDuration = 180.0f;
    float m_totalTimeAccumulator = 0.0f;
    float m_maxVortexStrength = 0.3f;
    float m_maxRainbowIntensity = 4.0f;
    float m_currentCollapseRadiusWorld = 0.0f;
    XMFLOAT3 m_size{};
    XMFLOAT2 m_numTiled{};
};