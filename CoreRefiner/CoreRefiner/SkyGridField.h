#pragma once
#include "Environment.h"
#include "RenderGraph.h"
#include "SkyGridCylinder.h"
#include "Channels.h"

class SkyGridField : public Environment
{
public:
    SkyGridField(Graphics& gfx, Rgph::RenderGraph& rg, Object_Type_Tag tag = environment_SkyGridField)
        :
        Environment(tag)
    {
        SetCollisionOnOff(false);

        visualPre = std::make_unique<SkyGridCylinder>(gfx);
        visualPre->LinkTechniques(rg);

        SetPosition(XMFLOAT3{ 0.0f, 30.0f, 0.0f });
    }

    void OnEnable(void) override 
    {
        visualPre->Reset();
    }

    void Update(float dt) override
    {
        if (GameStatsCodex::GetIsLearnt()) visualPre->StartFadeOut();

        if (visualPre->IsFadeOutFinished()) Deactivate();

        visualPre->Update(dt);
    }

    void Submit(void) override
    {
        visualPre->Submit(Chan::main);
    }

    void OnCollide(Character* other) override {}

private:
    std::unique_ptr<SkyGridCylinder> visualPre;
};