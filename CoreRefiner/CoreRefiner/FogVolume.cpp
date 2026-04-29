#include "FogVolume.h"
#include "Cube.h"
#include "BindableCommon.h"
#include "ConstantBuffersEx.h"
#include "imgui/imgui.h"
#include "Stencil.h"
#include "DynamicConstant.h"
#include "Channels.h"

FogVolume::FogVolume(Graphics& gfx, DirectX::XMFLOAT3 size)
{
	using namespace Bind;
	namespace dx = DirectX;

	auto model = CubeSlices::MakeLayeredSlices(kLayerCount, 2.0f, false);
	model.Transform(dx::XMMatrixScaling(size.x, size.y, size.z));

	const auto geometryTag = "$cubeSlices." + std::to_string(size.x);
	pVertices = VertexBuffer::Resolve(gfx, geometryTag, model.vertices);
	pIndices = IndexBuffer::Resolve(gfx, geometryTag, model.indices);
	pTopology = Topology::Resolve(gfx, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	auto tcb = std::make_shared<TransformCbuf>(gfx);

	{
		Technique shade("Shade", Chan::main);
		{
			Step only("lambertianTrans");

			only.AddBindable(Sampler::Resolve(gfx));

			auto pvs = VertexShader::Resolve(gfx, "FogVolume_VS.cso");
			only.AddBindable(InputLayout::Resolve(gfx, model.vertices.GetLayout(), *pvs));
			only.AddBindable(std::move(pvs));

			only.AddBindable(PixelShader::Resolve(gfx, "FogVolume_PS.cso"));

			Dcb::RawLayout lay;
			lay.Add<Dcb::Float4>("topColor");
			lay.Add<Dcb::Float4>("bottomColor");
			lay.Add<Dcb::Float>("topY");
			lay.Add<Dcb::Float>("bottomY");
			lay.Add<Dcb::Float>("density");
			lay.Add<Dcb::Float>("stepLen");
			lay.Add<Dcb::Float>("ditherAmp");
			auto buf = Dcb::Buffer(std::move(lay));
			buf["topColor"] = dx::XMFLOAT4{ 0.0f, 0.1f, 0.4f, 0.05f };
			buf["bottomColor"] = dx::XMFLOAT4{ 0.0f, 0.05f, 0.15f, 0.15f };
			buf["topY"] = 0.0f;
			buf["bottomY"] = -1000.0f;
			buf["density"] = 0.08f;
			buf["stepLen"] = size.y / float(kLayerCount - 1);   // world-space thickness
			buf["ditherAmp"] = 0.02f;
			only.AddBindable(std::make_shared<Bind::CachingPixelConstantBufferEX>(gfx, buf, 2u));

			only.AddBindable(Rasterizer::Resolve(gfx, false));
			only.AddBindable(Blender::Resolve(gfx, true));

			only.AddBindable(tcb);

			shade.AddStep(std::move(only));
		}
		AddTechnique(std::move(shade));
	}
}

void FogVolume::SetPosition(DirectX::XMFLOAT3 pos) noexcept
{
	trans.SetPosition(pos.x, pos.y, pos.z);
}

void FogVolume::SetRotation(float roll, float pitch, float yaw) noexcept
{
	trans.SetRotationDegreeToRad(roll, pitch, yaw);
}

void FogVolume::SetScale(DirectX::XMFLOAT3 size) noexcept
{
	trans.SetScale(size.x, size.y, size.z);
}

DirectX::XMMATRIX FogVolume::GetTransformXM() const noexcept
{
	return trans.GetTransformXM();
}