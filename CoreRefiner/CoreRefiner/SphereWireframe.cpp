#include "SphereWireframe.h"
#include "BindableCommon.h"
#include "GraphicsThrowMacros.h"
#include "DynamicVertex.h"
#include "Channels.h"

#include <cmath>
#include <vector>

namespace dx = DirectX;

namespace
{
	// Segments per great circle (Unity-style three rings).
	constexpr int kRingSegments = 48;
	constexpr float kMeshRadius = 0.5f;
	constexpr float kPi = 3.14159265358979323846f;
	constexpr float kTwoPi = 2.0f * kPi;

	// Append one closed ring as LINELIST segments into vertices/indices.
	// makePoint Maps angle theta in [0, 2pi] to a mesh-space position.
	template <typename F>
	void AppendRing(
		Dvtx::VertexBuffer& vertices,
		std::vector<unsigned short>& indices,
		F&& makePoint)
	{
		const auto base = static_cast<unsigned short>(vertices.Size());
		for (int i = 0; i < kRingSegments; ++i)
		{
			const float theta = kTwoPi * static_cast<float>(i) / static_cast<float>(kRingSegments);
			vertices.EmplaceBack(makePoint(theta));
		}
		for (int i = 0; i < kRingSegments; ++i)
		{
			const auto a = static_cast<unsigned short>(base + i);
			const auto b = static_cast<unsigned short>(base + ((i + 1) % kRingSegments));
			indices.push_back(a);
			indices.push_back(b);
		}
	}
}

SphereWireframe::SphereWireframe(Graphics& gfx, DirectX::XMFLOAT3 color, std::string tag)
{
	using namespace Bind;

	// Tag must differ from older lat/lon cache ("swire") so Resolve rebuilds geometry.
	const auto geometryTag = "swire3circ";
	Dvtx::VertexLayout layout;
	layout.Append(Dvtx::VertexLayout::Position3D);
	Dvtx::VertexBuffer vertices{ std::move(layout) };
	std::vector<unsigned short> indices;

	// Three orthogonal great circles (local XY / XZ / YZ), mesh radius 0.5.
	AppendRing(vertices, indices, [](float theta) {
		return dx::XMFLOAT3{ kMeshRadius * std::cos(theta), kMeshRadius * std::sin(theta), 0.0f };
	});
	AppendRing(vertices, indices, [](float theta) {
		return dx::XMFLOAT3{ kMeshRadius * std::cos(theta), 0.0f, kMeshRadius * std::sin(theta) };
	});
	AppendRing(vertices, indices, [](float theta) {
		return dx::XMFLOAT3{ 0.0f, kMeshRadius * std::cos(theta), kMeshRadius * std::sin(theta) };
	});

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

void SphereWireframe::DoSubmit(DirectX::XMFLOAT3 pos, float diameter)
{
	SetPosition(pos);
	SetScale({ diameter, diameter, diameter });
	Drawable::Submit(Chan::main);
}

void SphereWireframe::DoSubmit(DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 rot, float diameter)
{
	SetPosition(pos);
	SetRotation(rot.x, rot.y, rot.z);
	SetScale({ diameter, diameter, diameter });
	Drawable::Submit(Chan::main);
}

void SphereWireframe::SetPosition(DirectX::XMFLOAT3 pos) noexcept
{
	trans.SetPosition(pos.x, pos.y, pos.z);
}

void SphereWireframe::SetRotation(float roll, float pitch, float yaw) noexcept
{
	trans.SetRotationDegreeToRad(roll, pitch, yaw);
}

void SphereWireframe::SetScale(DirectX::XMFLOAT3 size) noexcept
{
	trans.SetScale(size.x, size.y, size.z);
}

DirectX::XMMATRIX SphereWireframe::GetTransformXM() const noexcept
{
	return trans.GetTransformXM();
}
