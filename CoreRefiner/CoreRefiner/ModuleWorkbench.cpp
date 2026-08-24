#include "ModuleWorkbench.h"
#include "AttackManager.h"
#include "ButtonCanvasComponent.h"
#include "Channels.h"
#include "Graphics.h"
#include "ModuleNodes.h"
#include "IModuleZone.h"
#include "InputCodex.h"
#include "ObjectCodex.h"
#include "Player.h"
#include "TextTypes.h"
#include "UiRoot.h"
#include "Win.h"
#include "Window.h"

#include <array>
#include <string>

ModuleWorkbench::ModuleWorkbench(Graphics& gfx, Rgph::RenderGraph& rg)
	:
	gfx_(gfx),
	rg_(rg)
{
	// 战斗 Field：0.5 Scale 后视觉半宽 75，贴右下，边距 32。
	constexpr float kCombatMargin = 32.0f;
	const float combatHalf =
		ModuleField::kHalfExtent * ModuleField::kCombatVisualScale;
	combatFieldOrigin_ = DirectX::XMFLOAT3{
		static_cast<float>(SCREEN_WIDTH) - kCombatMargin - combatHalf,
		static_cast<float>(SCREEN_HEIGHT) - kCombatMargin - combatHalf,
		0.0f
	};
	ComputeLayout_();

	PlaceDemoField_();
	field_.InitAllVisuals(gfx_, rg_, combatFieldOrigin_);
	field_.SetVisualScale(ModuleField::kCombatVisualScale);

	PlaceDemoWarehouse_();
	warehouse_.InitAllVisuals(gfx_, rg_, warehouseOrigin_);

	shop_.InitAllVisuals(gfx_, rg_, shopOrigin_);
	InitFightButton_();
}

ModuleWorkbench::~ModuleWorkbench() = default;

void ModuleWorkbench::ComputeLayout_() noexcept
{
	const float screenW = static_cast<float>(SCREEN_WIDTH);
	const float screenH = static_cast<float>(SCREEN_HEIGHT);
	/** 与 IModuleZone::kShellPad 相同；基类该常量是 protected。 */
	constexpr float kShellPad = 12.0f;

	const float fieldOuterHalf = ModuleField::kHalfExtent + kShellPad;
	const float fieldOuterH = fieldOuterHalf * 2.0f;
	const float fieldOuterW = fieldOuterH;

	const float warehouseHalfX =
		(static_cast<float>(ModuleWarehouse::kColumns - 1) * 0.5f) * ModuleWarehouse::kSlotPitch
		+ ModuleWarehouse::kBoundsPad;
	const float warehouseHalfY =
		(static_cast<float>(ModuleWarehouse::kMaxRows - 1) * 0.5f) * ModuleWarehouse::kSlotPitch
		+ ModuleWarehouse::kBoundsPad;
	const float warehouseSpanY =
		(static_cast<float>(ModuleWarehouse::kMaxRows - 1) * 0.5f) * ModuleWarehouse::kSlotPitch;
	const float warehouseOuterHalfX = warehouseHalfX + kShellPad;
	const float warehouseOuterHalfY = warehouseHalfY + kShellPad;
	const float warehouseOuterH = warehouseOuterHalfY * 2.0f;
	const float warehouseOuterW = warehouseOuterHalfX * 2.0f;
	const float rightOuterW = (fieldOuterW > warehouseOuterW) ? fieldOuterW : warehouseOuterW;

	/** 顶 / Field–仓 / 仓–按钮 / 底，以及左右与列间，共用同一 gap。 */
	const float gap = (screenH - fieldOuterH - warehouseOuterH - kFightBtnH) * 0.25f;
	const float rightLeft = screenW - gap - rightOuterW;
	const float rightCx = rightLeft + rightOuterW * 0.5f;

	layoutFieldOrigin_ = DirectX::XMFLOAT3{
		rightCx,
		gap + fieldOuterHalf,
		0.0f
	};

	const float warehouseShellTop = gap + fieldOuterH + gap;
	warehouseOrigin_ = DirectX::XMFLOAT3{
		rightCx,
		warehouseShellTop + warehouseOuterHalfY - warehouseSpanY,
		0.0f
	};

	const float warehouseShellBottom = warehouseShellTop + warehouseOuterH;
	fightBtnCenter_ = DirectX::XMFLOAT2{
		rightCx,
		warehouseShellBottom + gap + kFightBtnH * 0.5f
	};
	if (fightBtn_ != nullptr)
	{
		fightBtn_->SetLayoutLogicalCenterSize(
			fightBtnCenter_.x, fightBtnCenter_.y, kFightBtnW, kFightBtnH);
	}

	const float shopLeft = gap;
	const float shopRight = rightLeft - gap;
	const float shopTop = gap;
	const float shopBottom = fightBtnCenter_.y + kFightBtnH * 0.5f;
	const float shopHalfX = (shopRight - shopLeft) * 0.5f;
	const float shopHalfY = (shopBottom - shopTop) * 0.5f;
	shopOrigin_ = DirectX::XMFLOAT3{
		shopLeft + shopHalfX,
		shopTop + shopHalfY,
		0.0f
	};
	shop_.SetShellExtent(shopHalfX, shopHalfY);
}

