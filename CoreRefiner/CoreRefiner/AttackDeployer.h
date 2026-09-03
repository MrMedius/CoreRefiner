#pragma once
#include "Attack.h"
#include "Ball.h"
#include "ModuleNodeLabel.h"

#include "Rule_Orbit_Module.h"
#include "Rule_Return_Module.h"
#include "Attribute_Lifetime_Module.h"
#include "Attribute_SpeedRate_Module.h"
#include "Attribute_SizeRate_Module.h"
#include "Attribute_DamageRate_Module.h"
#include "Other_Revive_Module.h"

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

	Attack* parent{ nullptr };
	std::vector<Attack*> children;
	Attack* host{ nullptr };
	Player* player{ nullptr };
	/** @brief 当前 parent 是停放克隆，FlushStandby 不得把它当根弹发射。 */
	bool parked{ false };
};

struct DeployContext
{
	AttackStandby standby;
	std::vector<Attack*> shots;
	/** @brief 当前未 Flush 的流水线 Step；FlushStandby 时封到根弹。 */
	std::vector<AttackStepRecord> recipe;
	// 当前空子坑上等待 host 填入后再执行的安装
	std::vector<std::function<void(DeployContext&)>> pitQueue;
	/**
	 * @brief 扫到 Revive 之后为 true：后续 Step 只 Record，不改当前树。
	 * @note 由 Other_Revive 的 Apply 置位；新 Context 从 false 开始。
	 */
	bool recordOnly{ false };

	/**
	 * @brief 记下一条已成功 Apply 的 Step。
	 */
	void Record(const AttackStepRecord& rec)
	{
		recipe.push_back(rec);
	}

	/**
	 * @brief Revive 之后只记账；调用方若得到 true 应立刻 return。
	 */
	[[nodiscard]] bool TryRecordOnly(const AttackStepRecord& rec)
	{
		if (!recordOnly)
		{
			return false;
		}
		Record(rec);
		return true;
	}

