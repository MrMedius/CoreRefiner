#pragma once
#include "Graphics.h"
#include "RenderGraph.h"
#include "ModuleField.h"
#include "ModuleFieldCanvas.h"
#include "FieldNodes.h"
#include "ScanAssembler.h"
#include "FieldLayoutEditor.h"
#include "ObjectCodex.h"
#include "Player.h"
#include "Colors.h"
#include "AttackManager.h"
#include "InputCodex.h"

#include "Channels.h"

class Window;

class UI_Game
{
public:
	UI_Game(Graphics& gfx, Rgph::RenderGraph& rg)
		:
		gfx_(gfx),
		rg_(rg)
	{
		fieldOrigin_ = DirectX::XMFLOAT3{ 200.0f, 200.0f, 0.0f };
		combatFieldOrigin_ = fieldOrigin_;

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
	 * @brief Host window for letterbox-aware cursor snap during layout edit.
	 */
	void SetHostWindow(Window* window) noexcept { hostWindow_ = window; }

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

	/**
	 * @brief Enter pause layout-edit via FieldLayoutEditor (also Ready-all / clear masks).
	 */
	void BeginLayoutEdit()
	{
		if (fieldCanvas_ == nullptr)
		{
			return;
		}
		layoutEditor_.Begin(
			field_,
			assembler_,
			*fieldCanvas_,
			gfx_,
			rg_,
			combatFieldOrigin_);
		fieldOrigin_ = DirectX::XMFLOAT3{
			static_cast<float>(SCREEN_WIDTH) * 0.5f,
			static_cast<float>(SCREEN_HEIGHT) * 0.5f,
			0.0f
		};
	}

	/**
	 * @brief Leave layout-edit and keep current node positions.
	 */
	void EndLayoutEdit()
	{
		if (fieldCanvas_ == nullptr)
		{
			return;
		}
		layoutEditor_.End(field_, *fieldCanvas_);
		fieldOrigin_ = combatFieldOrigin_;
	}

	/**
	 * @brief Pause-only tick: layout editor only (no cooldown / scan / fire).
	 * @note Esc cancel is handled inside FieldLayoutEditor::Update.
	 */
	void UpdateLayoutEdit(float dt)
	{
		layoutEditor_.Update(dt, field_, hostWindow_);
	}

	void Submit(void)
	{
		if (fieldCanvas_ != nullptr)
		{
			fieldCanvas_->Submit(Chan::ui);
		}
		field_.SubmitAllVisuals();
		if (layoutEditor_.IsActive())
		{
			layoutEditor_.SubmitOverlay();
		}
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
	Window* hostWindow_{ nullptr };
	DirectX::XMFLOAT3 fieldOrigin_{ 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 combatFieldOrigin_{ 200.0f, 200.0f, 0.0f };
	std::unique_ptr<ModuleFieldCanvas> fieldCanvas_;
	ModuleField field_;
	ScanAssembler assembler_;
	FieldLayoutEditor layoutEditor_;
};
