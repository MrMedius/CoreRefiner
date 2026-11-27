#include "Time_PS.hlsli"

// 时间在 b1。这份参数在 b2，一共 48 字节。
// innerColor 是内沿。rgb 是颜色，a 是这一沿自己的透明度。a 为 0 表示这一沿是空的。
// outerColor 是外沿，规则相同。
// opacity 是整只环的播放透明度。淡出只乘它，不改两边的 a。
// innerWave 内圈波浪开关。0 关，1 开。
// outerWave 外圈波浪开关。0 关，1 开。
// amplitude 波浪幅度系数。开关关掉时不生效。
// 后四个 float 按常量缓冲对齐，占一份 float4。
cbuffer EffectRing : register(b2)
{
    float4 innerColor;
    float4 outerColor;
    float opacity;
    float innerWave;
    float outerWave;
    float amplitude;
};

// 内径相对外径的比例。必须和 Ring::Make 的默认值 0.75 一致。
static const float kInnerRatio = 0.75f;
// 绕环一圈有多少个波峰。
static const float kWaveFrequency = 8.0f;
// 波浪沿圆周流动的速度，单位弧度每秒。
static const float kWaveSpeed = 4.0f;

// modelPos 是模型空间位置，不含物体的放大。
float4 main(float3 modelPos : TEXCOORD0) : SV_Target
{
    // radius 是到圆心的水平距离。外沿是 1，内沿是 kInnerRatio。
    float radius = length(modelPos.xz);
    // band 把环带收成 0 到 1。0 是内沿，1 是外沿。
    float span = 1.0f - kInnerRatio;
    float band = saturate((radius - kInnerRatio) / span);
    // angle 是绕 +Y 的角度。波浪沿圆周走。
    float angle = atan2(modelPos.z, modelPos.x);

    // 两边都有颜色时，颜色和透明度一起从内沿插到外沿。
    float3 rgb = lerp(innerColor.rgb, outerColor.rgb, band);
    float spatialAlpha = lerp(innerColor.a, outerColor.a, band);
    // 某一沿 a 为 0 时那一沿是空的。颜色只用有色的那一沿，透明度仍从空淡到有色。
    if (innerColor.a <= 0.0f && outerColor.a > 0.0f)
    {
        rgb = outerColor.rgb;
    }
    else if (outerColor.a <= 0.0f && innerColor.a > 0.0f)
    {
        rgb = innerColor.rgb;
    }

    // 内圈波浪靠近内沿，外圈波浪靠近外沿。两者绕环的方向相反。
    float innerRipple = sin(angle * kWaveFrequency + totalTime * kWaveSpeed);
    float outerRipple = sin(angle * kWaveFrequency - totalTime * kWaveSpeed);
    float wave = saturate(innerWave) * innerRipple * (1.0f - band)
        + saturate(outerWave) * outerRipple * band;

    // alpha 是这一像素的透明度。幅度系数只推动波浪，再乘整只环的播放透明度。
    float alpha = saturate(spatialAlpha + wave * amplitude) * opacity;
    return float4(rgb, alpha);
}
