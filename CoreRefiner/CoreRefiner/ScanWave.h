#pragma once

#include "FieldModuleNode.h"

#include <DirectXMath.h>

struct ScanWave
{
	FieldModuleNode* source{ nullptr };
	DirectX::XMFLOAT2 center{ 0.0f, 0.0f };
	float radius{ 0.0f };
	float maxRadius{ 120.0f };
	float expandSpeed{ 80.0f };
	bool alive{ false };

	bool Expand(float dt)
	{
		if (!alive)
		{
			return false;
		}
		radius += expandSpeed * dt;
		if (radius >= maxRadius)
		{
			radius = maxRadius;
			alive = false;
			return true;
		}
		return false;
	}

	void Start(FieldModuleNode* src, DirectX::XMFLOAT2 at, float maxR, float speed)
	{
		source = src;
		center = at;
		radius = 0.0f;
		maxRadius = maxR;
		expandSpeed = speed;
		alive = true;
	}

	void Stop()
	{
		alive = false;
	}
};
