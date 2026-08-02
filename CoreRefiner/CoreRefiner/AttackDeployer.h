#pragma once
#include "Attack.h"
#include "Ball.h"

#include "OrbitLocalModule.h"
#include "LifetimeModule.h"
#include "SpeedRateModule.h"

#include "ObjectCodex.h"
#include "Graphics.h"
#include "RenderGraph.h"
#include "Player.h"

#include <cmath>
#include <memory>
#include <utility>
#include <vector>

enum class DeployStepKind : unsigned char
{
	Spawn_Ball,
	Attribute_Lifetime,
	Attribute_SpeedRate,
	Rule_Orbit,
	Other_Child,
};

enum class DeployTarget : unsigned char
{
	Focus,
	ShotRoot,
};

struct AttackStandby
{
	Graphics* gfx{ nullptr };
	Rgph::RenderGraph* rg{ nullptr };
	DirectX::XMFLOAT3 spawnPos{ 0.0f, 0.0f, 0.0f };
	float childDistributeRadius{ 2.0f };

	Attack* parent{ nullptr };
	std::vector<Attack*> children;
	Attack* host{ nullptr };
};

struct DeployContext
{
	AttackStandby standby;
	std::vector<Attack*> shots;

	void FlushStandby()
	{
		if (standby.parent != nullptr)
		{
			shots.push_back(standby.parent);
		}
		standby.parent = nullptr;
		standby.children.clear();
		standby.host = nullptr;
	}
};

class IDeployStep
{
public:
	virtual ~IDeployStep() = default;
	virtual void Apply(DeployContext& ctx) = 0;
	[[nodiscard]] virtual DeployStepKind GetKind() const noexcept = 0;
	[[nodiscard]] virtual const char* GetName() const noexcept = 0;
	[[nodiscard]] virtual bool HasModule() const noexcept { return false; }
	[[nodiscard]] virtual DeployTarget GetTarget() const noexcept { return DeployTarget::Focus; }

protected:
	IDeployStep() = default;
};

struct AttackRecipe
{
	std::vector<std::unique_ptr<IDeployStep>> steps;
	bool parentRootToPlayer{ false };
};

inline void RedistributeChildrenEvenly(AttackStandby& s)
{
	const std::size_t n = s.children.size();
	if (n == 0)
	{
		return;
	}

	const float radius = s.childDistributeRadius;
	for (std::size_t i = 0; i < n; ++i)
	{
		Attack* child = s.children[i];
		if (child == nullptr)
		{
			continue;
		}

		const float phase = DirectX::XM_2PI * static_cast<float>(i) / static_cast<float>(n);
		child->SetLocalPosition({
			radius * std::cos(phase),
			0.0f,
			radius * std::sin(phase)
			});

		if (OrbitLocalModule* orbit = child->GetModule<OrbitLocalModule>())
		{
			orbit->SetRadius(radius);
			orbit->SetPhase0(phase);
		}
	}
}
class DeployStep_Spawn_Ball final : public IDeployStep
{
public:
	explicit DeployStep_Spawn_Ball(
		DirectX::XMFLOAT3 scale = { 1.0f, 1.0f, 1.0f },
		bool enableCollider = true) noexcept
		:
		scale_(scale),
		enableCollider_(enableCollider)
	{}

	[[nodiscard]] DeployStepKind GetKind() const noexcept override { return DeployStepKind::Spawn_Ball; }
	[[nodiscard]] const char* GetName() const noexcept override { return "Spawn_Ball"; }

	void Apply(DeployContext& ctx) override
	{
		AttackStandby& s = ctx.standby;
		if (s.gfx == nullptr || s.rg == nullptr)
		{
			return;
		}

		if (s.host != nullptr)
		{
			ctx.FlushStandby();
		}

		Ball* ball = ObjectCodex::SpawnPooled<Ball>(
			attack_Ball,
			*s.gfx,
			*s.rg,
			s.spawnPos,
			DirectX::XMFLOAT3{ 0.0f, 0.0f, 0.0f });
		if (ball == nullptr)
		{
			return;
		}

		ball->ClearParent();
		ball->ClearModules();
		ball->SetMoveAccel({ 0.0f, 0.0f, 0.0f });
		ball->ResetMoveVelocity();
		ball->ApplyPresentation(scale_, enableCollider_);

		if (s.parent == nullptr)
		{
			s.parent = ball;
			s.host = ball;
			return;
		}

		ball->SetParent(s.parent);
		s.children.push_back(ball);
		s.host = ball;
		RedistributeChildrenEvenly(s);
	}

	static std::unique_ptr<DeployStep_Spawn_Ball> Make(
		DirectX::XMFLOAT3 scale = { 1.0f, 1.0f, 1.0f },
		bool enableCollider = true)
	{
		return std::make_unique<DeployStep_Spawn_Ball>(scale, enableCollider);
	}

private:
	DirectX::XMFLOAT3 scale_{ 1.0f, 1.0f, 1.0f };
	bool enableCollider_{ true };
};

class DeployStep_Other_Child final : public IDeployStep
{
public:
	[[nodiscard]] DeployStepKind GetKind() const noexcept override { return DeployStepKind::Other_Child; }
	[[nodiscard]] const char* GetName() const noexcept override { return "Other_Child"; }

