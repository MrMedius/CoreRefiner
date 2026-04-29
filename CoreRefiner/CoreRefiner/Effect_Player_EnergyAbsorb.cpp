#include "Effect_Player_EnergyAbsorb.h"
#include "ObjectCodex.h"
#include "Player.h"

void Effect_Player_EnergyAbsorb::Update(float dt)
{
    bool anyActive = false;

    for (auto& p : particles)
    {
        if (!p.active) continue;
        anyActive = true;

        // progress update (accelerate near end)
        const float accel = 1.0f + p.t;
        p.t += p.speed * dt * accel;

        if (p.t >= 1.0f)
        {
            p.t = 1.0f;
            p.active = false;

            if (auto* pl = ObjectCodex::FindFirstActiveObjectByTag<Player>(character_Player))
                pl->GetResourceBars()->SetEnergyChangedAt(effectType - 1, true);
            continue;
        }

        const XMFLOAT2 cur = Bezier2(p.p0, p.p1, p.p2, p.t);

        // scale animation similar to reference
        float scaleFactor = std::sinf(p.t * PI) + 0.5f;
        if (p.t > 0.9f)
        {
            const float shrinkT = (p.t - 0.8f) / 0.2f;
            scaleFactor *= (1.0f - shrinkT);
        }
        float s = p.baseScale * scaleFactor;
        if (s < 2.0f) s = 2.0f;

        // store trail point (distance threshold)
        if (!p.trails.empty())
        {
            const XMFLOAT2 last = p.trails.front().pos;
            const float dx = cur.x - last.x;
            const float dy = cur.y - last.y;
            const float distSq = dx * dx + dy * dy;

            if (distSq > kTrailRecordDistSq)
            {
                TrailPoint tp{};
                tp.pos = cur;
                tp.scale = s; // store current visual scale (looks better)
                tp.opacity = 1.0f;
                p.trails.push_front(tp);
            }
        }
        else
        {
            TrailPoint tp{};
            tp.pos = cur;
            tp.scale = s;
            tp.opacity = 1.0f;
            p.trails.push_front(tp);
        }

        if ((int)p.trails.size() > kMaxTrailPerParticle)
            p.trails.pop_back();

        // update head sprite
        auto* spr = sprites[p.spriteIdx].get();
        spr->SetPosition(cur.x, cur.y);
        spr->SetScale(s, s);
    }

    if (!anyActive)
        Deactivate();
}
