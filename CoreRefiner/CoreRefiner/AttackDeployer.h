#pragma once
#include "Attack.h"
#include "Ball.h"
#include "ModuleNodeLabel.h"

#include "Rule_Orbit_Module.h"
#include "Attribute_Lifetime_Module.h"
#include "Attribute_SpeedRate_Module.h"

#include "ObjectCodex.h"
#include "Graphics.h"
#include "RenderGraph.h"
#include "Player.h"
#include "XMath.h"

#include <cmath>
#include <functional>
#include <memory>
#include <tuple>
#include <utility>
#include <vector>

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
	// 当前空子坑上等待 host 填入后再执行的安装
	std::vector<std::function<void(DeployContext&)>> pitQueue;

	void FlushStandby()
	{
		if (standby.parent != nullptr)
		{
			shots.push_back(standby.parent);
		}
		standby.parent = nullptr;
		standby.children.clear();
		standby.host = nullptr;
		pitQueue.clear();
	}

	
	//把安装动作挂到当前空子坑；球体填坑后 DrainPitQueue 再跑。
	void EnqueueOnEmptyPit(std::function<void(DeployContext&)> job)
	{
		pitQueue.push_back(std::move(job));
	}

	//对当前 host 执行坑队列（先移出再跑，避免 Drain 中再次入队套娃）。
	void DrainPitQueue()
	{
		auto jobs = std::move(pitQueue);
		pitQueue.clear();
		for (auto& job : jobs)
		{
			if (job)
			{
				job(*this);
			}
		}
	}
};

// 打 standby.host
// host 空且已开子坑则入队，等填坑后再装。
template <typename T, typename... Args>
T* AddToFocus(DeployContext& ctx, Args&&... args)
{
	if (ctx.standby.host == nullptr)
	{
		if (ctx.standby.parent != nullptr)
		{
			ctx.EnqueueOnEmptyPit(
				[captured = std::make_tuple(std::forward<Args>(args)...)](DeployContext& c) mutable
				{
					std::apply([&c](auto&&... a)
					{
						AddToFocus<T>(c, std::forward<decltype(a)>(a)...);
					}, std::move(captured));
				});
		}
		return nullptr;
	}
	return ctx.standby.host->AddModule<T>(std::forward<Args>(args)...);
}

class IAttackNodeStep
{
public:
	virtual ~IAttackNodeStep() = default;
	virtual void Apply(DeployContext& ctx) = 0;
	[[nodiscard]] virtual ModuleNodeLabel GetModuleNodeLabel() const noexcept = 0;
	[[nodiscard]] virtual const char* GetName() const noexcept = 0;
	[[nodiscard]] virtual bool HasModule() const noexcept { return false; }
	[[nodiscard]] virtual DeployTarget GetTarget() const noexcept { return DeployTarget::Focus; }

protected:
	IAttackNodeStep() = default;
};

struct AttackRecipe
{
	std::vector<std::unique_ptr<IAttackNodeStep>> steps;
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
		child->SetLocalPosition(Vec3{
			radius * std::cos(phase),
			0.0f,
			radius * std::sin(phase)
		});

		if (Rule_Orbit_Module* orbit = child->GetModule<Rule_Orbit_Module>())
		{
			orbit->SetRadius(radius);
			orbit->SetPhase0(phase);
		}
	}
}

class AttackNodeStep_Spawn_Ball final : public IAttackNodeStep
{
public:
	explicit AttackNodeStep_Spawn_Ball(
		DirectX::XMFLOAT3 scale = { 1.0f, 1.0f, 1.0f },
		bool enableCollider = true) noexcept
		:
		scale_(scale),
		enableCollider_(enableCollider)
	{}

	[[nodiscard]] ModuleNodeLabel GetModuleNodeLabel() const noexcept override
	{
		return ModuleNodeLabel::Spawn_Ball;
	}
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

		ball->ResetHierarchy();
		ball->ClearModules();
		ball->SetMoveAccel({ 0.0f, 0.0f, 0.0f });
		ball->ResetMoveVelocity();
		ball->ApplyPresentation(scale_, enableCollider_);

		if (s.parent == nullptr || ball == s.parent)
		{
			s.parent = ball;
			s.host = ball;
			return;
		}

		ball->SetParent(s.parent);
		s.children.push_back(ball);
		s.host = ball;
		ctx.DrainPitQueue();
		RedistributeChildrenEvenly(s);
	}

	static std::unique_ptr<AttackNodeStep_Spawn_Ball> Make(
		DirectX::XMFLOAT3 scale = { 1.0f, 1.0f, 1.0f },
		bool enableCollider = true)
	{
		return std::make_unique<AttackNodeStep_Spawn_Ball>(scale, enableCollider);
	}

