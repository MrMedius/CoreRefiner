#pragma once
#include "Graphics.h"
#include "RenderGraph.h"
#include "ModuleField.h"
#include "ModuleFieldCanvas.h"
#include "FieldNodes.h"
#include "ScanAssembler.h"
#include "ObjectCodex.h"
#include "Player.h"
#include "Colors.h"
#include "AttackManager.h"

#include "Channels.h"

class UI_Game
{
public:
	UI_Game(Graphics& gfx, Rgph::RenderGraph& rg)
		:
		gfx_(gfx),
		rg_(rg)
	{
		fieldOrigin_ = DirectX::XMFLOAT3{ 200.0f, 200.0f, 0.0f };

		{
			constexpr float side = ModuleFieldCanvas::kDefaultFieldSide;
			fieldCanvas_ = std::make_unique<ModuleFieldCanvas>(gfx, 300u, 300u);
			fieldCanvas_->SetPosition(fieldOrigin_);
			fieldCanvas_->SetScale(DirectX::XMFLOAT3{ side, side, 1.0f });
			fieldCanvas_->LinkTechniques(rg);
		}

		PlaceDemoField_();
		field_.InitAllVisuals(gfx_, rg_, fieldOrigin_);
	}
	~UI_Game() = default;

	void SetAttackManager(AttackManager* manager) noexcept { attackManager_ = manager; }

	/**
	 * @brief Clear scan sessions, node cooldowns, and field ring draw (scene leave).
	 */
	void Reset()
	{
		assembler_.Reset();
		field_.ResetAllCooldowns();
		field_.SyncAllVisuals();
		if (fieldCanvas_ != nullptr)
		{
			fieldCanvas_->ClearWaves();
		}
	}

	void Update(float dt)
	{
		field_.TickAllCooldowns(dt);
		field_.SyncAllVisuals();

		Player* player = ObjectCodex::FindFirstActiveObjectByTag<Player>(character_Player);
		if (player != nullptr && player->GetIsAttack() && assembler_.CanStart(field_))
		{
			const DirectX::XMFLOAT3 spawnPos = player->GetPosition();
			DirectX::XMFLOAT3 aimVel{ 0.0f, 0.0f, 0.05f };
			if (attackManager_ != nullptr)
			{
				attackManager_->TryGetAimVelocity(spawnPos, aimVel);
			}
			assembler_.Begin(field_, gfx_, rg_, spawnPos, aimVel);
		}

		assembler_.Update(dt, field_);

		// Only TakeAllPendingFires erases sessions; keep Update → Take → draw order.
		for (FireBatch& batch : assembler_.TakeAllPendingFires())
		{
			if (attackManager_ != nullptr && player != nullptr)
			{
				const DirectX::XMFLOAT3 pos = player->GetPosition();
				DirectX::XMFLOAT3 vel{ 0.0f, 0.0f, 0.05f };
				attackManager_->TryGetAimVelocity(pos, vel);
				attackManager_->FireRoots(std::move(batch.roots), pos, vel);
			}
			else
			{
				for (Attack* root : batch.roots)
				{
					if (root != nullptr)
					{
						root->Deactivate();
					}
				}
			}
		}

		SyncFieldWaves_();
	}

	void Submit(void)
	{
		if (fieldCanvas_ != nullptr)
		{
			fieldCanvas_->Submit(Chan::ui);
		}
		field_.SubmitAllVisuals();
	}

	[[nodiscard]] ModuleField& GetField() noexcept { return field_; }
	[[nodiscard]] const ModuleField& GetField() const noexcept { return field_; }
	[[nodiscard]] ScanAssembler& GetAssembler() noexcept { return assembler_; }

private:
	void PlaceDemoField_()
	{
		field_.AddNode<FieldNode_Spawn_Ball_Core>(DirectX::XMFLOAT2{ 0.0f, 0.0f });

		field_.AddNode<FieldNode_Other_Child>(DirectX::XMFLOAT2{ -70.0f, 40.0f });
		field_.AddNode<FieldNode_Other_Child>(DirectX::XMFLOAT2{ 80.0f, 40.0f });

		field_.AddNode<FieldNode_Spawn_Ball>(DirectX::XMFLOAT2{ -50.0f, -40.0f });
		field_.AddNode<FieldNode_Spawn_Ball>(DirectX::XMFLOAT2{ 40.0f, -40.0f });

		field_.AddNode<FieldNode_Rule_Orbit>(DirectX::XMFLOAT2{ 0.0f, 75.0f });
		field_.AddNode<FieldNode_Rule_Orbit>(DirectX::XMFLOAT2{ 0.0f, -95.0f });

		field_.AddNode<FieldNode_Attribute_Lifetime>(DirectX::XMFLOAT2{ 80.0f, 0.0f }, 2.0f);
		field_.AddNode<FieldNode_Attribute_Lifetime>(DirectX::XMFLOAT2{ -110.0f, 0.0f }, 2.0f);

		field_.AddNode<FieldNode_Attribute_SpeedRate>(DirectX::XMFLOAT2{ 30.0f, 90.0f }, 0.5f);
		field_.AddNode<FieldNode_Attribute_SpeedRate>(DirectX::XMFLOAT2{ -50.0f, 90.0f }, 0.2f);
	}

	void SyncFieldWaves_()
	{
		if (fieldCanvas_ == nullptr)
		{
			return;
		}

		DirectX::XMFLOAT2 centers[ModuleFieldCanvas::kMaxRings]{};
		float radii[ModuleFieldCanvas::kMaxRings]{};
		unsigned count = 0u;

		assembler_.ForEachAliveWave([&](const ScanWave& wave)
		{
			if (count >= ModuleFieldCanvas::kMaxRings || wave.radius <= 0.0f)
			{
				return;
			}
			centers[count] = wave.center;
			radii[count] = wave.radius;
			++count;
		});

		fieldCanvas_->SetWavesLocal(
			centers, radii, count, ModuleFieldCanvas::kDefaultFieldSide);
	}

	Graphics& gfx_;
	Rgph::RenderGraph& rg_;
	AttackManager* attackManager_{ nullptr };
	DirectX::XMFLOAT3 fieldOrigin_{ 0.0f, 0.0f, 0.0f };
	std::unique_ptr<ModuleFieldCanvas> fieldCanvas_;
	ModuleField field_;
	ScanAssembler assembler_;
};
