#include "Field_Shape.h"
#include "Cube.h"
#include "BindableCommon.h"
#include "Channels.h"

Field_Shape::Field_Shape(Graphics& gfx, DirectX::XMFLOAT3 size)
{
	using namespace Bind;
	namespace dx = DirectX;

	auto model = Cube::Make();
	model.Transform(dx::XMMatrixScaling(size.x, size.y, size.z));
	const auto geometryTag = "$cube." + std::to_string(size.x);

	pVertices = VertexBuffer::Resolve(gfx, geometryTag, model.vertices);
	pIndices = IndexBuffer::Resolve(gfx, geometryTag, model.indices);
	pTopology = Topology::Resolve(gfx, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	{
		Technique shade("Shade", Chan::main);
		{
			Step only("lambertian");

			auto pvs = VertexShader::Resolve(gfx, "Solid_VS.cso");
			only.AddBindable(InputLayout::Resolve(gfx, model.vertices.GetLayout(), *pvs));
			only.AddBindable(std::move(pvs));

			only.AddBindable(PixelShader::Resolve(gfx, "Solid_PS.cso"));

			struct PSColorConstant
			{
				dx::XMFLOAT3 color = { 0.0f,0.0f,0.5f }; // 纯白色
				float padding;
			} colorConst;
			only.AddBindable(PixelConstantBuffer<PSColorConstant>::Resolve(gfx, colorConst, 1u));

			only.AddBindable(std::make_shared<TransformCbuf>(gfx));
			only.AddBindable(Rasterizer::Resolve(gfx, false));

			shade.AddStep(std::move(only));
		}
		AddTechnique(std::move(shade));
	}
}

void Field_Shape::Update(float dt)
{
	trans.RotateDegreeToRad(1.0f, 1.0f, 1.0f);
}

void Field_Shape::SetPosition(DirectX::XMFLOAT3 pos) noexcept
{
	trans.SetPosition(pos.x, pos.y, pos.z);
}

void Field_Shape::SetRotation(float roll, float pitch, float yaw) noexcept
{
	trans.SetRotationDegreeToRad(roll, pitch, yaw);
}

void Field_Shape::SetScale(DirectX::XMFLOAT3 size) noexcept
{
	trans.SetScale(size.x, size.y, size.z);
}

DirectX::XMMATRIX Field_Shape::GetTransformXM() const noexcept
{
	return trans.GetTransformXM();
}