	void FlushStandby()
	{
		if (standby.parent != nullptr)
		{
			standby.parent->SealAssembledRecipe(std::move(recipe));
			if (!standby.parked)
			{
				shots.push_back(standby.parent);
			}
		}
		recipe.clear();
		standby.parent = nullptr;
		standby.children.clear();
		standby.host = nullptr;
		standby.parked = false;
		recordOnly = false;
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
T* AddToFocus(DeployContext& ctx, const AttackStepRecord& rec, Args&&... args)
{
	if (ctx.TryRecordOnly(rec))
	{
		return nullptr;
	}
	if (ctx.standby.host == nullptr)
	{
		if (ctx.standby.parent != nullptr)
		{
			ctx.EnqueueOnEmptyPit(
				[captured = std::make_tuple(std::forward<Args>(args)...), rec](DeployContext& c) mutable
				{
					std::apply([&c, rec](auto&&... a)
					{
						AddToFocus<T>(c, rec, std::forward<decltype(a)>(a)...);
					}, std::move(captured));
				});
		}
		return nullptr;
	}
	T* raw = ctx.standby.host->AddModule<T>(std::forward<Args>(args)...);
	if (raw != nullptr)
	{
		ctx.Record(rec);
	}
	return raw;
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

	constexpr float kDefaultChildRingRadius{ 2.0f };
	float radius = kDefaultChildRingRadius;
	for (Attack* child : s.children)
	{
		if (child == nullptr)
		{
			continue;
		}
		if (Rule_Orbit_Module* orbit = child->GetModule<Rule_Orbit_Module>())
		{
			radius = orbit->GetRadius();
		}
	}

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
		/**
		 * 已有 host = 新主体：先封上一套进 shots（扫描才能立刻开火）。
		 * 必须在 TryRecordOnly 之前，否则 Revive 后的 Spawn 会被只记账、永远不 Flush。
		 */
		if (s.host != nullptr)
		{
			ctx.FlushStandby();
		}
		if (ctx.TryRecordOnly(AttackStepRecordMake::SpawnBall(scale_, enableCollider_)))
		{
			return;
		}

		if (s.gfx == nullptr || s.rg == nullptr)
		{
			return;
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
		ball->SetLifeTime(0.5f);
		ball->ResetLifeTimer();
		ball->ApplyPresentation(scale_, enableCollider_);

		if (s.parent == nullptr || ball == s.parent)
		{
			s.parent = ball;
			s.host = ball;
			ctx.Record(AttackStepRecordMake::SpawnBall(scale_, enableCollider_));
			return;
		}

		ball->SetParent(s.parent);
		s.children.push_back(ball);
		s.host = ball;
		ctx.Record(AttackStepRecordMake::SpawnBall(scale_, enableCollider_));
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
		if (ctx.TryRecordOnly(AttackStepRecordMake::OtherChild()))
		{
			return;
		}
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
		ctx.Record(AttackStepRecordMake::OtherChild());
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
		AddToFocus<Attribute_Lifetime_Module>(
			ctx,
			AttackStepRecordMake::Lifetime(durationSeconds_),
			durationSeconds_);
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
		AddToFocus<Attribute_SpeedRate_Module>(
			ctx,
			AttackStepRecordMake::SpeedRate(speedRate_),
			speedRate_);
	}

	static std::unique_ptr<AttackNodeStep_Attribute_SpeedRate> Make(float speedRate)
	{
		return std::make_unique<AttackNodeStep_Attribute_SpeedRate>(speedRate);
	}

private:
	float speedRate_{ 1.0f };
};

class AttackNodeStep_Attribute_SizeRate final : public IAttackNodeStep
{
public:
	explicit AttackNodeStep_Attribute_SizeRate(float sizeRate) noexcept
		: sizeRate_{ sizeRate }
	{}

	[[nodiscard]] ModuleNodeLabel GetModuleNodeLabel() const noexcept override
	{
		return ModuleNodeLabel::Attribute_SizeRate;
	}
	[[nodiscard]] const char* GetName() const noexcept override { return "Attribute_SizeRate"; }
	[[nodiscard]] bool HasModule() const noexcept override { return true; }
	[[nodiscard]] DeployTarget GetTarget() const noexcept override { return DeployTarget::Focus; }

	void Apply(DeployContext& ctx) override
	{
		AddToFocus<Attribute_SizeRate_Module>(
			ctx,
			AttackStepRecordMake::SizeRate(sizeRate_),
			sizeRate_);
	}

	static std::unique_ptr<AttackNodeStep_Attribute_SizeRate> Make(float sizeRate)
	{
		return std::make_unique<AttackNodeStep_Attribute_SizeRate>(sizeRate);
	}

private:
	float sizeRate_{ 0.0f };
};

class AttackNodeStep_Attribute_DamageRate final : public IAttackNodeStep
{
public:
	explicit AttackNodeStep_Attribute_DamageRate(float damageRate) noexcept
		: damageRate_{ damageRate }
	{}

	[[nodiscard]] ModuleNodeLabel GetModuleNodeLabel() const noexcept override
	{
		return ModuleNodeLabel::Attribute_DamageRate;
	}
	[[nodiscard]] const char* GetName() const noexcept override { return "Attribute_DamageRate"; }
	[[nodiscard]] bool HasModule() const noexcept override { return true; }
	[[nodiscard]] DeployTarget GetTarget() const noexcept override { return DeployTarget::Focus; }

	void Apply(DeployContext& ctx) override
	{
		AddToFocus<Attribute_DamageRate_Module>(
			ctx,
			AttackStepRecordMake::DamageRate(damageRate_),
			damageRate_);
	}

	static std::unique_ptr<AttackNodeStep_Attribute_DamageRate> Make(float damageRate)
	{
		return std::make_unique<AttackNodeStep_Attribute_DamageRate>(damageRate);
	}

private:
	float damageRate_{ 0.0f };
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
		if (ctx.TryRecordOnly(AttackStepRecordMake::Orbit(orbitRadius_, orbitPhase_)))
		{
			return;
		}
		AttackStandby& s = ctx.standby;
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

		const bool aroundParentShot = (s.host != s.parent && s.parent != nullptr);
		ObjectBase* center = aroundParentShot
			? static_cast<ObjectBase*>(s.parent)
			: static_cast<ObjectBase*>(s.player);
		if (center == nullptr)
		{
			return;
		}

		float phase = orbitPhase_;
		if (aroundParentShot)
		{
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
		}
		else if (phase < 0.0f)
		{
			phase = 0.0f;
		}
		s.host->AddModule<Rule_Orbit_Module>(orbitRadius_, phase, center);
		ctx.Record(AttackStepRecordMake::Orbit(orbitRadius_, phase));
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

class AttackNodeStep_Rule_Return final : public IAttackNodeStep
{
public:
	[[nodiscard]] ModuleNodeLabel GetModuleNodeLabel() const noexcept override
	{
		return ModuleNodeLabel::Rule_Return;
	}
	[[nodiscard]] const char* GetName() const noexcept override { return "Rule_Return"; }
	[[nodiscard]] bool HasModule() const noexcept override { return true; }
	[[nodiscard]] DeployTarget GetTarget() const noexcept override { return DeployTarget::ShotRoot; }

	void Apply(DeployContext& ctx) override
	{
		if (ctx.TryRecordOnly(AttackStepRecordMake::Return()))
		{
			return;
		}
		AttackStandby& s = ctx.standby;
		if (s.parent == nullptr)
		{
			return;
		}
		if (s.parent->GetModule<Rule_Return_Module>() != nullptr)
		{
			return;
		}
		s.parent->AddModule<Rule_Return_Module>(s.gfx, s.player);
		ctx.Record(AttackStepRecordMake::Return());
	}

	static std::unique_ptr<AttackNodeStep_Rule_Return> Make()
	{
		return std::make_unique<AttackNodeStep_Rule_Return>();
	}
};

class AttackNodeStep_Other_Revive final : public IAttackNodeStep
{
public:
	[[nodiscard]] ModuleNodeLabel GetModuleNodeLabel() const noexcept override
	{
		return ModuleNodeLabel::Other_Revive;
	}
	[[nodiscard]] const char* GetName() const noexcept override { return "Other_Revive"; }
	[[nodiscard]] bool HasModule() const noexcept override { return true; }
	[[nodiscard]] DeployTarget GetTarget() const noexcept override { return DeployTarget::ShotRoot; }

	void Apply(DeployContext& ctx) override
	{
		if (ctx.TryRecordOnly(AttackStepRecordMake::Revive()))
		{
			return;
		}
		AttackStandby& s = ctx.standby;
		if (s.parent == nullptr)
		{
			return;
		}
		if (s.gfx == nullptr || s.rg == nullptr)
		{
			return;
		}
		if (s.parent->GetModule<Other_Revive_Module>() != nullptr)
		{
			return;
		}

		s.parent->AddModule<Other_Revive_Module>(s.gfx, s.rg, s.player);
		ctx.Record(AttackStepRecordMake::Revive());
		ctx.recordOnly = true;
	}

	static std::unique_ptr<AttackNodeStep_Other_Revive> Make()
	{
		return std::make_unique<AttackNodeStep_Other_Revive>();
	}
};

/**
 * @brief 按快照 Apply 一条 Step。
 * @note Revive 与扫描相同（挂一层模块并 recordOnly）。Passive 仍跳过。
 */
inline void ApplyAttackStepRecord(DeployContext& ctx, const AttackStepRecord& rec)
{
	switch (rec.label)
	{
	case ModuleNodeLabel::Core_Ball:
	case ModuleNodeLabel::Spawn_Ball:
		AttackNodeStep_Spawn_Ball::Make(DirectX::XMFLOAT3{ rec.a, rec.b, rec.c },rec.flag)->Apply(ctx);
		break;
	case ModuleNodeLabel::Other_Child:
		AttackNodeStep_Other_Child::Make()->Apply(ctx);
		break;
	case ModuleNodeLabel::Other_Revive:
		AttackNodeStep_Other_Revive::Make()->Apply(ctx);
		break;
	case ModuleNodeLabel::Attribute_Lifetime:
		AttackNodeStep_Attribute_Lifetime::Make(rec.a)->Apply(ctx);
		break;
	case ModuleNodeLabel::Attribute_SpeedRate:
		AttackNodeStep_Attribute_SpeedRate::Make(rec.a)->Apply(ctx);
		break;
	case ModuleNodeLabel::Attribute_SizeRate:
		AttackNodeStep_Attribute_SizeRate::Make(rec.a)->Apply(ctx);
		break;
	case ModuleNodeLabel::Attribute_DamageRate:
		AttackNodeStep_Attribute_DamageRate::Make(rec.a)->Apply(ctx);
		break;
	case ModuleNodeLabel::Rule_Orbit:
		AttackNodeStep_Rule_Orbit::Make(rec.a, rec.b)->Apply(ctx);
		break;
	case ModuleNodeLabel::Rule_Return:
		AttackNodeStep_Rule_Return::Make()->Apply(ctx);
		break;
	case ModuleNodeLabel::Passive_DamageFix:
	case ModuleNodeLabel::Count:
	default:
		break;
	}
}

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
		ctx.standby.player = player;

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

	/**
	 * @brief 按封存快照装配一棵树。不 SpawnAt，由调用方 AdoptLive 后开火。
	 * @note Revive 世代消耗不在这里；由 Other_Revive_Module::Replay_ 在 Apply 前跳过第一条。
	 */
	static std::vector<Attack*> DeployRecords(const std::vector<AttackStepRecord>& records, Graphics& gfx, Rgph::RenderGraph& rg, DirectX::XMFLOAT3 pos, Player* player)
	{
		DeployContext ctx{};
		ctx.standby.gfx = &gfx;
		ctx.standby.rg = &rg;
		ctx.standby.spawnPos = pos;
		ctx.standby.player = player;

		for (const AttackStepRecord& rec : records)
		{
			ApplyAttackStepRecord(ctx, rec);
		}

		ctx.FlushStandby();
		return ctx.shots;
	}
};
