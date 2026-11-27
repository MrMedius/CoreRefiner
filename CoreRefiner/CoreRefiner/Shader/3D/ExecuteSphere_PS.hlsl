// viewPos 是表面点，viewNormal 是朝外的法线。都在视线空间。
// kEdgePower 越大，中间越透，深色越贴着轮廓。
static const float kEdgePower = 0.5;

float4 main(float3 viewPos : TEXCOORD0, float3 viewNormal : TEXCOORD1) : SV_Target
{
    float3 normal = normalize(viewNormal);
    float3 toCamera = normalize(-viewPos);
    float facing = saturate(dot(normal, toCamera));
    float alpha = pow(1.0 - facing, kEdgePower);
    return float4(0.0, 0.0, 0.0, alpha);
}
