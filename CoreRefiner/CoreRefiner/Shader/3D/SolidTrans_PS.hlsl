cbuffer CBuf : register(b1)
{
    float4 materialColor;
};

float4 main() : SV_Target
{
    return materialColor;
}
