#pragma once
#include <optional>
#include "DynamicVertex.h"
#include "IndexedTriangleList.h"
#include <DirectXMath.h>
#include "Math.h"
#include <array>

class Cube
{
public:
	static IndexedTriangleList Make(std::optional<Dvtx::VertexLayout> layout = {})
	{
		using namespace Dvtx;
		using Type = Dvtx::VertexLayout::ElementType;

		if (!layout)
		{
			layout = Dvtx::VertexLayout{};
			layout->Append(Type::Position3D);
		}

		constexpr float side = 1.0f / 2.0f;

		VertexBuffer vertices(std::move(*layout), 8u);
		vertices[0].Attr<Type::Position3D>() = { -side,-side,-side };
		vertices[1].Attr<Type::Position3D>() = {  side,-side,-side };
		vertices[2].Attr<Type::Position3D>() = { -side, side,-side };
		vertices[3].Attr<Type::Position3D>() = {  side, side,-side };
		vertices[4].Attr<Type::Position3D>() = { -side,-side, side };
		vertices[5].Attr<Type::Position3D>() = {  side,-side, side };
		vertices[6].Attr<Type::Position3D>() = { -side, side, side };
		vertices[7].Attr<Type::Position3D>() = {  side, side, side };

		return{
			std::move(vertices),{
				0,2,1, 2,3,1,
				1,3,5, 3,7,5,
				2,6,3, 3,6,7,
				4,5,7, 4,7,6,
				0,4,2, 2,4,6,
				0,1,4, 1,5,4
			}
		};
	}
	static IndexedTriangleList MakeIndependent( Dvtx::VertexLayout layout )
	{
		using namespace Dvtx;
		using Type = Dvtx::VertexLayout::ElementType;

		constexpr float side = 1.0f / 2.0f;

		Dvtx::VertexBuffer vertices( std::move( layout ),24u );
		vertices[0].Attr<Type::Position3D>() =  { -side,-side,-side };// 0 near side
		vertices[1].Attr<Type::Position3D>() =  {  side,-side,-side };// 1
		vertices[2].Attr<Type::Position3D>() =  { -side, side,-side };// 2
		vertices[3].Attr<Type::Position3D>() =  {  side, side,-side };// 3
		vertices[4].Attr<Type::Position3D>() =  { -side,-side, side };// 4 far side
		vertices[5].Attr<Type::Position3D>() =  {  side,-side, side };// 5
		vertices[6].Attr<Type::Position3D>() =  { -side, side, side };// 6
		vertices[7].Attr<Type::Position3D>() =  {  side, side, side };// 7
		vertices[8].Attr<Type::Position3D>() =  { -side,-side,-side };// 8 left side
		vertices[9].Attr<Type::Position3D>() =  { -side, side,-side };// 9
		vertices[10].Attr<Type::Position3D>() = { -side,-side, side };// 10
		vertices[11].Attr<Type::Position3D>() = { -side, side, side };// 11
		vertices[12].Attr<Type::Position3D>() = {  side,-side,-side };// 12 right side
		vertices[13].Attr<Type::Position3D>() = {  side, side,-side };// 13
		vertices[14].Attr<Type::Position3D>() = {  side,-side, side };// 14
		vertices[15].Attr<Type::Position3D>() = {  side, side, side };// 15
		vertices[16].Attr<Type::Position3D>() = { -side,-side,-side };// 16 bottom side
		vertices[17].Attr<Type::Position3D>() = {  side,-side,-side };// 17
		vertices[18].Attr<Type::Position3D>() = { -side,-side, side };// 18
		vertices[19].Attr<Type::Position3D>() = {  side,-side, side };// 19
		vertices[20].Attr<Type::Position3D>() = { -side, side,-side };// 20 top side
		vertices[21].Attr<Type::Position3D>() = {  side, side,-side };// 21
		vertices[22].Attr<Type::Position3D>() = { -side, side, side };// 22
		vertices[23].Attr<Type::Position3D>() = {  side, side, side };// 23

		return{
			std::move( vertices ),{
				0,2, 1,   2,3,1,
				4,5, 7,   4,7,6,
				8,10, 9,  10,11,9,
				12,13,15, 12,15,14,
				16,17,18, 18,17,19,
				20,23,21, 20,22,23
			}
		};
	}
	static IndexedTriangleList MakeIndependentTextured()
	{
		using namespace Dvtx;
		using Type = Dvtx::VertexLayout::ElementType;

		auto itl = MakeIndependent( std::move( VertexLayout{}
			.Append( Type::Position3D )
			.Append( Type::Normal )
			.Append( Type::Texture2D )
		) );

		itl.vertices[0].Attr<Type::Texture2D>() = { 0.0f,0.0f };
		itl.vertices[1].Attr<Type::Texture2D>() = { 1.0f,0.0f };
		itl.vertices[2].Attr<Type::Texture2D>() = { 0.0f,1.0f };
		itl.vertices[3].Attr<Type::Texture2D>() = { 1.0f,1.0f };
		itl.vertices[4].Attr<Type::Texture2D>() = { 0.0f,0.0f };
		itl.vertices[5].Attr<Type::Texture2D>() = { 1.0f,0.0f };
		itl.vertices[6].Attr<Type::Texture2D>() = { 0.0f,1.0f };
		itl.vertices[7].Attr<Type::Texture2D>() = { 1.0f,1.0f };
		itl.vertices[8].Attr<Type::Texture2D>() = { 0.0f,0.0f };
		itl.vertices[9].Attr<Type::Texture2D>() = { 1.0f,0.0f };
		itl.vertices[10].Attr<Type::Texture2D>() = { 0.0f,1.0f };
		itl.vertices[11].Attr<Type::Texture2D>() = { 1.0f,1.0f };
		itl.vertices[12].Attr<Type::Texture2D>() = { 0.0f,0.0f };
		itl.vertices[13].Attr<Type::Texture2D>() = { 1.0f,0.0f };
		itl.vertices[14].Attr<Type::Texture2D>() = { 0.0f,1.0f };
		itl.vertices[15].Attr<Type::Texture2D>() = { 1.0f,1.0f };
		itl.vertices[16].Attr<Type::Texture2D>() = { 0.0f,0.0f };
		itl.vertices[17].Attr<Type::Texture2D>() = { 1.0f,0.0f };
		itl.vertices[18].Attr<Type::Texture2D>() = { 0.0f,1.0f };
		itl.vertices[19].Attr<Type::Texture2D>() = { 1.0f,1.0f };
		itl.vertices[20].Attr<Type::Texture2D>() = { 0.0f,0.0f };
		itl.vertices[21].Attr<Type::Texture2D>() = { 1.0f,0.0f };
		itl.vertices[22].Attr<Type::Texture2D>() = { 0.0f,1.0f };
		itl.vertices[23].Attr<Type::Texture2D>() = { 1.0f,1.0f };

		return itl;
	}
	static IndexedTriangleList MakeIndependentBlockTextured()
	{
		using namespace Dvtx;
		using Type = Dvtx::VertexLayout::ElementType;

		auto itl = MakeIndependent(std::move(VertexLayout{}
			.Append(Type::Position3D)
			.Append(Type::Normal)
			.Append(Type::Texture2D)
		));

		// right face
		itl.vertices[0].Attr<Type::Texture2D>() = { 0.0f,0.5f }; 
		itl.vertices[1].Attr<Type::Texture2D>() = { 1.0f,0.5f }; 
		itl.vertices[2].Attr<Type::Texture2D>() = { 0.0f,0.25f };
		itl.vertices[3].Attr<Type::Texture2D>() = { 1.0f,0.25f };
		// left face
		itl.vertices[4].Attr<Type::Texture2D>() = { 0.0f,0.5f };
		itl.vertices[5].Attr<Type::Texture2D>() = { 1.0f,0.5f };
		itl.vertices[6].Attr<Type::Texture2D>() = { 0.0f,0.25f };
		itl.vertices[7].Attr<Type::Texture2D>() = { 1.0f,0.25f };
		// front face
		itl.vertices[8].Attr<Type::Texture2D>() = { 0.0f,0.5f }; 
		itl.vertices[9].Attr<Type::Texture2D>() = { 0.0f,0.25f }; 
		itl.vertices[10].Attr<Type::Texture2D>() = { 1.0f,0.5f };
		itl.vertices[11].Attr<Type::Texture2D>() = { 1.0f,0.25f };
		// back face
		itl.vertices[12].Attr<Type::Texture2D>() = { 0.0f,0.5f };
		itl.vertices[13].Attr<Type::Texture2D>() = { 0.0f,0.25f };
		itl.vertices[14].Attr<Type::Texture2D>() = { 1.0f,0.5f };
		itl.vertices[15].Attr<Type::Texture2D>() = { 1.0f,0.25f };
		// down face
		itl.vertices[16].Attr<Type::Texture2D>() = { 0.0f,0.75f };
		itl.vertices[17].Attr<Type::Texture2D>() = { 1.0f,0.75f };
		itl.vertices[18].Attr<Type::Texture2D>() = { 0.0f,1.0f };
		itl.vertices[19].Attr<Type::Texture2D>() = { 1.0f,1.0f };
		// upper face
		itl.vertices[20].Attr<Type::Texture2D>() = { 0.0f,0.0f };
		itl.vertices[21].Attr<Type::Texture2D>() = { 1.0f,0.0f };
		itl.vertices[22].Attr<Type::Texture2D>() = { 0.0f,0.25f };
		itl.vertices[23].Attr<Type::Texture2D>() = { 1.0f,0.25f };

		return itl;
	}
};




