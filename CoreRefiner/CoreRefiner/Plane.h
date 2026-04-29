#pragma once
#include <optional>
#include "DynamicVertex.h"
#include <vector>
#include <array>
#include "IndexedTriangleList.h"
#include "Math.h"

class Plane
{
public:
	static IndexedTriangleList MakeTesselatedTextured(Dvtx::VertexLayout layout, int divisions_x, int divisions_y)
	{
		namespace dx = DirectX;
		assert(divisions_x >= 1);
		assert(divisions_y >= 1);

		constexpr float width = 1.0f;
		constexpr float height = 1.0f;
		const int nVertices_x = divisions_x + 1;
		const int nVertices_y = divisions_y + 1;
		Dvtx::VertexBuffer vb{ std::move(layout) };

		{
			const float side_x = width / 2.0f;
			const float side_y = height / 2.0f;
			const float divisionSize_x = width / float(divisions_x);
			const float divisionSize_y = height / float(divisions_y);
			const float divisionSize_x_tc = 1.0f / float(divisions_x);
			const float divisionSize_y_tc = 1.0f / float(divisions_y);

			for (int y = 0, i = 0; y < nVertices_y; y++)
			{
				const float y_pos = float(y) * divisionSize_y - side_y;
				const float y_pos_tc = 1.0f - float(y) * divisionSize_y_tc;
				for (int x = 0; x < nVertices_x; x++, i++)
				{
					const float x_pos = float(x) * divisionSize_x - side_x;
					const float x_pos_tc = float(x) * divisionSize_x_tc;
					vb.EmplaceBack(
						dx::XMFLOAT3{ x_pos,y_pos,0.0f },
						dx::XMFLOAT3{ 0.0f,0.0f,-1.0f },
						dx::XMFLOAT2{ x_pos_tc,y_pos_tc }
					);
				}
			}
		}

		std::vector<unsigned short> indices;
		indices.reserve(sq(divisions_x * divisions_y) * 6);
		{
			const auto vxy2i = [nVertices_x](size_t x, size_t y)
				{
					return (unsigned short)(y * nVertices_x + x);
				};
			for (size_t y = 0; y < divisions_y; y++)
			{
				for (size_t x = 0; x < divisions_x; x++)
				{
					const std::array<unsigned short, 4> indexArray =
					{ vxy2i(x,y),vxy2i(x + 1,y),vxy2i(x,y + 1),vxy2i(x + 1,y + 1) };
					indices.push_back(indexArray[0]);
					indices.push_back(indexArray[2]);
					indices.push_back(indexArray[1]);
					indices.push_back(indexArray[1]);
					indices.push_back(indexArray[2]);
					indices.push_back(indexArray[3]);
				}
			}
		}

		return{ std::move(vb),std::move(indices) };
	}
	static IndexedTriangleList Make()
	{
		using Dvtx::VertexLayout;
		VertexLayout vl;
		vl.Append(VertexLayout::Position3D);
		vl.Append(VertexLayout::Normal);
		vl.Append(VertexLayout::Texture2D);

		return MakeTesselatedTextured(std::move(vl), 1, 1);
	}
	static IndexedTriangleList Make2D(int divisions_x, int divisions_y)
	{
		using Dvtx::VertexLayout;
		VertexLayout layout;
		layout.Append(VertexLayout::Position3D);
		layout.Append(VertexLayout::Texture2D);

		namespace dx = DirectX;
		assert(divisions_x >= 1);
		assert(divisions_y >= 1);

		constexpr float width = 1.0f;
		constexpr float height = 1.0f;
		const int nVertices_x = divisions_x + 1;
		const int nVertices_y = divisions_y + 1;
		Dvtx::VertexBuffer vb{ std::move(layout) };

		{
			const float side_x = width / 2.0f;
			const float side_y = height / 2.0f;
			const float divisionSize_x = width / float(divisions_x);
			const float divisionSize_y = height / float(divisions_y);
			const float divisionSize_x_tc = 1.0f / float(divisions_x);
			const float divisionSize_y_tc = 1.0f / float(divisions_y);

			for (int y = 0, i = 0; y < nVertices_y; y++)
			{
				const float y_pos = float(y) * divisionSize_y - side_y;
				const float y_pos_tc = 1.0f - float(y) * divisionSize_y_tc;
				for (int x = 0; x < nVertices_x; x++, i++)
				{
					const float x_pos = float(x) * divisionSize_x - side_x;
					const float x_pos_tc = float(x) * divisionSize_x_tc;
					vb.EmplaceBack(
						dx::XMFLOAT3{ x_pos,y_pos,0.0f },
						dx::XMFLOAT2{ x_pos_tc,y_pos_tc }
					);
				}
			}
		}

		std::vector<unsigned short> indices;
		indices.reserve(sq(divisions_x * divisions_y) * 6);
		{
			const auto vxy2i = [nVertices_x](size_t x, size_t y)
				{
					return (unsigned short)(y * nVertices_x + x);
				};
			for (size_t y = 0; y < divisions_y; y++)
			{
				for (size_t x = 0; x < divisions_x; x++)
				{
					const std::array<unsigned short, 4> indexArray =
					{ vxy2i(x,y),vxy2i(x + 1,y),vxy2i(x,y + 1),vxy2i(x + 1,y + 1) };
					indices.push_back(indexArray[0]);
					indices.push_back(indexArray[2]);
					indices.push_back(indexArray[1]);
					indices.push_back(indexArray[1]);
					indices.push_back(indexArray[2]);
					indices.push_back(indexArray[3]);
				}
			}
		}

		return{ std::move(vb),std::move(indices) };
	}
};





class PlanePolygon
{
public:
	static IndexedTriangleList Make2D(int sides)
	{
		using Dvtx::VertexLayout;
		VertexLayout layout;
		layout.Append(VertexLayout::Position3D);
		layout.Append(VertexLayout::Texture2D);

		Dvtx::VertexBuffer vb{ std::move(layout) };

		// center
		vb.EmplaceBack(
			DirectX::XMFLOAT3{ 0.0f, 0.0f, 0.0f },
			DirectX::XMFLOAT2{ 0.5f, 0.5f }
		);

		// perimeter (sides+1 to close)
		const float step = 2.0f * PI / float(sides);
		const float start = PI / 2;

		for (int i = 0; i <= sides; ++i)
		{
			float a = step * float(i) + start;
			float x = cosf(a) * 0.5f;
			float y = sinf(a) * 0.5f;

			float u = x + 0.5f;
			float v = 0.5f - y;

			vb.EmplaceBack(
				DirectX::XMFLOAT3{ x, y, 0.0f },
				DirectX::XMFLOAT2{ u, v }
			);
		}

		std::vector<unsigned short> indices;
		indices.reserve(sides * 3);

		for (int i = 1; i <= sides; ++i)
		{
			indices.push_back(0);
			indices.push_back((unsigned short)i);
			indices.push_back((unsigned short)(i + 1));
		}

		return { std::move(vb), std::move(indices) };
	}
};