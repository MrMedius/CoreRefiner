#pragma once
#include "Graphics.h"
#include "Canvas2D.h"
#include "ModuleField.h"
#include "FieldNodes.h"

#include "Channels.h"

class UI_Game
{
public:
	UI_Game(Graphics& gfx, Rgph::RenderGraph& rg)
	{
		{
			float side = 300.0f;
			bgCanvas = std::make_unique<Canvas2D>(gfx, 300.0f, 300.0f);
			bgCanvas->SetPosition(DirectX::XMFLOAT3{ 200.0f, 200.0f, 0.0f });
			bgCanvas->SetScale(DirectX::XMFLOAT3{ 300.0f, 300.0f, 1.0f });
			for (float i = 0; i < side; i++)
				for (float j = 0; j < side; j++)
					bgCanvas->PutPixel(i, j, Color(100u, 150u, 50u, 150u));
			bgCanvas->LinkTechniques(rg);
		}

		PlaceDemoField_();
	}
	~UI_Game() = default;

	void Update(float dt)
	{
		field_.TickAllCooldowns(dt);
		if (bgCanvas != nullptr)
		{
			field_.Redraw(*bgCanvas);
		}
	}

	void Submit(void)
	{
		bgCanvas->Submit(Chan::ui);
	}

	[[nodiscard]] ModuleField& GetField() noexcept { return field_; }
	[[nodiscard]] const ModuleField& GetField() const noexcept { return field_; }

private:
	/** @brief Demo layout: 1 core + 1~2 of each satellite token. */
	void PlaceDemoField_()
	{
		field_.AddNode<CoreSpawnNode>(DirectX::XMFLOAT2{ 0.0f, 0.0f });

		field_.AddNode<ChildPitNode>(DirectX::XMFLOAT2{ -70.0f, 40.0f });
		field_.AddNode<ChildPitNode>(DirectX::XMFLOAT2{ 70.0f, 40.0f });

		field_.AddNode<SpawnBallNode>(DirectX::XMFLOAT2{ -70.0f, -40.0f });
		field_.AddNode<SpawnBallNode>(DirectX::XMFLOAT2{ 70.0f, -40.0f });

		field_.AddNode<OrbitNode>(DirectX::XMFLOAT2{ 0.0f, 75.0f });
		field_.AddNode<OrbitNode>(DirectX::XMFLOAT2{ 0.0f, -75.0f });

		field_.AddNode<LifetimeNode>(DirectX::XMFLOAT2{ 90.0f, 0.0f }, 2.0f);
		field_.AddNode<LifetimeNode>(DirectX::XMFLOAT2{ -90.0f, 0.0f }, 2.0f);

		field_.AddNode<SpeedRateNode>(DirectX::XMFLOAT2{ 50.0f, 90.0f }, 0.5f);
		field_.AddNode<SpeedRateNode>(DirectX::XMFLOAT2{ -50.0f, 90.0f }, 0.2f);
	}

	std::unique_ptr<Canvas2D> bgCanvas;
	ModuleField field_;
};
