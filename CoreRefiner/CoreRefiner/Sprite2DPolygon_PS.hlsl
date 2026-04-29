Texture2D tex : register(t0);
SamplerState splr : register(s0);

cbuffer RingParams : register(b0)
{
    float startAngleDeg;
    float endAngleDeg;
    float ratio;
};

static const float PI = 3.14159265359;
static const float TWO_PI = 2 * PI;

float NormalizeAngle(float a)
{
    a = fmod(a, TWO_PI);
    if (a < 0)
        a += TWO_PI;
    return a;
}

bool InArc(float angle, float start, float sweep)
{
    float d = NormalizeAngle(angle - start);
    return d <= sweep + 1e-6;
}

float4 main(float2 tc : Texcoord, float2 uv : TexcoordPure) : SV_Target
{
    float4 color = tex.Sample(splr, tc);
    if (color.a < 0.001)
        discard;

    float start = NormalizeAngle(TWO_PI * (startAngleDeg / 360.0));
    float end = NormalizeAngle(TWO_PI * (endAngleDeg / 360.0));

    // get the range
    float sweep = NormalizeAngle(end - start);
    float drawSweep = sweep * saturate(ratio);

    float2 p = uv - float2(0.5, 0.5);

    // get angle
    float angle = atan2(-p.x, p.y);
    angle = NormalizeAngle(angle);

    if (!InArc(angle, start, drawSweep))
        discard;

    return color;
}
