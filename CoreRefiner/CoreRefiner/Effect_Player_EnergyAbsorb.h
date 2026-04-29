#pragma once
#include "Effect.h"
#include "RenderGraph.h"
#include "Sprite2D.h"
#include <deque>
#include <vector>
#include <random>
#include <algorithm>
#include "Channels.h"

class Effect_Player_EnergyAbsorb : public Effect
{
public:
    Effect_Player_EnergyAbsorb(Graphics& gfxIn, Rgph::RenderGraph& rg, XMFLOAT3 position, int type, Object_Type_Tag tag = effect_Player_EnergyAbsorb)
        :
        Effect(tag),
        gfx(gfxIn)
    {
        // parameters init
        SetCollisionOnOff(false);

        // end points init
        float posX = SCREEN_WIDTH - 360.0f;
        float posY = 550.0f;
        endPoints[1] = { posX + 0.0f * 140.0f, posY };
        endPoints[2] = { posX + 1.0f * 140.0f, posY };
        endPoints[3] = { posX + 2.0f * 140.0f, posY };

        // graphics init
        sprites.reserve(kMaxParticles + kMaxTrailSprites);
        particles.reserve(kMaxParticles);
        // create sprites for head particles and trail sprites
        for (int i = 0; i < kMaxParticles + kMaxTrailSprites; ++i)
        {
            auto sp = std::make_unique<Sprite2D>(gfx, std::vector<std::string>{ "asset\\Images\\Player\\Player_EnergyAbsorb.png" });
            sp->SetScale(16.0f, 16.0f);
            sp->LinkTechniques(rg);
            sprites.push_back(std::move(sp));

            if (i < kMaxParticles) particles.push_back(Particle{});
        }
    }

    void SpawnWithType(XMFLOAT3 pos, int type, int count)
    {
        effectType = std::clamp(type, 1, 3);
        particleCnt = std::clamp(count, 1, kMaxParticles);
        SpawnAt(pos);
    }

    void SpawnAt(XMFLOAT3 pos, bool flip = false) override
    {
        XMFLOAT2 sp{};
        if (!WorldToScreenPx(pos, sp))
        {
            Deactivate();
            return;
        }

        static thread_local std::mt19937 rng{ std::random_device{}() };
        std::uniform_real_distribution<float> distSpeed(0.5f, 1.0f);
        std::uniform_real_distribution<float> distScale(15.0f, 20.0f);
        std::uniform_real_distribution<float> distCtrl(-350.0f, 350.0f);
        std::uniform_real_distribution<float> distStart(-40.0f, 40.0f);

        // clear all particles
        for (auto& p : particles)
        {
            p.active = false;
            p.trails.clear();
        }

        // spawn particleCnt particles using first particleCnt sprites as heads
        for (int i = 0; i < particleCnt; ++i)
        {
            auto& p = particles[i];
            p.active = true;
            p.type = effectType;
            p.spriteIdx = i;

            // start point
            p.p0 = { sp.x + distStart(rng), sp.y + distStart(rng) };
            // end point
            p.p2 = endPoints[effectType];
            // control point
            const float midX = (p.p0.x + p.p2.x) * 0.5f;
            const float midY = (p.p0.y + p.p2.y) * 0.5f;
            p.p1 = { midX + distCtrl(rng), midY + distCtrl(rng) - 150.0f };

            // other parameters
            p.t = 0.0f;
            p.speed = distSpeed(rng);
            p.baseScale = distScale(rng);

            // init sprite head
            auto* s = sprites[p.spriteIdx].get();
            s->SetFrame(3, 1, effectType, 1, 1, 0, false);

            const XMFLOAT2 cur = Bezier2(p.p0, p.p1, p.p2, 0.0f);
            s->SetPosition(cur.x, cur.y);
            s->SetScale(p.baseScale, p.baseScale);

            // init trails (store first point)
            TrailPoint tp{};
            tp.pos = cur;
            tp.scale = p.baseScale;
            tp.opacity = 1.0f;
            p.trails.push_front(tp);
        }
    }

    void OnEnable(void) override {}
    void Update(float dt) override;

