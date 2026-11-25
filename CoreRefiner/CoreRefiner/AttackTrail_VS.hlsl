#include "Transform.hlsli"
#include "Time_VS.hlsli"

// camRight、camUp 用来求视线。count 是这一帧的颗数。travel 是这一发已经飞过的距离。
// hostPos 是宿主当前位置。direction 是飞行方向，身后是它的反方向。
// 时间用 Time_VS.hlsli 的 totalTime，寄存器是 b3。
cbuffer TrailState : register(b1)
{
    float3 camRight;
    float count;
    float3 camUp;
    float travel;
    float3 hostPos;
    float pad1;
    float3 direction;
    float pad2;
};

// 每颗走完这一段时间就回到宿主身边重新开始。
static const float kLife = 0.8;
// 最老的一颗落在宿主身后这么远。
static const float kTrail = 7.5;
// 刚出现时的整边长，和消失前的整边长。
static const float kSize0 = 1.0;
static const float kSize1 = 0.1;
// 刚出现时的透明度。
static const float kAlpha0 = 0.85;
// 侧向散开和上抬的最大距离。
static const float kSpread = 1.6;
static const float kLift = 1.1;

// 喷出、沿飞行反方向拉开、末端缩小变淡。
static const uint kStageBurst = 1;
static const uint kStageStretch = 1;
static const uint kStageFade = 2;

struct VSOut
{
    float2 tc : Texcoord;
    float fade : Fade;
    float4 pos : SV_Position;
};

// n 是这颗的编号。同一编号每帧得到同一个 0 到 1 的数。
float Hash(float n)
{
    return frac(sin(n) * 43758.5453);
}

// age 是 0 到 1 的寿命。不到 0.25 喷出，不到 0.7 拉开，剩下的变淡。
uint StageOf(float age)
{
    if (age < 0.25)
    {
        return kStageBurst;
    }
    if (age < 0.7)
    {
        return kStageStretch;
    }
    return kStageFade;
}

// corner 是共用四边形的角。id 是这一颗的编号。
VSOut main(float3 corner : Position, float2 tc : Texcoord, uint id : SV_InstanceID)
{
    const float safeCount = max(count, 1.0);
    // 0 是刚离开宿主，1 是这颗寿命的尽头。颗数把它们错开。
    const float age = frac(totalTime / kLife - (float)id / safeCount);
    const float sideRand = Hash((float)id + 1.0) * 2.0 - 1.0;
    const float liftRand = Hash((float)id + 17.0);
    const float sizeScale = lerp(0.75, 1.4, Hash((float)id + 31.0));

    float along = 0.0;
    float sideAmt = 0.0;
    float liftAmt = 0.0;
    float size = kSize0;
    float fade = kAlpha0;
    const uint stage = StageOf(age);
    switch (stage)
    {
        case 0:
        {
            // 从宿主身边张到侧向随机位置。
            const float t = age / 0.25;
            along = t * kTrail * 0.25;
            sideAmt = sideRand * kSpread * t;
            liftAmt = liftRand * kLift * t;
            size = kSize0 * sizeScale;
            fade = kAlpha0;
            break;
        }
        case 1:
        {
            // 侧向保持，沿飞行反方向继续拉开。
            const float t = (age - 0.25) / 0.45;
            along = lerp(kTrail * 0.25, kTrail * 0.7, t);
            sideAmt = sideRand * kSpread;
            liftAmt = liftRand * kLift;
            size = kSize0 * sizeScale;
            fade = kAlpha0;
            break;
        }
        default:
        {
            // 拉到尽头，同时缩小变淡。
            const float t = saturate((age - 0.7) / 0.3);
            along = lerp(kTrail * 0.7, kTrail, t);
            sideAmt = sideRand * kSpread;
            liftAmt = liftRand * kLift;
            size = lerp(kSize0, kSize1, t) * sizeScale;
            fade = kAlpha0 * (1.0 - t);
            break;
        }
    }

    float3 side = cross(direction, float3(0.0, 1.0, 0.0));
    if (dot(side, side) < 1e-4)
    {
        side = cross(direction, float3(1.0, 0.0, 0.0));
    }
    side = normalize(side);
    const float3 lift = normalize(cross(side, direction));
    // 飞过的距离还没到这一颗的位置时，先不画。拖尾随移动变长。
    if (along > travel)
    {
        fade = 0.0;
    }
    const float3 center = hostPos - direction * along + side * sideAmt + lift * liftAmt;

    // 尖角所在的上下轴转到飞行方向。左右轴让平面仍朝向镜头。
    const float3 look = cross(camRight, camUp);
    float3 width = cross(look, direction);
    if (dot(width, width) < 1e-4)
    {
        width = cross(camUp, direction);
    }
    width = normalize(width);
    const float3 world = center + direction * (corner.y * size) + width * (corner.x * size);

    VSOut output;
    output.pos = mul(float4(world, 1.0f), modelViewProj);
    output.tc = float2(tc.x, 1.0f - tc.y);
    output.fade = fade;
    return output;
}
