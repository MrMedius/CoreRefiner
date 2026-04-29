#pragma once
#include "Environment.h"
#include "RenderGraph.h"
#include "CubeTiledTutorial.h"
#include "Channels.h"
#include "Player.h"

class BlockTiledTutorial : public Environment
{
public:
    BlockTiledTutorial(Graphics& gfx, Rgph::RenderGraph& rg,
        XMFLOAT3 position, XMFLOAT3 size, XMFLOAT2 numTiled,
        bool onCollision,
        Object_Type_Tag tag = environment_BlockTiledTutorial)
        : Environment(tag)
    {
        pPlayer = ObjectCodex::FindFirstActiveObjectByTag<Player>(character_Player);

        SetPosition(position);
        SetSize(size);
        SetCollisionSize(size);
        SetCollisionOnOff(onCollision);
        m_originalCollisionSize = size;

        visualPre = std::make_unique<CubeTiledTutorial>(gfx, size, numTiled);
        visualPre->LinkTechniques(rg);
        visualPre->SetPosition(transInfo.position);

        DirectX::XMFLOAT3 localHalf{ 0.5f,0.5f,0.5f };
        boxCollider = BoxCollider::BuildFromWorldMatrix(transInfo.GetWorldMatrix(), localHalf);
    }

    void OnEnable() override 
    {
        visualPre->Reset();
        SetCollisionOnOff(true);
    }

    void Update(float dt) override
    {
        if (GameStatsCodex::GetIsLearnt() && !visualPre->IsCollapsing())
        {
            visualPre->StartCollapse();
        }
        if (visualPre->IsInsideCollapseHole(pPlayer->GetPosition()) && visualPre->IsCollapsing())
        {
            Deactivate();
            SetCollisionOnOff(false);
        }
        if (visualPre->IsCollapseFinished())
        {
            Deactivate();
        }

        visualPre->Update(dt);
    }

    void Submit() override
    {
        visualPre->Submit(Chan::main);
    }

    void OnCollide(Character*) override {}

private:
    std::unique_ptr<CubeTiledTutorial> visualPre;
    Player* pPlayer;
    XMFLOAT3   m_originalCollisionSize{};
};