#pragma once

#include "IAttackNodeStep.h"
#include "Ball.h"
#include "ObjectCodex.h"
#include "XMath.h"

#include "Module_Rule_Orbit.h"
#include "Module_Rule_Return.h"
#include "Module_Attribute_Lifetime.h"
#include "Module_Attribute_SpeedRate.h"
#include "Module_Attribute_SizeRate.h"
#include "Module_Attribute_DamageRate.h"
#include "Module_Other_Revive.h"

#include <cmath>
#include <memory>
#include <vector>

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
		if (Module_Rule_Orbit* orbit = child->GetModule<Module_Rule_Orbit>())
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

		if (Module_Rule_Orbit* orbit = child->GetModule<Module_Rule_Orbit>())
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
		 * Revive 之后只记账：不得先 Flush。否则第二次 Spawn 会把当前树推进 shots，
		 * Flush 再清掉 recordOnly，后面的 Child 永远无法在重放时开槽。
		 */
		if (ctx.TryRecordOnly(AttackStepRecordMake::SpawnBall(scale_, enableCollider_)))
		{
			return;
		}
		/**
		 * 非 recordOnly 且已有 host：这是新的根弹，先把上一套封进 shots。
		 * Child 已把 host 置空时走下面的填坑，不 Flush。
		 */
		if (s.host != nullptr)
		{
			ctx.FlushStandby();
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
		AddToFocus<Module_Attribute_Lifetime>(
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
		AddToFocus<Module_Attribute_SpeedRate>(
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
		AddToFocus<Module_Attribute_SizeRate>(
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
		AddToFocus<Module_Attribute_DamageRate>(
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
		s.host->AddModule<Module_Rule_Orbit>(orbitRadius_, phase, center);
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
		if (s.parent->GetModule<Module_Rule_Return>() != nullptr)
		{
			return;
		}
		s.parent->AddModule<Module_Rule_Return>(s.gfx, s.player);
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
		if (s.parent->GetModule<Module_Other_Revive>() != nullptr)
		{
			return;
		}

		s.parent->AddModule<Module_Other_Revive>(s.gfx, s.rg, s.player);
		ctx.Record(AttackStepRecordMake::Revive());
		ctx.recordOnly = true;
	}

	static std::unique_ptr<AttackNodeStep_Other_Revive> Make()
	{
		return std::make_unique<AttackNodeStep_Other_Revive>();
	}
};

inline void ApplyAttackStepRecord(DeployContext& ctx, const AttackStepRecord& rec);

/**
 * @brief 把 recipe 上一条再 Apply/Record 一次。自身不进配方。
 * @note 空配方、或末尾连续同一 label 已达上限则空操作。复制 Revive = 再买一世。
 */
class AttackNodeStep_Other_Repeat final : public IAttackNodeStep
{
public:
	[[nodiscard]] ModuleNodeLabel GetModuleNodeLabel() const noexcept override
	{
		return ModuleNodeLabel::Other_Repeat;
	}
	[[nodiscard]] const char* GetName() const noexcept override { return "Other_Repeat"; }

	void Apply(DeployContext& ctx) override
	{
		if (ctx.recipe.empty())
		{
			return;
		}
		if (CountTrailingSameLabel_(ctx.recipe) >= kMaxTrailingCopies_)
		{
			return;
		}
		const AttackStepRecord last = ctx.recipe.back();
		ApplyAttackStepRecord(ctx, last);
	}

	static std::unique_ptr<AttackNodeStep_Other_Repeat> Make()
	{
		return std::make_unique<AttackNodeStep_Other_Repeat>();
	}

private:
	static constexpr int kMaxTrailingCopies_ = 8;

	/**
	 * @brief 配方末尾与最后一条相同 label 的连续条数。
	 */
	[[nodiscard]] static int CountTrailingSameLabel_(const std::vector<AttackStepRecord>& recipe) noexcept
	{
		const ModuleNodeLabel label = recipe.back().label;
		int n = 0;
		for (auto it = recipe.rbegin(); it != recipe.rend(); ++it)
		{
			if (it->label != label)
			{
				break;
			}
			++n;
		}
		return n;
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
	case ModuleNodeLabel::Other_Repeat:
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
