// 球心和半径在世界空间。b1。centerWorld.xyz 是球心，radius 是当前半径。
// color 是球内保留的颜色。
cbuffer ModelClip : register(b1)
{
    float3 centerWorld;
    float radius;
    float4 color;
};

// worldPos 是这一像素的世界位置。球外丢掉，球内留下。
float4 main(float3 worldPos : TEXCOORD0) : SV_Target
{
    float dist = distance(worldPos, centerWorld);
    if (dist > radius)
    {
        discard;
    }
    return color;
}