void ModuleWorkbench::InitFightButton_()
{
	Ui::ButtonCanvasStyle style{};
	style.primaryFont = Text::FontSource::System(L"Microsoft YaHei UI");
	style.fontSize = 22.0f;

	fightBtn_ = std::make_unique<Ui::ButtonCanvasComponent>(
		gfx_, 601u, fightBtnCenter_.x, fightBtnCenter_.y, kFightBtnW, kFightBtnH, style);
	fightBtn_->Button().SetLabel("战斗！(第 1 波)");
	fightBtn_->Button().SetOnClick([this] {
		if (onFight_)
		{
			onFight_();
		}
	});

	uiRoot_ = std::make_unique<Ui::UiRoot>();
	uiRoot_->Clear();
	fightBtn_->RegisterTo(*uiRoot_);
	uiRoot_->RebuildTabOrder();
	uiRoot_->InitLinkTechniques(rg_);
}

void ModuleWorkbench::SetNextWave(int wave)
{
	if (wave < 1)
	{
		wave = 1;
	}
	if (wave == paintedWave_ || fightBtn_ == nullptr)
	{
		return;
	}
	paintedWave_ = wave;
	fightBtn_->Button().SetLabel("战斗！(第 " + std::to_string(wave) + " 波)");
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
{}

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
	field_.SetVisualScale(1.0f);

	std::array<IModuleZone*, ZoneCount()> zones{};
	zones[ToIndex(ZoneId::Field)] = &field_;
	zones[ToIndex(ZoneId::Warehouse)] = &warehouse_;
	zones[ToIndex(ZoneId::Shop)] = &shop_;

	std::array<DirectX::XMFLOAT3, ZoneCount()> origins{};
	origins[ToIndex(ZoneId::Field)] = layoutFieldOrigin_;
	origins[ToIndex(ZoneId::Warehouse)] = warehouseOrigin_;
	origins[ToIndex(ZoneId::Shop)] = shopOrigin_;

	layoutEditor_.Begin(zones, origins, gfx_, rg_, combatFieldOrigin_);
}

void ModuleWorkbench::EndLayoutEdit()
{
	field_.SetVisualScale(ModuleField::kCombatVisualScale);
	layoutEditor_.End();
}

void ModuleWorkbench::UpdateLayoutEdit(float dt, Window* hostWindow)
{
	if (uiRoot_ != nullptr)
	{
		uiRoot_->UpdateAfterInput();
	}
	layoutEditor_.Update(dt, hostWindow);
}

void ModuleWorkbench::SubmitField()
{
	field_.SubmitBackground();
	field_.SubmitNodes();
}

void ModuleWorkbench::SubmitPrep()
{
	field_.SubmitBackground();
	if (layoutEditor_.IsActive())
	{
		warehouse_.SubmitBackground();
		shop_.SubmitBackground();
	}
	field_.SubmitNodes();
	if (layoutEditor_.IsActive())
	{
		warehouse_.SubmitNodes();
		shop_.SubmitNodes();
		shop_.SubmitHud();
		layoutEditor_.SubmitOverlay();
	}
	if (uiRoot_ != nullptr)
	{
		uiRoot_->Submit(Chan::ui);
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