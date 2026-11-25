#pragma once
#include "EnvironmentBase.h"
#include "RenderGraph.h"
#include "Cube.h"
#include "BindableCommon.h"
#include "Channels.h"
#include "VisualComponent.h"
#include "BoxColliderComponent.h"

// 场地自己的半透明方块。尺寸在构造时烤进网格，只给 Field 用。
class Field_Shape : public Drawable
{
public:
	// size 是方块三边的世界尺寸。
	Field_Shape(Graphics& gfx, DirectX::XMFLOAT3 size)
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
			Step only("lambertianTrans");

			auto pvs = VertexShader::Resolve(gfx, "SolidTrans_VS.cso");
			only.AddBindable(InputLayout::Resolve(gfx, model.vertices.GetLayout(), *pvs));
			only.AddBindable(std::move(pvs));
			only.AddBindable(PixelShader::Resolve(gfx, "SolidTrans_PS.cso"));

			// color 是场地的半透明蓝。
			struct PSColorConstant
			{
				dx::XMFLOAT4 color = { 0.0f, 0.0f, 0.5f, 0.5f };
			} colorConst;
			only.AddBindable(PixelConstantBuffer<PSColorConstant>::Resolve(gfx, colorConst, 1u));
			only.AddBindable(std::make_shared<TransformCbuf>(gfx));
			only.AddBindable(Rasterizer::Resolve(gfx, false));
			only.AddBindable(Blender::Resolve(gfx, true));

			shade.AddStep(std::move(only));
			AddTechnique(std::move(shade));
		}
	}

	// dt 是这一帧的秒数。把网格绕三轴各转一度。
	void Update(float dt)
	{
		(void)dt;
		trans.RotateDegreeToRad(1.0f, 1.0f, 1.0f);
	}

	DirectX::XMMATRIX GetTransformXM() const noexcept override
	{
		return trans.GetTransformXM();
	}
};

class Field : public EnvironmentBase
{
public:
	Field(Graphics& gfx, Rgph::RenderGraph& rg, XMFLOAT3 position, XMFLOAT3 size, bool onCollision, Object_Type_Tag tag = environment_Field)
		:
		EnvironmentBase(tag)
	{
		// parameters init
		SetPosition(position);
		SetSize(size);

		// graphics init — VisualComponent owns the Drawable (mesh baked to size; syncScale=false)
		{
			auto shape = std::make_unique<Field_Shape>(gfx, size);
			shape->LinkTechniques(rg);
			AddComponent<VisualComponent>(std::move(shape), Chan::main | Chan::shadow, false, false);
		}

		// BoxColliderComponent + FromWorldMatrix (do not SetCollisionSize — it would overwrite matrix half)
		DirectX::XMFLOAT3 localHalf{ 0.5f, 0.5f, 0.5f };
		pCollider_ = AddComponent<BoxColliderComponent>(ColliderSyncMode::FromWorldMatrix, localHalf);
		pCollider_->SetEnabled(onCollision);
		pCollider_->LinkDebugWire(gfx, rg, XMFLOAT3(1.0f, 0.0f, 0.0f), "wireFieldBox");
	}
	void OnEnable(void) override {}
	void Update(float dt) override
	{
		ObjectBase::Update(dt);
	}
	void Submit(void) override
	{
		ObjectBase::Submit();
	}
	void OnCollide(CharacterBase* other) override {}
private:
	BoxColliderComponent* pCollider_{ nullptr };
};
