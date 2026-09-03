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
			Step only("lambertianTrans");

			auto pvs = VertexShader::Resolve(gfx, "SolidTrans_VS.cso");
			only.AddBindable(InputLayout::Resolve(gfx, model.vertices.GetLayout(), *pvs));
			only.AddBindable(std::move(pvs));

			only.AddBindable(PixelShader::Resolve(gfx, "SolidTrans_PS.cso"));

			struct PSColorConstant
			{
				dx::XMFLOAT4 color = { 0.0f, 0.0f, 0.5f, 0.5f };
			} colorConst;
			only.AddBindable(PixelConstantBuffer<PSColorConstant>::Resolve(gfx, colorConst, 1u));

			only.AddBindable(std::make_shared<TransformCbuf>(gfx));
			only.AddBindable(Rasterizer::Resolve(gfx, false));
			only.AddBindable(Blender::Resolve(gfx, true));

			shade.AddStep(std::move(only));
		}
		AddTechnique(std::move(shade));
	}
}

void Field_Shape::Update(float dt)
{
	trans.RotateDegreeToRad(1.0f, 1.0f, 1.0f);
}

DirectX::XMMATRIX Field_Shape::GetTransformXM() const noexcept
{
	return trans.GetTransformXM();
}