    void Submit(void) override
    {
        // trail sprites are after kMaxParticles
        int trailUsed = 0;

        // 1) draw trails
        for (const auto& p : particles)
        {
            if (!p.active) continue;
            if (p.trails.size() < 2) continue;

            // draw each segment
            for (size_t i = 0; i + 1 < p.trails.size(); ++i)
            {
                const auto& tp1 = p.trails[i];
                const auto& tp2 = p.trails[i + 1];

                const float dx = tp1.pos.x - tp2.pos.x;
                const float dy = tp1.pos.y - tp2.pos.y;
                const float dist = std::sqrtf(dx * dx + dy * dy);

                int steps = (int)(dist / kTrailStep);
                if (steps < 1) steps = 1;

                const float ratio1 = (float)i / (float)(p.trails.size() - 1);
                const float ratio2 = (float)(i + 1) / (float)(p.trails.size() - 1);

                for (int j = 0; j < steps; ++j)
                {
                    if (trailUsed >= kMaxTrailSprites) break;

                    const float t = (float)j / (float)steps;

                    XMFLOAT2 drawPos{};
                    drawPos.x = tp1.pos.x * (1.0f - t) + tp2.pos.x * t;
                    drawPos.y = tp1.pos.y * (1.0f - t) + tp2.pos.y * t;

                    const float globalRatio = ratio1 * (1.0f - t) + ratio2 * t;
                    float sizeFactor = 1.0f - globalRatio;

                    // shrink near end of animation
                    float animSizeFactor = 1.0f;
                    if (p.t > 0.9f)
                    {
                        const float shrinkT = (p.t - 0.8f) / 0.2f;
                        animSizeFactor = (1.0f - shrinkT);
                    }

                    float finalSize = tp1.scale * sizeFactor * animSizeFactor;
                    if (finalSize < 1.0f) finalSize = 1.0f;

                    // use trail sprite
                    auto* s = sprites[kMaxParticles + trailUsed].get();
                    ++trailUsed;

                    s->SetFrame(3, 1, p.type, 1, 1, 0, false);
                    s->SetPosition(drawPos.x, drawPos.y);
                    s->SetScale(finalSize, finalSize);
                    s->Submit(Chan::ui);
                }

                if (trailUsed >= kMaxTrailSprites) break;
            }
        }

        // 2) draw head particles
        for (const auto& p : particles)
        {
            if (!p.active) continue;
            sprites[p.spriteIdx]->Submit(Chan::ui);
        }
    }

    void OnCollide(Character* other) override {}

private:
    struct TrailPoint
    {
        XMFLOAT2 pos;
        float scale;
        float opacity;
    };

    struct Particle
    {
        XMFLOAT2 p0{}, p1{}, p2{};
        float t{ 0.0f };
        float speed{ 0.0f };
        float baseScale{ 15.0f };
        int type{ 1 }; // 1..3
        bool active{ false };
        int spriteIdx{ -1 };

        std::deque<TrailPoint> trails;
    };

    static XMFLOAT2 Bezier2(const XMFLOAT2& p0, const XMFLOAT2& p1, const XMFLOAT2& p2, float t)
    {
        const float u = 1.0f - t;
        const float tt = t * t;
        const float uu = u * u;
        return {
            uu * p0.x + 2.0f * u * t * p1.x + tt * p2.x,
            uu * p0.y + 2.0f * u * t * p1.y + tt * p2.y
        };
    }

    bool WorldToScreenPx(const XMFLOAT3& worldPos, XMFLOAT2& outPx) const
    {
        using namespace DirectX;

        XMVECTOR v = XMVectorSet(worldPos.x, worldPos.y, worldPos.z, 1.0f);
        v = XMVector3TransformCoord(v, gfx.GetCamera());
        v = XMVector3TransformCoord(v, gfx.GetProjection());

        XMFLOAT3 ndc{};
        XMStoreFloat3(&ndc, v);

        if (ndc.z < 0.0f || ndc.z > 1.0f) return false;

        const float sx = (ndc.x + 1.0f) * 0.5f * (float)SCREEN_WIDTH;
        const float sy = (1.0f - ndc.y) * 0.5f * (float)SCREEN_HEIGHT;

        outPx = { sx, sy };
        return true;
    }

private:
    Graphics& gfx;

    // sprites: [0..kMaxParticles-1] = head, [kMaxParticles..] = trail
    std::vector<std::unique_ptr<Sprite2D>> sprites;
    std::vector<Particle> particles;

    // runtime params
    int effectType{ 1 };
    int particleCnt{ 1 };

    // points
    XMFLOAT2 endPoints[4]{};

    // trail params
    static constexpr int kMaxParticles = 5;
    static constexpr int kMaxTrailPerParticle = 10;
    static constexpr float kTrailRecordDistSq = 4.0f; // (2px)^2
    static constexpr float kTrailStep = 2.0f;         // 2px/step
    static constexpr int kMaxTrailSprites = kMaxParticles * kMaxTrailPerParticle;
};
