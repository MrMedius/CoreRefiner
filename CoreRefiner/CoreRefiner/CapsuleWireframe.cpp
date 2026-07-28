#include "CapsuleWireframe.h"
#include "BindableCommon.h"
#include "DynamicVertex.h"
#include "Channels.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace dx = DirectX;

namespace
{
	constexpr int kRingSegments = 32;
	constexpr float kMeshRadius = 0.5f;
	constexpr float kMeshHalfSeg = 0.5f;
	constexpr float kPi = 3.14159265358979323846f;
	constexpr float kTwoPi = 2.0f * kPi;

	void PushLine(
		Dvtx::VertexBuffer& vertices,
		std::vector<unsigned short>& indices,
		dx::XMFLOAT3 p0,
		dx::XMFLOAT3 p1)
	{
		const auto i0 = static_cast<unsigned short>(vertices.Size());
		vertices.EmplaceBack(p0);
		vertices.EmplaceBack(p1);
		indices.push_back(i0);
		indices.push_back(static_cast<unsigned short>(i0 + 1));
	}

	void PushRingXY(
		Dvtx::VertexBuffer& vertices,
		std::vector<unsigned short>& indices,
		float y,
		float radius)
	{
		const auto base = static_cast<unsigned short>(vertices.Size());
		for (int i = 0; i < kRingSegments; ++i)
		{
			const float th = kTwoPi * static_cast<float>(i) / static_cast<float>(kRingSegments);
			vertices.EmplaceBack(dx::XMFLOAT3{ radius * std::cos(th), y, radius * std::sin(th) });
		}
		for (int i = 0; i < kRingSegments; ++i)
		{
			indices.push_back(static_cast<unsigned short>(base + i));
			indices.push_back(static_cast<unsigned short>(base + ((i + 1) % kRingSegments)));
		}
	}

	void PushHemisphereArcs(
		Dvtx::VertexBuffer& vertices,
		std::vector<unsigned short>& indices,
		float centerY,
		float radius,
		bool top)
	{
		const float ySign = top ? 1.0f : -1.0f;
		{
			const auto base = static_cast<unsigned short>(vertices.Size());
			for (int i = 0; i <= kRingSegments / 2; ++i)
			{
				const float th = kPi * static_cast<float>(i) / static_cast<float>(kRingSegments / 2);
				vertices.EmplaceBack(dx::XMFLOAT3{
					radius * std::cos(th),
					centerY + ySign * radius * std::sin(th),
					0.0f });
			}
			for (int i = 0; i < kRingSegments / 2; ++i)
			{
				indices.push_back(static_cast<unsigned short>(base + i));
				indices.push_back(static_cast<unsigned short>(base + i + 1));
			}
		}
		{
			const auto base = static_cast<unsigned short>(vertices.Size());
			for (int i = 0; i <= kRingSegments / 2; ++i)
			{
				const float th = kPi * static_cast<float>(i) / static_cast<float>(kRingSegments / 2);
				vertices.EmplaceBack(dx::XMFLOAT3{
					0.0f,
					centerY + ySign * radius * std::sin(th),
					radius * std::cos(th) });
			}
			for (int i = 0; i < kRingSegments / 2; ++i)
			{
				indices.push_back(static_cast<unsigned short>(base + i));
				indices.push_back(static_cast<unsigned short>(base + i + 1));
			}
		}
	}
}

CapsuleWireframe::CapsuleWireframe(Graphics& gfx, DirectX::XMFLOAT3 color, std::string tag)
{
	using namespace Bind;

	const auto geometryTag = "cawire";
	Dvtx::VertexLayout layout;
	layout.Append(Dvtx::VertexLayout::Position3D);
	Dvtx::VertexBuffer vertices{ std::move(layout) };
	std::vector<unsigned short> indices;

	const float yA = kMeshHalfSeg;
	const float yB = -kMeshHalfSeg;
	PushRingXY(vertices, indices, yA, kMeshRadius);
	PushRingXY(vertices, indices, yB, kMeshRadius);
	PushHemisphereArcs(vertices, indices, yA, kMeshRadius, true);
	PushHemisphereArcs(vertices, indices, yB, kMeshRadius, false);
	for (int i = 0; i < 4; ++i)
	{
		const float th = kTwoPi * static_cast<float>(i) / 4.0f;
		const float x = kMeshRadius * std::cos(th);
		const float z = kMeshRadius * std::sin(th);
		PushLine(vertices, indices, { x, yA, z }, { x, yB, z });
	}

	pVertices = VertexBuffer::Resolve(gfx, geometryTag, vertices);
	pIndices = IndexBuffer::Resolve(gfx, geometryTag, indices);
	pTopology = Topology::Resolve(gfx, D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

	{
		Technique line{ Chan::main };
		Step only("lambertian");
		auto pvs = VertexShader::Resolve(gfx, "Solid_VS.cso");
		only.AddBindable(InputLayout::Resolve(gfx, vertices.GetLayout(), *pvs));
		only.AddBindable(std::move(pvs));
		only.AddBindable(PixelShader::Resolve(gfx, "Solid_PS.cso"));
		PSColorConstant colorConst{ color };
		only.AddBindable(PixelConstantBuffer<PSColorConstant>::Resolve(gfx, colorConst, 1u, tag));
		only.AddBindable(std::make_shared<TransformCbuf>(gfx));
		only.AddBindable(Rasterizer::Resolve(gfx, false));
		line.AddStep(std::move(only));
		AddTechnique(std::move(line));
	}
}

void CapsuleWireframe::DoSubmit(DirectX::XMFLOAT3 pointA, DirectX::XMFLOAT3 pointB, float radius)
{
	using namespace dx;
	XMVECTOR a = XMLoadFloat3(&pointA);
	XMVECTOR b = XMLoadFloat3(&pointB);
	XMVECTOR mid = XMVectorScale(XMVectorAdd(a, b), 0.5f);
	XMVECTOR axis = XMVectorSubtract(b, a);
	float segLen = XMVectorGetX(XMVector3Length(axis));
	if (segLen < 1e-5f)
	{
		segLen = 1e-5f;
		axis = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	}
	else
	{
		axis = XMVector3Normalize(axis);
	}

	XMStoreFloat3(&mid_, mid);
	XMStoreFloat3(&axisY_, axis);

	// Orthonormal basis: Y = capsule axis
	XMVECTOR ref = (std::fabs(XMVectorGetY(axis)) > 0.99f)
		? XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f)
		: XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMVECTOR x = XMVector3Normalize(XMVector3Cross(ref, axis));
	XMVECTOR z = XMVector3Cross(x, axis);
	XMStoreFloat3(&axisX_, x);
	XMStoreFloat3(&axisZ_, z);

	scaleXZ_ = radius * 2.0f;
	scaleY_ = segLen;
	Drawable::Submit(Chan::main);
}

DirectX::XMMATRIX CapsuleWireframe::GetTransformXM() const noexcept
{
	using namespace dx;
	// Local axes as rows of rotation (matches S * R * T used elsewhere with row vectors).
	const XMMATRIX R = XMMATRIX(
		axisX_.x, axisX_.y, axisX_.z, 0.0f,
		axisY_.x, axisY_.y, axisY_.z, 0.0f,
		axisZ_.x, axisZ_.y, axisZ_.z, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f);
	return XMMatrixScaling(scaleXZ_, scaleY_, scaleXZ_) *
		R *
		XMMatrixTranslation(mid_.x, mid_.y, mid_.z);
}
