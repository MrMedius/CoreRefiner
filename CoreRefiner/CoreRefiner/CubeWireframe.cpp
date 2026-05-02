#include "CubeWireframe.h"
#include "BindableCommon.h"
#include "GraphicsThrowMacros.h"
#include "DynamicVertex.h"
#include "Stencil.h"
#include "Channels.h"

namespace dx = DirectX;

CubeWireframe::CubeWireframe(Graphics& gfx, DirectX::XMFLOAT3 color, std::string tag)
{
	using namespace Bind;

	const auto geometryTag = "cwire";
	Dvtx::VertexLayout layout;
	layout.Append(Dvtx::VertexLayout::Position3D);
	Dvtx::VertexBuffer vertices{ std::move(layout) };
	{
		constexpr float side = 0.5f;
		vertices.EmplaceBack(dx::XMFLOAT3{ -side, -side, -side });
		vertices.EmplaceBack(dx::XMFLOAT3{  side, -side, -side });
		vertices.EmplaceBack(dx::XMFLOAT3{ -side,  side, -side });
		vertices.EmplaceBack(dx::XMFLOAT3{  side,  side, -side });
		vertices.EmplaceBack(dx::XMFLOAT3{ -side, -side,  side });
		vertices.EmplaceBack(dx::XMFLOAT3{  side, -side,  side });
		vertices.EmplaceBack(dx::XMFLOAT3{ -side,  side,  side });
		vertices.EmplaceBack(dx::XMFLOAT3{  side,  side,  side });
	}

	std::vector<unsigned short> indices;
	{
		indices.push_back(0);	indices.push_back(1);
		indices.push_back(1);	indices.push_back(3);
		indices.push_back(3);	indices.push_back(2);
		indices.push_back(2);	indices.push_back(0);

		indices.push_back(4);	indices.push_back(5);
		indices.push_back(5);	indices.push_back(7);
		indices.push_back(7);	indices.push_back(6);
		indices.push_back(6);	indices.push_back(4);

		indices.push_back(0);	indices.push_back(4);		
		indices.push_back(1);	indices.push_back(5);		
		indices.push_back(2);	indices.push_back(6);		
		indices.push_back(3);	indices.push_back(7);
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

void CubeWireframe::DoSubmit(DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 size)
{
	SetPosition(pos);
	SetScale(size);
	Drawable::Submit(Chan::main);
}

void CubeWireframe::DoSubmit(DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 rot, DirectX::XMFLOAT3 size)
{
	SetPosition(pos);
	SetRotation(rot.x, rot.y, rot.z);
	SetScale(size);
	Drawable::Submit(Chan::main);
}

void CubeWireframe::SetPosition(DirectX::XMFLOAT3 pos) noexcept
{
	trans.SetPosition(pos.x, pos.y, pos.z);
}

void CubeWireframe::SetRotation(float roll, float pitch, float yaw) noexcept
{
	trans.SetRotationDegreeToRad(roll, pitch, yaw);
}

void CubeWireframe::SetScale(DirectX::XMFLOAT3 size) noexcept
{
	trans.SetScale(size.x, size.y, size.z);
}

DirectX::XMMATRIX CubeWireframe::GetTransformXM() const noexcept
{
	return trans.GetTransformXM();
}