class CubeSlices
{
public:
	static IndexedTriangleList MakeLayeredSlices(int layerCount = 50, float exponent = 2.0f, bool includeTexcoord = false)
	{
		using namespace Dvtx;
		using Type = VertexLayout::ElementType;

		VertexLayout layout;
		layout.Append(Type::Position3D);
		if (includeTexcoord)
		{
			layout.Append(Type::Texture2D);
		}

		VertexBuffer vb(std::move(layout), layerCount * 4u);

		std::vector<std::uint16_t> indices;
		indices.reserve(layerCount * 6u);

		constexpr float half = 0.5f;

		for (int i = 0; i < layerCount; ++i)
		{
			float t = float(i) / float(layerCount - 1);

			// Nonlinear layer distribution
			// t^exponent changing the regularity of interlayer spacing, and the bands will be significantly weakened.
			t = std::pow(t, exponent);

			// local y in [-0.5, 0.5]
			const float y = -half + t * 1.0f;

			const std::uint16_t base = (std::uint16_t)(i * 4);

			vb[base + 0].Attr<Type::Position3D>() = { -half, y,  half };
			vb[base + 1].Attr<Type::Position3D>() = { half, y,  half };
			vb[base + 2].Attr<Type::Position3D>() = { -half, y, -half };
			vb[base + 3].Attr<Type::Position3D>() = { half, y, -half };

			if (includeTexcoord)
			{
				vb[base + 0].Attr<Type::Texture2D>() = { 0.0f, 0.0f };
				vb[base + 1].Attr<Type::Texture2D>() = { 1.0f, 0.0f };
				vb[base + 2].Attr<Type::Texture2D>() = { 0.0f, 1.0f };
				vb[base + 3].Attr<Type::Texture2D>() = { 1.0f, 1.0f };
			}

			// triangles
			indices.push_back(base + 0);
			indices.push_back(base + 1);
			indices.push_back(base + 2);

			indices.push_back(base + 1);
			indices.push_back(base + 3);
			indices.push_back(base + 2);
		}

		return { std::move(vb), std::move(indices) };
	}
};