	void Apply(DeployContext& ctx) override
	{
		AttackStandby& s = ctx.standby;
		if (s.parent == nullptr)
		{
			return;
		}
		if (s.host == nullptr)
		{
			return;
		}
		s.host = nullptr;
	}

	static std::unique_ptr<DeployStep_Other_Child> Make()
	{
		return std::make_unique<DeployStep_Other_Child>();
	}
};

class DeployStep_Attribute_Lifetime final : public IDeployStep
{
public:
	explicit DeployStep_Attribute_Lifetime(float durationSeconds) noexcept
		:
		durationSeconds_(durationSeconds)
	{}

	[[nodiscard]] DeployStepKind GetKind() const noexcept override { return DeployStepKind::Attribute_Lifetime; }
	[[nodiscard]] const char* GetName() const noexcept override { return "Attribute_Lifetime"; }
	[[nodiscard]] bool HasModule() const noexcept override { return true; }
	[[nodiscard]] DeployTarget GetTarget() const noexcept override { return DeployTarget::ShotRoot; }

	void Apply(DeployContext& ctx) override
	{
		if (ctx.standby.parent != nullptr)
		{
			ctx.standby.parent->AddModule<LifetimeModule>(durationSeconds_);
		}
	}

	static std::unique_ptr<DeployStep_Attribute_Lifetime> Make(float durationSeconds)
	{
		return std::make_unique<DeployStep_Attribute_Lifetime>(durationSeconds);
	}

private:
	float durationSeconds_{ 2.0f };
};

class DeployStep_Attribute_SpeedRate final : public IDeployStep
{
public:
	explicit DeployStep_Attribute_SpeedRate(float speedRate) noexcept 
		: speedRate_{ speedRate }
	{}

	[[nodiscard]] DeployStepKind GetKind() const noexcept override { return DeployStepKind::Attribute_SpeedRate; }
	[[nodiscard]] const char* GetName() const noexcept override { return "Attribute_SpeedRate"; }
	[[nodiscard]] bool HasModule() const noexcept override { return true; }
	[[nodiscard]] DeployTarget GetTarget() const noexcept override { return DeployTarget::ShotRoot; }

	void Apply(DeployContext& ctx) override
	{
		if (ctx.standby.parent != nullptr)
		{
			ctx.standby.parent->AddModule<SpeedRateModule>(speedRate_);
		}
	}

	static std::unique_ptr<DeployStep_Attribute_SpeedRate> Make(float speedRate)
	{
		return std::make_unique<DeployStep_Attribute_SpeedRate>(speedRate);
	}

private:
	float speedRate_{ 1.0f };
};


class DeployStep_Rule_Orbit final : public IDeployStep
{
public:
	DeployStep_Rule_Orbit(float radius, float angularSpeed, float phase = -1.0f) noexcept
		:
		orbitRadius_(radius),
		orbitAngularSpeed_(angularSpeed),
		orbitPhase_(phase)
	{}

	[[nodiscard]] DeployStepKind GetKind() const noexcept override { return DeployStepKind::Rule_Orbit; }
	[[nodiscard]] const char* GetName() const noexcept override { return "Rule_Orbit"; }
	[[nodiscard]] bool HasModule() const noexcept override { return true; }

	void Apply(DeployContext& ctx) override
	{
		AttackStandby& s = ctx.standby;
		if (s.host == nullptr || s.host == s.parent)
		{
			return;
		}

		s.childDistributeRadius = orbitRadius_;

		float phase = orbitPhase_;
		if (phase < 0.0f)
		{
			phase = 0.0f;
			const std::size_t n = s.children.size();
			for (std::size_t i = 0; i < n; ++i)
			{
				if (s.children[i] == s.host)
				{
					phase = (n > 0)
						? (DirectX::XM_2PI * static_cast<float>(i) / static_cast<float>(n))
						: 0.0f;
					break;
				}
			}
		}

		s.host->AddModule<OrbitLocalModule>(orbitRadius_, orbitAngularSpeed_, phase);
	}

	static std::unique_ptr<DeployStep_Rule_Orbit> Make(
		float radius,
		float angularSpeed,
		float phase = -1.0f)
	{
		return std::make_unique<DeployStep_Rule_Orbit>(radius, angularSpeed, phase);
	}

private:
	float orbitRadius_{ 2.0f };
	float orbitAngularSpeed_{ 3.5f };
	float orbitPhase_{ -1.0f };
};

class AttackDeployer
{
public:
	static std::vector<Attack*> Deploy(
		const AttackRecipe& recipe,
		Graphics& gfx,
		Rgph::RenderGraph& rg,
		DirectX::XMFLOAT3 pos,
		Player* player)
	{
		DeployContext ctx{};
		ctx.standby.gfx = &gfx;
		ctx.standby.rg = &rg;
		ctx.standby.spawnPos = pos;

		for (const auto& step : recipe.steps)
		{
			if (step != nullptr)
			{
				step->Apply(ctx);
			}
		}

		ctx.FlushStandby();

		if (recipe.parentRootToPlayer && player != nullptr)
		{
			for (Attack* root : ctx.shots)
			{
				if (root != nullptr)
				{
					root->SetParent(player);
				}
			}
		}

		return ctx.shots;
	}
};