private:
	DirectX::XMFLOAT3 scale_{ 1.0f, 1.0f, 1.0f };
	bool enableCollider_{ true };
};

class AttackNodeStep_Other_Child final : public IAttackNodeStep
{
public:
	[[nodiscard]] ModuleNodeLabel GetModuleNodeLabel() const noexcept override
	{
		return ModuleNodeLabel::Other_Child;
	}
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

	static std::unique_ptr<AttackNodeStep_Other_Child> Make()
	{
		return std::make_unique<AttackNodeStep_Other_Child>();
	}
};

class AttackNodeStep_Attribute_Lifetime final : public IAttackNodeStep
{
public:
	explicit AttackNodeStep_Attribute_Lifetime(float durationSeconds) noexcept
		:
		durationSeconds_(durationSeconds)
	{}

	[[nodiscard]] ModuleNodeLabel GetModuleNodeLabel() const noexcept override
	{
		return ModuleNodeLabel::Attribute_Lifetime;
	}
	[[nodiscard]] const char* GetName() const noexcept override { return "Attribute_Lifetime"; }
	[[nodiscard]] bool HasModule() const noexcept override { return true; }
	[[nodiscard]] DeployTarget GetTarget() const noexcept override { return DeployTarget::Focus; }

	void Apply(DeployContext& ctx) override
	{
		AddToFocus<Attribute_Lifetime_Module>(ctx, durationSeconds_);
	}

	static std::unique_ptr<AttackNodeStep_Attribute_Lifetime> Make(float durationSeconds)
	{
		return std::make_unique<AttackNodeStep_Attribute_Lifetime>(durationSeconds);
	}

private:
	float durationSeconds_{ 2.0f };
};

class AttackNodeStep_Attribute_SpeedRate final : public IAttackNodeStep
{
public:
	explicit AttackNodeStep_Attribute_SpeedRate(float speedRate) noexcept
		: speedRate_{ speedRate }
	{}

	[[nodiscard]] ModuleNodeLabel GetModuleNodeLabel() const noexcept override
	{
		return ModuleNodeLabel::Attribute_SpeedRate;
	}
	[[nodiscard]] const char* GetName() const noexcept override { return "Attribute_SpeedRate"; }
	[[nodiscard]] bool HasModule() const noexcept override { return true; }
	[[nodiscard]] DeployTarget GetTarget() const noexcept override { return DeployTarget::Focus; }

	void Apply(DeployContext& ctx) override
	{
		AddToFocus<Attribute_SpeedRate_Module>(ctx, speedRate_);
	}

	static std::unique_ptr<AttackNodeStep_Attribute_SpeedRate> Make(float speedRate)
	{
		return std::make_unique<AttackNodeStep_Attribute_SpeedRate>(speedRate);
	}

private:
	float speedRate_{ 1.0f };
};

class AttackNodeStep_Rule_Orbit final : public IAttackNodeStep
{
public:
	AttackNodeStep_Rule_Orbit(float radius, float phase = -1.0f) noexcept
		:
		orbitRadius_(radius),
		orbitPhase_(phase)
	{}

	[[nodiscard]] ModuleNodeLabel GetModuleNodeLabel() const noexcept override
	{
		return ModuleNodeLabel::Rule_Orbit;
	}
	[[nodiscard]] const char* GetName() const noexcept override { return "Rule_Orbit"; }
	[[nodiscard]] bool HasModule() const noexcept override { return true; }

	void Apply(DeployContext& ctx) override
	{
		AttackStandby& s = ctx.standby;
		if (s.host == s.parent && s.host != nullptr)
		{
			return;
		}
		if (s.host == nullptr)
		{
			if (s.parent == nullptr)
			{
				return;
			}
			const float radius = orbitRadius_;
			const float phase = orbitPhase_;
			ctx.EnqueueOnEmptyPit([radius, phase](DeployContext& c)
			{
				AttackNodeStep_Rule_Orbit(radius, phase).Apply(c);
			});
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
		s.host->AddModule<Rule_Orbit_Module>(orbitRadius_, phase);
	}

	static std::unique_ptr<AttackNodeStep_Rule_Orbit> Make(
		float radius,
		float phase = -1.0f)
	{
		return std::make_unique<AttackNodeStep_Rule_Orbit>(radius, phase);
	}

private:
	float orbitRadius_{ 2.0f };
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
