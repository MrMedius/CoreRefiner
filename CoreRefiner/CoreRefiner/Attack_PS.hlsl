// colorId 是颜色编号。0 灰，1 白，超出范围用灰。
// padding 补齐这一份常量缓冲，不参与着色。
cbuffer AttackShade : register(b1)
{
    uint colorId;
    float3 padding;
};

// 颜色表。下标就是 colorId。
static const float3 kShadeColors[2] =
{
    float3(0.5f, 0.5f, 0.5f),
    float3(1.0f, 1.0f, 1.0f),
};

// 按 colorId 取颜色。编号不是 1 时用灰色。
float3 ShadeColor()
{
    float3 color = kShadeColors[0];
    if (colorId == 1u)
    {
        color = kShadeColors[1];
    }
    return color;
}

float4 main() : SV_Target
{
    return float4(ShadeColor(), 1.0f);
}
