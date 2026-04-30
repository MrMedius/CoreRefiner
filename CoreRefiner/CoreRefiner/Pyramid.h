#pragma once
#include "IndexedTriangleList.h"
#include <DirectXMath.h>
#include "Math.h"

class Pyramid
{
public:
    static IndexedTriangleList Make(std::optional<Dvtx::VertexLayout> layout = std::nullopt, int baseVertexCount = 4)
    {
        using Element = Dvtx::VertexLayout::ElementType;
        if (!layout)
        {
            layout = Dvtx::VertexLayout{}.Append(Element::Position3D);
        }

        constexpr float baseRadius = 0.5f;
        constexpr float height = 0.5f;
        const float angleStep = DirectX::XM_2PI / baseVertexCount;

        Dvtx::VertexBuffer vertices(std::move(*layout), baseVertexCount + 1u);
        for (int i = 0; i < baseVertexCount; ++i)
        {
            float angle = i * angleStep;
            float x = baseRadius * std::cos(angle);
            float z = baseRadius * std::sin(angle);
            vertices[i].Attr<Element::Position3D>() = { x, -baseRadius, z };
        }
        vertices[baseVertexCount].Attr<Element::Position3D>() = { 0.0f, height, 0.0f };

        std::vector<unsigned short> indices;

        // 底面（从下方看为顺时针）
        for (int i = 1; i < baseVertexCount - 1; ++i)
        {
            indices.push_back(0);
            indices.push_back(i);
            indices.push_back(i + 1);
        }

        // 侧面（从外侧看为顺时针）
        for (int i = 0; i < baseVertexCount; ++i)
        {
            int next = (i + 1) % baseVertexCount;
            indices.push_back(next);
            indices.push_back(i);
            indices.push_back(baseVertexCount);
        }

        return {
            std::move(vertices),
            std::move(indices)
        };
    }
};