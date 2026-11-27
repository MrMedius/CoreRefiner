#pragma once

#include "IModuleNode.h"

#include <DirectXMath.h>

struct ScanWave
{
	// 发出这圈环的 Node。
	IModuleNode* source{ nullptr };
	// 环的圆心，用源 Node 的场内坐标。
	DirectX::XMFLOAT2 center{ 0.0f, 0.0f };
	// 当前环半径。从源 Node 的边缘开始，之后每帧向外加。
	float radius{ 0.0f };
	// 环停住的半径。等于起点加上扩散距离。
	float maxRadius{ 120.0f };
	// 每秒向外扩的距离。
	float expandSpeed{ 80.0f };
	// 为真时这圈环还在扩。
	bool alive{ false };

	// 向外扩一帧。dt 是这一帧的秒数。扩到终点时返回真，并停住。
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

	// 从源 Node 的边缘开始一圈环。src 是发出环的 Node，at 是圆心。
	// maxR 是扩散距离，终点是边缘再向外这么远。speed 是每秒扩出的距离。
	void Start(IModuleNode* src, DirectX::XMFLOAT2 at, float maxR, float speed)
	{
		source = src;
		center = at;
		// origin 是起点，取源 Node 的半径。没有源时从圆心开始。
		const float origin = (src != nullptr) ? src->GetHitRadius() : 0.0f;
		radius = origin;
		maxRadius = origin + maxR;
		expandSpeed = speed;
		alive = true;
	}

	void Stop()
	{
		alive = false;
	}
};
