#pragma once
#include "Graphics.h"
#include "RenderGraph.h"
#include "Canvas2D.h"
#include "ModuleField.h"
#include "ModuleFieldDraw.h"
#include "FieldNodes.h"
#include "ScanAssembler.h"
#include "ObjectCodex.h"
#include "Player.h"
#include "Colors.h"
#include "AttackManager.h"

#include "Channels.h"

#include <cmath>

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
			constexpr float side = 300.0f;
			bgCanvas = std::make_unique<Canvas2D>(gfx, 300u, 300u);
			bgCanvas->SetPosition(fieldOrigin_);
			bgCanvas->SetScale(DirectX::XMFLOAT3{ side, side, 1.0f });
			bgCanvas->Clear(Color(100u, 150u, 50u, 150u));
			bgCanvas->NotifyPixelsChanged();
			bgCanvas->LinkTechniques(rg);
		}

		PlaceDemoField_();
		field_.InitAllVisuals(gfx_, rg_, fieldOrigin_);
	}
	~UI_Game() = default;

	void SetAttackManager(AttackManager* manager) noexcept { attackManager_ = manager; }

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

		if (bgCanvas != nullptr)
		{
			// Clear trails from pixel rings until ModuleFieldCanvas (phase B).
			bgCanvas->Clear(Color(100u, 150u, 50u, 150u));
			DrawWaveOverlay_(*bgCanvas);
		}
	}

	void Submit(void)
	{
		if (bgCanvas != nullptr)
		{
			bgCanvas->Submit(Chan::ui);
		}
		field_.SubmitAllVisuals();
	}

	[[nodiscard]] ModuleField& GetField() noexcept { return field_; }
	[[nodiscard]] const ModuleField& GetField() const noexcept { return field_; }
	[[nodiscard]] ScanAssembler& GetAssembler() noexcept { return assembler_; }

private:
	void PlaceDemoField_()
	{
		field_.AddNode<CoreSpawnNode>(DirectX::XMFLOAT2{ 0.0f, 0.0f });

		field_.AddNode<ChildPitNode>(DirectX::XMFLOAT2{ -70.0f, 40.0f });
		field_.AddNode<ChildPitNode>(DirectX::XMFLOAT2{ 80.0f, 40.0f });

		field_.AddNode<SpawnBallNode>(DirectX::XMFLOAT2{ -50.0f, -40.0f });
		field_.AddNode<SpawnBallNode>(DirectX::XMFLOAT2{ 40.0f, -40.0f });

		field_.AddNode<OrbitNode>(DirectX::XMFLOAT2{ 0.0f, 75.0f });
		field_.AddNode<OrbitNode>(DirectX::XMFLOAT2{ 0.0f, -95.0f });

		field_.AddNode<LifetimeNode>(DirectX::XMFLOAT2{ 80.0f, 0.0f }, 2.0f);
		field_.AddNode<LifetimeNode>(DirectX::XMFLOAT2{ -110.0f, 0.0f }, 2.0f);

		field_.AddNode<SpeedRateNode>(DirectX::XMFLOAT2{ 30.0f, 90.0f }, 0.5f);
		field_.AddNode<SpeedRateNode>(DirectX::XMFLOAT2{ -50.0f, 90.0f }, 0.2f);
	}

	void DrawWaveOverlay_(Canvas2D& bg) const
	{
		bool any = false;
		assembler_.ForEachAliveWave([&](const ScanWave& wave)
		{
			if (wave.radius <= 0.0f)
			{
				return;
			}

			int cx = 0;
			int cy = 0;
			ModuleFieldDraw::LocalToPixel(
				wave.center.x, wave.center.y,
				bg.GetCanvasWidth(), bg.GetCanvasHeight(),
				cx, cy);

			const int r = static_cast<int>(std::lround(wave.radius));
			ModuleFieldDraw::DrawRing(bg, cx, cy, r, 3, Color(255u, 255u, 80u, 255u));
			ModuleFieldDraw::DrawRing(bg, cx, cy, r + 2, 1, Color(255u, 255u, 200u, 180u));
			any = true;
		});

		bg.NotifyPixelsChanged();
		(void)any;
	}

	Graphics& gfx_;
	Rgph::RenderGraph& rg_;
	AttackManager* attackManager_{ nullptr };
	DirectX::XMFLOAT3 fieldOrigin_{ 0.0f, 0.0f, 0.0f };
	std::unique_ptr<Canvas2D> bgCanvas;
	ModuleField field_;
	ScanAssembler assembler_;
};
