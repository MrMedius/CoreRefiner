// ScanWaveField PS — distance-field scan rings over Canvas texture (field background).
// 
// tc in [-0.5, 0.5] (canvas local); for a W×H field, world/local half-extent H/2 maps to 0.5.
// rings[i].xy = center in that UV space; rings[i].z = radius in the same units.
// thickness is UV-space half-width of the ring band (screen-stable when canvas scale is fixed).
// Cap = kMaxScanRings (8), aligned with ScanAssembler::kMaxSessions.

Texture2D tex : register(t0);
SamplerState splr : register(s0);

static const int kMaxScanRings = 8;

cbuffer ScanWaveFieldParamsCBuf : register(b0)
{
	float4 ringColor;
	float thickness;
	int ringCount;
	float aspect;
	float _pad0;
	// xy = center (UV), z = radius (UV), w unused
	float4 rings[kMaxScanRings];
};

float RingWeight(float dist, float radius, float thick)
{
	float d = abs(dist - radius);
	return saturate(1.0 - d / max(thick, 1e-5));
}

float4 main(float2 tc : Texcoord, float2 tc_Org : OriginalTexcoord) : SV_Target
{
	float4 bg = tex.Sample(splr, tc_Org);

	float2 p = tc * float2(aspect, 1.0);

	float ringSum = 0.0;
	const int n = clamp(ringCount, 0, kMaxScanRings);
	[loop]
	for (int i = 0; i < n; ++i)
	{
		float2 center = rings[i].xy * float2(aspect, 1.0);
		float radius = rings[i].z;
		float dist = length(p - center);
		ringSum = max(ringSum, RingWeight(dist, radius, thickness));
	}

	if (ringSum > 0.0)
	{
		float4 col = ringColor;
		col.a *= ringSum;
		if (col.a < 0.1)
			discard;
		return col;
	}

	if (bg.a < 0.1)
		discard;
	return bg;
}
