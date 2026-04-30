#include "Player_Shape.h"
#include "Sphere.h"
#include "Pyramid.h"
#include "BindableCommon.h"
#include "Channels.h"

// HEAD
Player_Head::Player_Head(Graphics& gfx, DirectX::XMFLOAT3 size)
{
	using namespace Bind;
	namespace dx = DirectX;

	auto model_Head = Sphere::Make();
	model_Head.Transform(dx::XMMatrixScaling(size.x / 3.0f, -size.y / 3.0f, size.z / 3.0f) * dx::XMMatrixTranslation(0.0f, size.y, 0.0f));
	const auto geometryTag = "$sphere." + std::to_string(size.x);

	pVertices = VertexBuffer::Resolve(gfx, geometryTag, model_Head.vertices);
	pIndices = IndexBuffer::Resolve(gfx, geometryTag, model_Head.indices);
	pTopology = Topology::Resolve(gfx, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	{
		Technique head{ "Shade", Chan::main };
		Step only("lambertian");

		auto pvs = VertexShader::Resolve(gfx, "Solid_VS.cso");
		only.AddBindable(InputLayout::Resolve(gfx, model_Head.vertices.GetLayout(), *pvs));
		only.AddBindable(std::move(pvs));

		only.AddBindable(PixelShader::Resolve(gfx, "Solid_PS.cso"));

		struct PSColorConstant
		{
			dx::XMFLOAT3 color = { 1.0f,0.0f,0.0f }; // 纯白色
			float padding;
		} colorConst;
		only.AddBindable(PixelConstantBuffer<PSColorConstant>::Resolve(gfx, colorConst, 1u));

		only.AddBindable(std::make_shared<TransformCbuf>(gfx));
		only.AddBindable(Rasterizer::Resolve(gfx, false));

		head.AddStep(std::move(only));
		AddTechnique(std::move(head));
	}
	
}

void Player_Head::Update(float dt)
{
	trans.RotateDegreeToRad(1.0f, 1.0f, 1.0f);
}

void Player_Head::SetPosition(DirectX::XMFLOAT3 pos) noexcept
{
	trans.SetPosition(pos.x, pos.y, pos.z);
}

void Player_Head::SetRotation(float roll, float pitch, float yaw) noexcept
{
	trans.SetRotationDegreeToRad(roll, pitch, yaw);
}

void Player_Head::SetScale(DirectX::XMFLOAT3 size) noexcept
{
	trans.SetScale(size.x, size.y, size.z);
}

DirectX::XMMATRIX Player_Head::GetTransformXM() const noexcept
{
	return trans.GetTransformXM();
}


// BODY
Player_Body::Player_Body(Graphics& gfx, DirectX::XMFLOAT3 size)
{
	using namespace Bind;
	namespace dx = DirectX;

	auto model_Body = Pyramid::Make(std::nullopt, 10);
	model_Body.Transform(dx::XMMatrixScaling(size.x, -size.y, size.z));
	const auto geometryTag = "$pyramid." + std::to_string(size.x);
	
	pVertices = VertexBuffer::Resolve(gfx, geometryTag, model_Body.vertices);
	pIndices = IndexBuffer::Resolve(gfx, geometryTag, model_Body.indices);
	pTopology = Topology::Resolve(gfx, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	{
		Technique body{ "Shade", Chan::main };
		Step only("lambertian");

		auto pvs = VertexShader::Resolve(gfx, "Solid_VS.cso");
		only.AddBindable(InputLayout::Resolve(gfx, model_Body.vertices.GetLayout(), *pvs));
		only.AddBindable(std::move(pvs));

		only.AddBindable(PixelShader::Resolve(gfx, "Solid_PS.cso"));

		struct PSColorConstant
		{
			dx::XMFLOAT3 color = { 1.0f,1.0f,1.0f }; // 纯白色
			float padding;
		} colorConst;
		only.AddBindable(PixelConstantBuffer<PSColorConstant>::Resolve(gfx, colorConst, 1u));

		only.AddBindable(std::make_shared<TransformCbuf>(gfx));
		only.AddBindable(Rasterizer::Resolve(gfx, false));

		body.AddStep(std::move(only));
		AddTechnique(std::move(body));
	}
}

void Player_Body::Update(float dt)
{
	trans.RotateDegreeToRad(1.0f, 1.0f, 1.0f);
}

void Player_Body::SetPosition(DirectX::XMFLOAT3 pos) noexcept
{
	trans.SetPosition(pos.x, pos.y, pos.z);
}

void Player_Body::SetRotation(float roll, float pitch, float yaw) noexcept
{
	trans.SetRotationDegreeToRad(roll, pitch, yaw);
}

void Player_Body::SetScale(DirectX::XMFLOAT3 size) noexcept
{
	trans.SetScale(size.x, size.y, size.z);
}

DirectX::XMMATRIX Player_Body::GetTransformXM() const noexcept
{
	return trans.GetTransformXM();
}