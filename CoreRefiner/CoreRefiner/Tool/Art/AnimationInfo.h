#pragma once
// Animation Related
struct SpriteAnimeInfo
{
    unsigned int numU{ 1 };
    unsigned int numV{ 1 };
    unsigned int FrameStart{ 0 };
    unsigned int FrameTotalCount{ 0 };
    float FPS{ 30.0f };
};
struct SpriteAnimeManualAssistant
{
    unsigned int FrameNo{ 1 };
    float Accum{ 0.0f };
    float FrameTime{ 0.0f };

    float FPS{ 30.0f };
    bool Loop{ true };
    unsigned int FrameTotal{ 1 };

    void Reset() noexcept
    {
        FrameNo = 1;
        Accum = 0.0f;
        FrameTime = 1.0f / FPS;
    }
    void Update(float dt) noexcept
    {
        Accum += dt;

        while (Accum >= FrameTime)
        {
            Accum -= FrameTime;
            FrameNo++;
            if (FrameNo > FrameTotal && Loop) FrameNo = 1;
        }
    }
    bool Finished() const noexcept
    {
        return (!Loop) && (FrameNo >= FrameTotal);
    }
    int GetCurrentFrame(void)
    {
        return FrameNo;
    }
};
