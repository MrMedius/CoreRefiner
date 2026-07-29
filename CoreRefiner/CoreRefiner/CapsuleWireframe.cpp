#include "CapsuleWireframe.h"
#include "BindableCommon.h"
#include "DynamicVertex.h"
#include "Channels.h"

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

	/**
	 * @brief Outer hemisphere meridian arcs in local +Y (equator at y=0, pole at +radius).
	 * @note No equator ring — cylinder end rings already mark the join.
	 */
	void PushHemisphereMeridians(
		Dvtx::VertexBuffer& vertices,
		std::vector<unsigned short>& indices,
		float radius)
	{
		const int half = kRingSegments / 2;
		// XY plane arc: (r cos th, r sin th, 0), th in [0, pi]
		{
			const auto base = static_cast<unsigned short>(vertices.Size());
			for (int i = 0; i <= half; ++i)
			{
				const float th = kPi * static_cast<float>(i) / static_cast<float>(half);
				vertices.EmplaceBack(dx::XMFLOAT3{
					radius * std::cos(th),
					radius * std::sin(th),
					0.0f });
			}
			for (int i = 0; i < half; ++i)
			{
				indices.push_back(static_cast<unsigned short>(base + i));
				indices.push_back(static_cast<unsigned short>(base + i + 1));
			}
		}
		// YZ plane arc: (0, r sin th, r cos th), th in [0, pi]
		{
			const auto base = static_cast<unsigned short>(vertices.Size());
			for (int i = 0; i <= half; ++i)
			{
				const float th = kPi * static_cast<float>(i) / static_cast<float>(half);
				vertices.EmplaceBack(dx::XMFLOAT3{
					0.0f,
					radius * std::sin(th),
					radius * std::cos(th) });
			}
			for (int i = 0; i < half; ++i)
			{
				indices.push_back(static_cast<unsigned short>(base + i));
				indices.push_back(static_cast<unsigned short>(base + i + 1));
			}
		}
	}

	/**
	 * @brief Orthonormal basis with Y = normalized outward (fallback +Y if degenerate).
	 */
	void BuildOutwardBasis(
		dx::FXMVECTOR outward,
		dx::XMFLOAT3& outX,
		dx::XMFLOAT3& outY,
		dx::XMFLOAT3& outZ)
	{
		dx::XMVECTOR y = outward;
		const float lenSq = dx::XMVectorGetX(dx::XMVector3LengthSq(y));
		if (lenSq < 1e-10f)
		{
			y = dx::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
		}
		else
		{
			y = dx::XMVector3Normalize(y);
		}
		dx::XMVECTOR ref = (std::fabs(dx::XMVectorGetY(y)) > 0.99f)
			? dx::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f)
			: dx::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
		dx::XMVECTOR x = dx::XMVector3Normalize(dx::XMVector3Cross(ref, y));
		dx::XMVECTOR z = dx::XMVector3Cross(x, y);
		dx::XMStoreFloat3(&outX, x);
		dx::XMStoreFloat3(&outY, y);
		dx::XMStoreFloat3(&outZ, z);
	}
}

CapsuleWireframe::HemisphereWire::HemisphereWire(
	Graphics& gfx,
	DirectX::XMFLOAT3 color,
	std::string tag)
{
	using namespace Bind;

	const auto geometryTag = "cawire.hemi";
	Dvtx::VertexLayout layout;
	layout.Append(Dvtx::VertexLayout::Position3D);
	Dvtx::VertexBuffer vertices{ std::move(layout) };
	std::vector<unsigned short> indices;
	PushHemisphereMeridians(vertices, indices, kMeshRadius);

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

void CapsuleWireframe::HemisphereWire::DoSubmit(
	DirectX::XMFLOAT3 center,
	DirectX::XMFLOAT3 outwardAxis,
	float radius)
{
	using namespace dx;
	center_ = center;
	BuildOutwardBasis(XMLoadFloat3(&outwardAxis), axisX_, axisY_, axisZ_);
	scale_ = radius * 2.0f;
	Drawable::Submit(Chan::main);
}

DirectX::XMMATRIX CapsuleWireframe::HemisphereWire::GetTransformXM() const noexcept
{
	using namespace dx;
	const XMMATRIX R = XMMATRIX(
		axisX_.x, axisX_.y, axisX_.z, 0.0f,
		axisY_.x, axisY_.y, axisY_.z, 0.0f,
		axisZ_.x, axisZ_.y, axisZ_.z, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f);
	return XMMatrixScaling(scale_, scale_, scale_) *
		R *
		XMMatrixTranslation(center_.x, center_.y, center_.z);
}

CapsuleWireframe::CylinderWire::CylinderWire(Graphics& gfx, DirectX::XMFLOAT3 color, std::string tag)
{
	using namespace Bind;

	const auto geometryTag = "cawire.cyl";
	Dvtx::VertexLayout layout;
	layout.Append(Dvtx::VertexLayout::Position3D);
	Dvtx::VertexBuffer vertices{ std::move(layout) };
	std::vector<unsigned short> indices;

	const float yA = kMeshHalfSeg;
	const float yB = -kMeshHalfSeg;
	PushRingXY(vertices, indices, yA, kMeshRadius);
	PushRingXY(vertices, indices, yB, kMeshRadius);
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
		only.AddBindable(PixelConstantBuffer<PSColorConstant>::Resolve(gfx, colorConst, 1u, tag + ".cyl"));
		only.AddBindable(std::make_shared<TransformCbuf>(gfx));
		only.AddBindable(Rasterizer::Resolve(gfx, false));
		line.AddStep(std::move(only));
		AddTechnique(std::move(line));
	}
}

