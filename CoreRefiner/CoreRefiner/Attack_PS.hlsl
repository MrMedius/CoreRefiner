#include "Time_PS.hlsli"

// count 是这次上传的条数。
// opacity 是整颗球外层的透明度。1 不透明。
// padding 补齐到 16 字节，不参与着色。
// ids 按写入顺序存放颜色编号，最多 256 个。每 4 个编成一个 uint4。
cbuffer AttackShade : register(b2)
{
    uint count;
    float opacity;
    float2 padding;
    uint4 ids[64];
};

// 颜色表。0 灰，1 白，2 黑。
static const float3 kShadeColors[3] =
{
    float3(0.5f, 0.5f, 0.5f),
    float3(1.0f, 1.0f, 1.0f),
    float3(0.0f, 0.0f, 0.0f),
};

// 沿模型 +Z 弯出波纹的圈数。
static const float kWaveFrequency = 6.0f;
// 波纹把条纹边界推开的幅度，单位弧度。
static const float kWaveAmplitude = 0.2f;
// 波纹沿模型 +Z 流动的速度，单位弧度每秒。
static const float kTwistSpeed = 30.0f;
// 条纹绕模型 +Z 转的速度，单位圈每秒。
static const float kTurnsPerSecond = 1.0f;
// 外层变淡时，和黑球相同。越大中间越透，实色越贴着轮廓。
static const float kEdgePower = 0.5f;

// 按颜色编号取颜色。不是 1 或 2 时用灰色。
float3 ColorOf(uint colorId)
{
    float3 color = kShadeColors[0];
    if (colorId == 1u)
    {
        color = kShadeColors[1];
    }
    else if (colorId == 2u)
    {
        color = kShadeColors[2];
    }
    return color;
}

// index 是条纹序号。从 ids 里取出对应的颜色编号。
uint IdAt(uint index)
{
    uint4 pack = ids[index >> 2];
    uint lane = index & 3u;
    uint colorId = pack.x;
    if (lane == 1u)
    {
        colorId = pack.y;
    }
    else if (lane == 2u)
    {
        colorId = pack.z;
    }
    else if (lane == 3u)
    {
        colorId = pack.w;
    }
    return colorId;
}

// modelPos 是模型空间位置。没有条纹时保持灰色。
// 绕模型 +Z 等分并随时间转动，边界再沿 +Z 起波。
float3 ShadeColor(float3 modelPos)
{
    float3 color;
    
    if (count == 0u)
    {
        color = kShadeColors[0];
    }
    else if (count == 1)
    {
        color = ColorOf(IdAt(0u));
    }
    else
    {
        // angle 是绕 +Z 的角度。波浪沿 +Z 把边界推开，并随时间流动。
        float angle = atan2(modelPos.y, modelPos.x);
        angle += sin(modelPos.z * kWaveFrequency + totalTime * kTwistSpeed) * kWaveAmplitude;
        // turns 是落在哪一条上的圈数小数。
        float turns = frac(angle / 6.2831853f + totalTime * kTurnsPerSecond);
        uint slice = (uint)(turns * (float)count);
        if (slice >= count)
        {
            slice = count - 1u;
        }
        color = ColorOf(IdAt(slice));
    }
    
    return color;
}

float Transparency(float3 viewPos, float3 viewNormal)
{
    float alpha = 1.0f;
    if (opacity < 1.0f)
    {
        // facing 越接近 1 越正对镜头。正对处透，轮廓处实。
        float3 normal = normalize(viewNormal);
        float3 toCamera = normalize(-viewPos);
        float facing = saturate(dot(normal, toCamera));
        alpha = pow(1.0f - facing, kEdgePower);
    }
    return alpha;
}

// viewPos、viewNormal 在视线空间。opacity 小于 1 时按视线变淡，等于 1 时整颗不透明。
float4 main(float3 modelPos : TEXCOORD0, float3 viewPos : TEXCOORD1, float3 viewNormal : TEXCOORD2) : SV_Target
{
    return float4(ShadeColor(modelPos), Transparency(viewPos, viewNormal));
}
