#pragma once
#include "Environment.h"
#include "RenderGraph.h"
#include "Channels.h"
#include "VisualComponent.h"
#include "BoxColliderComponent.h"
#include "Character.h"
#include "ObjectCodex.h"
#include "GameStatsCodex.h"
#include "ColliderComponentBase.h"
#include "Collision3D.h"
#include "Drawable.h"
#include "Cube.h"
#include "BindableCommon.h"
#include "XMath.h"


class Coin_Shape : public Drawable
{
public:
	explicit Coin_Shape(Graphics& gfx)
	{
		using namespace Bind;
		namespace dx = DirectX;

		auto model = Cube::Make();
		const auto geometryTag = "$coin.cube.unit";

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
					dx::XMFLOAT3 color = { 0.25f, 0.55f, 1.0f };
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

	DirectX::XMMATRIX GetTransformXM() const noexcept override
	{
		return trans.GetTransformXM();
	}
};


class Coin : public Environment
{
public:
	static constexpr float kSize = 0.6f;
	/** @brief World-space radius: inside this, the coin flies toward the player. */
	static constexpr float kMagnetRadius = 5.0f;
	/** @brief Magnet travel speed in world units per second. */
	static constexpr float kMagnetSpeed = 14.0f;
	/** @brief 波末强制吸取速度；保证场地对角也能在超时前飞到。 */
	static constexpr float kVacuumSpeed = 40.0f;

	Coin(
		Graphics& gfx,
		Rgph::RenderGraph& rg,
		XMFLOAT3 position,
		Object_Type_Tag tag = environment_Coin)
		:
		Environment(tag)
	{
		SetPosition(position);
		SetSize({ kSize, kSize, kSize });

		{
			auto shape = std::make_unique<Coin_Shape>(gfx);
			shape->LinkTechniques(rg);
			AddComponent<VisualComponent>(std::move(shape), Chan::main, false, true);
		}

		pCollider_ = AddComponent<BoxColliderComponent>(ColliderSyncMode::FollowCenter);
		pCollider_->SetCollisionSize({ kSize, kSize, kSize });
		pCollider_->SetEnabled(true);
		pCollider_->LinkDebugWire(gfx, rg, XMFLOAT3{ 0.25f, 0.55f, 1.0f }, "wireCoinBox");
	}

	void SpawnAt(XMFLOAT3 position)
	{
		collected_ = false;
		forceMagnet_ = false;
		SetPosition(position);
		SetSize({ kSize, kSize, kSize });
		if (pCollider_ != nullptr)
		{
			pCollider_->SetCollisionSize({ kSize, kSize, kSize });
			pCollider_->SetEnabled(true);
			pCollider_->SyncFromOwner();
		}
	}

	void OnEnable(void) override
	{
		collected_ = false;
		forceMagnet_ = false;
		if (pCollider_ != nullptr)
		{
			pCollider_->SetEnabled(true);
			pCollider_->SyncFromOwner();
		}
	}

	void Update(float dt) override
	{
		Rotate(0.0f, 90.0f * dt, 0.0f);
		AttractTowardPlayer_(dt);
		TryCollectFromPlayer_();
		ObjectBase::Update(dt);
	}

	void Submit(void) override
	{
		ObjectBase::Submit();
	}

	void OnCollide(Character* other) override
	{
		if (other == nullptr || other->GetTag() != character_Player)
		{
			return;
		}
		Collect_();
	}

	/** @brief 立刻结算；已收集则忽略。超时收尾用。 */
	void CollectNow()
	{
		Collect_();
	}

	/** @brief 波末强制吸取：无视半径，改用 kVacuumSpeed。 */
	void SetForceMagnet(bool on) noexcept
	{
		forceMagnet_ = on;
	}

private:
	void AttractTowardPlayer_(float dt)
	{
		if (collected_)
		{
			return;
		}

		Character* player = ObjectCodex::FindFirstActiveObjectByTag<Character>(character_Player);
		if (player == nullptr)
		{
			return;
		}

		const Vec3 delta = V(player->GetPosition()) - V(GetPosition());
		const float distSq = delta.LengthSq();
		if (distSq <= 1.0e-8f)
		{
			return;
		}
		if (!forceMagnet_ && distSq > kMagnetRadius * kMagnetRadius)
		{
			return;
		}

		const float dist = delta.Length();
		const float speed = forceMagnet_ ? kVacuumSpeed : kMagnetSpeed;
		const float step = speed * dt;
		if (step >= dist)
		{
			SetPosition(player->GetPosition());
			return;
		}

		const Vec3 dir = delta * (1.0f / dist);
		SetPosition((V(GetPosition()) + dir * step).ToFloat3());
	}

	void TryCollectFromPlayer_()
	{
		if (collected_ || pCollider_ == nullptr || !pCollider_->IsEnabled())
		{
			return;
		}

		Character* player = ObjectCodex::FindFirstActiveObjectByTag<Character>(character_Player);
		if (player == nullptr)
		{
			return;
		}

		auto* playerCol = player->GetComponent<ColliderComponentBase>();
		if (playerCol == nullptr || !playerCol->IsEnabled())
		{
			return;
		}

		if (!Collider3D::CollisionSystem::IsOverlap(pCollider_->GetVolume(), playerCol->GetVolume()))
		{
			return;
		}
		Collect_();
	}

	void Collect_()
	{
		if (collected_)
		{
			return;
		}
		collected_ = true;
		GameStatsCodex::AddCurrency(1);
		if (pCollider_ != nullptr)
		{
			pCollider_->SetEnabled(false);
		}
		RequestDisable();
	}

	BoxColliderComponent* pCollider_{ nullptr };
	bool collected_{ false };
	bool forceMagnet_{ false };
};