void CapsuleWireframe::CylinderWire::DoSubmit(
	DirectX::XMFLOAT3 pointA,
	DirectX::XMFLOAT3 pointB,
	float radius)
{
	using namespace dx;
	XMVECTOR a = XMLoadFloat3(&pointA);
	XMVECTOR b = XMLoadFloat3(&pointB);
	XMVECTOR mid = XMVectorScale(XMVectorAdd(a, b), 0.5f);
	XMVECTOR axis = XMVectorSubtract(b, a);
	float segLen = XMVectorGetX(XMVector3Length(axis));
	if (segLen < 1e-5f)
	{
		return;
	}
	axis = XMVector3Normalize(axis);

	XMStoreFloat3(&mid_, mid);
	XMStoreFloat3(&axisY_, axis);

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

DirectX::XMMATRIX CapsuleWireframe::CylinderWire::GetTransformXM() const noexcept
{
	using namespace dx;
	const XMMATRIX R = XMMATRIX(
		axisX_.x, axisX_.y, axisX_.z, 0.0f,
		axisY_.x, axisY_.y, axisY_.z, 0.0f,
		axisZ_.x, axisZ_.y, axisZ_.z, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f);
	return XMMatrixScaling(scaleXZ_, scaleY_, scaleXZ_) *
		R *
		XMMatrixTranslation(mid_.x, mid_.y, mid_.z);
}

CapsuleWireframe::CapsuleWireframe(Graphics& gfx, DirectX::XMFLOAT3 color, std::string tag)
	:
	endA_(std::make_unique<HemisphereWire>(gfx, color, tag + ".endA")),
	endB_(std::make_unique<HemisphereWire>(gfx, color, tag + ".endB")),
	cylinder_(std::make_unique<CylinderWire>(gfx, color, tag))
{
}

void CapsuleWireframe::LinkTechniques(Rgph::RenderGraph& rg)
{
	endA_->LinkTechniques(rg);
	endB_->LinkTechniques(rg);
	cylinder_->LinkTechniques(rg);
}

void CapsuleWireframe::DoSubmit(
	DirectX::XMFLOAT3 pointA,
	DirectX::XMFLOAT3 pointB,
	float radius)
{
	using namespace dx;
	const XMVECTOR a = XMLoadFloat3(&pointA);
	const XMVECTOR b = XMLoadFloat3(&pointB);
	const XMVECTOR ab = XMVectorSubtract(a, b);
	XMFLOAT3 outA{};
	XMFLOAT3 outB{};
	if (XMVectorGetX(XMVector3LengthSq(ab)) < 1e-10f)
	{
		// Degenerate segment ≈ sphere: opposite domes make a full wire sphere.
		outA = { 0.0f, 1.0f, 0.0f };
		outB = { 0.0f, -1.0f, 0.0f };
	}
	else
	{
		// Outward = away from the other hemisphere center (collision surface).
		XMStoreFloat3(&outA, ab);
		XMStoreFloat3(&outB, XMVectorSubtract(b, a));
	}

	endA_->DoSubmit(pointA, outA, radius);
	endB_->DoSubmit(pointB, outB, radius);
	cylinder_->DoSubmit(pointA, pointB, radius);
}
