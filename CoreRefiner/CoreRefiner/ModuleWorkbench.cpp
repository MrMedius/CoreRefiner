#include "ModuleWorkbench.h"
#include "AttackManager.h"
#include "Channels.h"
#include "ModuleNodes.h"
#include "IModuleZone.h"
#include "InputCodex.h"
#include "ObjectCodex.h"
#include "Player.h"
#include "Win.h"
#include "Window.h"
#include <array>

ModuleWorkbench::ModuleWorkbench(Graphics& gfx, Rgph::RenderGraph& rg)
	:
	gfx_(gfx),
	rg_(rg)
{
	// Single source for the three layout origins used by combat + pause edit.
	combatFieldOrigin_ = DirectX::XMFLOAT3{ 200.0f, 200.0f, 0.0f };
	layoutFieldOrigin_ = DirectX::XMFLOAT3{
		static_cast<float>(SCREEN_WIDTH) * 0.8f,
		static_cast<float>(SCREEN_HEIGHT) * 0.3f,
		0.0f
	};
	warehouseOrigin_ = DirectX::XMFLOAT3{
		static_cast<float>(SCREEN_WIDTH) * 0.8f,
		static_cast<float>(SCREEN_HEIGHT) * 0.7f,
		0.0f
	};

	PlaceDemoField_();
	field_.InitAllVisuals(gfx_, rg_, combatFieldOrigin_);

	PlaceDemoWarehouse_();
	warehouse_.InitAllVisuals(gfx_, rg_, warehouseOrigin_);
}

void ModuleWorkbench::PlaceDemoField_()
{
	field_.AddNode<ModuleNode_Spawn_Ball_Core>(DirectX::XMFLOAT2{ 0.0f, 0.0f });

	field_.AddNode<ModuleNode_Other_Child>(DirectX::XMFLOAT2{ -70.0f, 40.0f });
	field_.AddNode<ModuleNode_Other_Child>(DirectX::XMFLOAT2{ 80.0f, 40.0f });

	field_.AddNode<ModuleNode_Spawn_Ball>(DirectX::XMFLOAT2{ -50.0f, -40.0f });
	field_.AddNode<ModuleNode_Spawn_Ball>(DirectX::XMFLOAT2{ 40.0f, -40.0f });

	field_.AddNode<ModuleNode_Rule_Orbit>(DirectX::XMFLOAT2{ 0.0f, 75.0f });
	field_.AddNode<ModuleNode_Rule_Orbit>(DirectX::XMFLOAT2{ 0.0f, -95.0f });

	field_.AddNode<ModuleNode_Attribute_Lifetime>(DirectX::XMFLOAT2{ 80.0f, 0.0f }, 2.0f);
	field_.AddNode<ModuleNode_Attribute_Lifetime>(DirectX::XMFLOAT2{ -110.0f, 0.0f }, 2.0f);

	field_.AddNode<ModuleNode_Attribute_SpeedRate>(DirectX::XMFLOAT2{ 30.0f, 90.0f }, 0.5f);
	field_.AddNode<ModuleNode_Attribute_SpeedRate>(DirectX::XMFLOAT2{ -50.0f, 90.0f }, 0.2f);
}

void ModuleWorkbench::PlaceDemoWarehouse_()
{
	const DirectX::XMFLOAT2 zero{ 0.0f, 0.0f };
	warehouse_.AddNode<ModuleNode_Spawn_Ball>(zero);
	warehouse_.AddNode<ModuleNode_Spawn_Ball>(zero);
	warehouse_.AddNode<ModuleNode_Other_Child>(zero);
	warehouse_.AddNode<ModuleNode_Other_Child>(zero);
	warehouse_.AddNode<ModuleNode_Rule_Orbit>(zero);
	warehouse_.AddNode<ModuleNode_Attribute_Lifetime>(zero, 2.0f);
	warehouse_.AddNode<ModuleNode_Attribute_SpeedRate>(zero, 0.5f);
	warehouse_.AddNode<ModuleNode_Attribute_SpeedRate>(zero, 0.2f);
}

void ModuleWorkbench::Reset()
{
	assembler_.Reset();
	field_.ResetAllCooldowns();
	field_.SyncAllVisuals();
	field_.ClearWaves();
}

void ModuleWorkbench::Update(float dt, AttackManager* attackManager)
{
	field_.TickAllCooldowns(dt);
	field_.SyncAllVisuals();

	Player* player = ObjectCodex::FindFirstActiveObjectByTag<Player>(character_Player);
	auto& input = InputCodex::Get();

	// RMB: force-finish scans and arm all pending shots (same-frame Take fires them).
	if (input.MouseRightTriggered())
	{
		assembler_.ForceFinish(field_);
	}
	else if (player != nullptr && player->GetIsAttack() && assembler_.CanStart(field_))
	{
		const DirectX::XMFLOAT3 spawnPos = player->GetPosition();
		DirectX::XMFLOAT3 aimVel{ 0.0f, 0.0f, 0.05f };
		if (attackManager != nullptr)
		{
			attackManager->TryGetAimVelocity(spawnPos, aimVel);
		}
		assembler_.Begin(field_, gfx_, rg_, spawnPos, aimVel);
	}

	assembler_.Update(dt, field_);

	// Only TakeAllPendingFires erases sessions; keep Update → Take → draw order.
	for (FireBatch& batch : assembler_.TakeAllPendingFires())
	{
		if (attackManager != nullptr && player != nullptr)
		{
			const DirectX::XMFLOAT3 pos = player->GetPosition();
			DirectX::XMFLOAT3 vel{ 0.0f, 0.0f, 0.05f };
			attackManager->TryGetAimVelocity(pos, vel);
			attackManager->FireRoots(std::move(batch.roots), pos, vel);
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

void ModuleWorkbench::BeginLayoutEdit()
{
	if (field_.GetCanvas() == nullptr)
	{
		return;
	}

	assembler_.Reset();
	field_.ClearWaves();
	field_.ResetAllCooldowns();

	std::array<IModuleZone*, ZoneCount()> zones{};
	zones[ToIndex(ZoneId::Field)] = &field_;
	zones[ToIndex(ZoneId::Warehouse)] = &warehouse_;

	std::array<DirectX::XMFLOAT3, ZoneCount()> origins{};
	origins[ToIndex(ZoneId::Field)] = layoutFieldOrigin_;
	origins[ToIndex(ZoneId::Warehouse)] = warehouseOrigin_;

	layoutEditor_.Begin(zones, origins, gfx_, rg_, combatFieldOrigin_);
}

void ModuleWorkbench::EndLayoutEdit()
{
	layoutEditor_.End();
}

void ModuleWorkbench::UpdateLayoutEdit(float dt, Window* hostWindow)
{
	layoutEditor_.Update(dt, hostWindow);
}

void ModuleWorkbench::Submit()
{
	// Global order: all zone backgrounds → all zone nodes → editor overlay.
	field_.SubmitBackground();
	if (layoutEditor_.IsActive())
	{
		warehouse_.SubmitBackground();
	}
	field_.SubmitNodes();
	if (layoutEditor_.IsActive())
	{
		warehouse_.SubmitNodes();
		layoutEditor_.SubmitOverlay();
	}
}

void ModuleWorkbench::SyncFieldWaves_()
{
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

	field_.SetWavesLocal(
		centers, radii, count, ModuleFieldCanvas::kDefaultFieldSide);
}
