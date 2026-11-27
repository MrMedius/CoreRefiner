float rand(float2 co)
{
    return frac(sin(dot(co, float2(12.9898, 78.233))) * 43758.5453);
}

float2 safe_mod(float2 x, float2 y)
{
    return x - y * floor(x / y);
}