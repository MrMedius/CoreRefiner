#pragma once

namespace Ui
{
	struct UiRect
	{
		float minX = 0.0f;
		float minY = 0.0f;
		float maxX = 0.0f;
		float maxY = 0.0f;

		bool Contains(float x, float y) const noexcept
		{
			return x >= minX && x <= maxX && y >= minY && y <= maxY;
		}
	};
}