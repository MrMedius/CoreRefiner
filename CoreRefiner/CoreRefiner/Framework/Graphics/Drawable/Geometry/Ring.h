#pragma once
#include "IndexedTriangleList.h"
#include "Framework/Helpers/Mathematics/Math.h"

#include <cassert>

// 平铺在 XZ 平面上的圆环。外径是 1，可见的一面朝 +Y。
class Ring
{
public:
	// 按分段生成圆环。
	// layout 只能有 Position3D 这一个元素。
	// segments 是圆周分成的段数，至少 3。
	// innerRatio 是内径除以外径，取 (0, 1)。越接近 1，环越细。
	static IndexedTriangleList MakeTesselated(Dvtx::VertexLayout layout, int segments, float innerRatio)
	{
		assert(segments >= 3);
		// 外圈和内圈各一段顶点，索引必须放进 unsigned short。
		assert(segments <= 32767);
		assert(innerRatio > 0.0f && innerRatio < 1.0f);

		// outerRadius 是圆环外沿到圆心的距离。模型固定为 1，放大交给变换。
		constexpr float outerRadius = 1.0f;
		// innerRadius 是圆环内沿到圆心的距离。
		const float innerRadius = outerRadius * innerRatio;
		// step 是相邻两个分段之间的弧度。
		const float step = 2.0f * PI / float(segments);

		Dvtx::VertexBuffer vb{ std::move(layout) };
		for (int i = 0; i < segments; ++i)
		{
			const float angle = step * float(i);
			const float c = cosf(angle);
			const float s = sinf(angle);
			vb.EmplaceBack(DirectX::XMFLOAT3{ c * outerRadius, 0.0f, s * outerRadius });
		}
		for (int i = 0; i < segments; ++i)
		{
			const float angle = step * float(i);
			const float c = cosf(angle);
			const float s = sinf(angle);
			vb.EmplaceBack(DirectX::XMFLOAT3{ c * innerRadius, 0.0f, s * innerRadius });
		}

		std::vector<unsigned short> indices;
		indices.reserve(static_cast<size_t>(segments) * 6u);
		for (int i = 0; i < segments; ++i)
		{
			// outer / inner 是当前分段的外圈和内圈顶点。
			const auto outer = static_cast<unsigned short>(i);
			const auto inner = static_cast<unsigned short>(segments + i);
			const int next = (i + 1) % segments;
			// outerNext / innerNext 是下一分段，最后一段接回第一段。
			const auto outerNext = static_cast<unsigned short>(next);
			const auto innerNext = static_cast<unsigned short>(segments + next);
			indices.push_back(outer);
			indices.push_back(inner);
			indices.push_back(outerNext);
			indices.push_back(inner);
			indices.push_back(innerNext);
			indices.push_back(outerNext);
		}

		return { std::move(vb), std::move(indices) };
	}

	// 生成默认圆环。innerRatio 默认 0.75，圆周 32 段。
	static IndexedTriangleList Make(float innerRatio = 0.75f, int segments = 32)
	{
		using Element = Dvtx::VertexLayout::ElementType;
		Dvtx::VertexLayout layout;
		layout.Append(Element::Position3D);
		return MakeTesselated(std::move(layout), segments, innerRatio);
	}
};
