#pragma once
#include "IAttackNodeStep.h"
#include "Ball.h"
#include "ObjectCodex.h"
#include "XMath.h"

#include "Module_Attribute_LifetimeRate.h"
#include "Module_Attribute_SpeedRate.h"
#include "Module_Attribute_SizeRate.h"
#include "Module_Attribute_DamageRate.h"

#include "Module_Rule_Orbit.h"
#include "Module_Rule_Return.h"
#include "Module_Rule_Revive.h"
#include "Module_Element_Yin.h"
#include "Module_Element_Yang.h"

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
	for (AttackBase_Moduled* child : s.children)
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
		AttackBase_Moduled* child = s.children[i];
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

namespace 
{
	// Core / Spawn 出球相同：按 rec.label 记账，避免两套 Apply 日后改漏。
	inline void ApplyBallSpawnStep(DeployContext& ctx, const AttackStepRecord& rec)
	{
		AttackStandby& s = ctx.standby;
		const DirectX::XMFLOAT3 scale{ rec.a, rec.b, rec.c };
		const bool enableCollider = rec.flag;

		// 重生之后仍在记账。host 还在表示子物体栏位没开，这次出球是新的一发。
		// 先封存上一发。Flush 会清掉 recordOnly，这颗球再按普通出球走。
		if (ctx.recordOnly && s.host != nullptr)
		{
			ctx.FlushStandby();
		}
		// host 为空表示栏位开着。这次生成记进配方，再把焦点放回主体。
		// 栏位因此算占用，下一颗生成才会封存前面这一份。
		if (ctx.recordOnly && s.host == nullptr)
		{
			ctx.Record(rec);
			s.host = s.parent;
			return;
		}
		// host 是当前焦点。还在就说明没有栏位，上一套先封进 shots。
		// 子物体栏位把 host 置空后不封存，下面把这颗球填进坑里。
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
		ball->ResetLifeTimer();
		ball->ApplyPresentation(scale, enableCollider);

		if (s.parent == nullptr || ball == s.parent)
		{
			s.parent = ball;
			s.host = ball;
			// 出球记录在前；没有主体时先入队的 Node 在这里补上。
			ctx.Record(rec);
			ctx.DrainPitQueue();
			return;
		}

		ball->SetParent(s.parent);
		s.children.push_back(ball);
		s.host = ball;
		ctx.Record(rec);
		ctx.DrainPitQueue();
		RedistributeChildrenEvenly(s);
	}
}

// ————————————————————————————————————————————————————
// Kind —— Core
// ————————————————————————————————————————————————————
class AttackNodeStep_Core_Ball final : public IAttackNodeStep
{
public:
	explicit AttackNodeStep_Core_Ball(
		DirectX::XMFLOAT3 scale = { 1.0f, 1.0f, 1.0f },
		bool enableCollider = true) noexcept
		:
		scale_(scale),
		enableCollider_(enableCollider)
	{}

	[[nodiscard]] ModuleNodeLabel GetModuleNodeLabel() const noexcept override
	{
		return ModuleNodeLabel::Core_Ball;
	}
	[[nodiscard]] const char* GetName() const noexcept override { return "Core_Ball"; }

	void Apply(DeployContext& ctx) override
	{
		ApplyBallSpawnStep(ctx, AttackStepRecordMake::CoreBall(scale_, enableCollider_));
	}

	static std::unique_ptr<AttackNodeStep_Core_Ball> Make(
		DirectX::XMFLOAT3 scale = { 1.0f, 1.0f, 1.0f },
		bool enableCollider = true)
	{
		return std::make_unique<AttackNodeStep_Core_Ball>(scale, enableCollider);
	}

private:
	DirectX::XMFLOAT3 scale_{ 1.0f, 1.0f, 1.0f };
	bool enableCollider_{ true };
};

// ————————————————————————————————————————————————————
// Kind —— Spawn
// ————————————————————————————————————————————————————
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
		ApplyBallSpawnStep(ctx, AttackStepRecordMake::SpawnBall(scale_, enableCollider_));
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

// ————————————————————————————————————————————————————
// Kind —— Attribute
// ————————————————————————————————————————————————————
class AttackNodeStep_Attribute_LifetimeRate final : public IAttackNodeStep
{
public:
	explicit AttackNodeStep_Attribute_LifetimeRate(float lifetimeRate) noexcept
		:
		lifetimeRate_(lifetimeRate)
	{}

	[[nodiscard]] ModuleNodeLabel GetModuleNodeLabel() const noexcept override
	{
		return ModuleNodeLabel::Attribute_LifetimeRate;
	}
	[[nodiscard]] const char* GetName() const noexcept override { return "Attribute_LifetimeRate"; }
	[[nodiscard]] bool HasModule() const noexcept override { return true; }
	[[nodiscard]] DeployTarget GetTarget() const noexcept override { return DeployTarget::Focus; }

	void Apply(DeployContext& ctx) override
	{
		AddToFocus<Module_Attribute_LifetimeRate>(
			ctx,
			AttackStepRecordMake::LifetimeRate(lifetimeRate_),
			lifetimeRate_);
	}

	static std::unique_ptr<AttackNodeStep_Attribute_LifetimeRate> Make(float lifetimeRate)
	{
		return std::make_unique<AttackNodeStep_Attribute_LifetimeRate>(lifetimeRate);
	}

private:
	float lifetimeRate_{ 0.5f };
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

// ————————————————————————————————————————————————————
// Kind —— Rule
// ————————————————————————————————————————————————————
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
			// 没有主体，或子坑还没填球：先入队，等生成主体后再装。
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
			ctx.EnqueueOnEmptyPit([](DeployContext& c)
			{
				AttackNodeStep_Rule_Return{}.Apply(c);
			});
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

class AttackNodeStep_Rule_Child final : public IAttackNodeStep
{
public:
	[[nodiscard]] ModuleNodeLabel GetModuleNodeLabel() const noexcept override
	{
		return ModuleNodeLabel::Rule_Child;
	}
	[[nodiscard]] const char* GetName() const noexcept override { return "Rule_Child"; }

	void Apply(DeployContext& ctx) override
	{
		AttackStandby& s = ctx.standby;
		// 记账时也把焦点让出来。后面的生成靠 host 是否为空判断栏位开了没有。
		if (ctx.recordOnly && s.parent != nullptr && s.host != nullptr)
		{
			s.host = nullptr;
		}
		if (ctx.TryRecordOnly(AttackStepRecordMake::Child()))
		{
			return;
		}
		if (s.parent == nullptr)
		{
			ctx.EnqueueOnEmptyPit([](DeployContext& c)
			{
				AttackNodeStep_Rule_Child{}.Apply(c);
			});
			return;
		}
		if (s.host == nullptr)
		{
			return;
		}
		s.host = nullptr;
		ctx.Record(AttackStepRecordMake::Child());
	}

	static std::unique_ptr<AttackNodeStep_Rule_Child> Make()
	{
		return std::make_unique<AttackNodeStep_Rule_Child>();
	}
};

class AttackNodeStep_Rule_Revive final : public IAttackNodeStep
{
public:
	[[nodiscard]] ModuleNodeLabel GetModuleNodeLabel() const noexcept override
	{
		return ModuleNodeLabel::Rule_Revive;
	}
	[[nodiscard]] const char* GetName() const noexcept override { return "Rule_Revive"; }
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
			ctx.EnqueueOnEmptyPit([](DeployContext& c)
			{
				AttackNodeStep_Rule_Revive{}.Apply(c);
			});
			return;
		}
		if (s.gfx == nullptr || s.rg == nullptr)
		{
			return;
		}
		if (s.parent->GetModule<Module_Rule_Revive>() != nullptr)
		{
			return;
		}

		s.parent->AddModule<Module_Rule_Revive>(s.gfx, s.rg, s.player);
		ctx.Record(AttackStepRecordMake::Revive());
		ctx.recordOnly = true;
	}

	static std::unique_ptr<AttackNodeStep_Rule_Revive> Make()
	{
		return std::make_unique<AttackNodeStep_Rule_Revive>();
	}
};

// ————————————————————————————————————————————————————
// Kind —— Element
// ————————————————————————————————————————————————————
// 把阴模块挂到当前这发弹上。重复和复活重放时走这里。
class AttackNodeStep_Element_Yin final : public IAttackNodeStep
{
public:
	// markCount 这次命中加上的标记数。extraDamage 每触发一次扣的隐藏伤害。
	AttackNodeStep_Element_Yin(float markCount, float extraDamage) noexcept
		:
		markCount_(markCount),
		extraDamage_(extraDamage)
	{}

	[[nodiscard]] ModuleNodeLabel GetModuleNodeLabel() const noexcept override
	{
		return ModuleNodeLabel::Element_Yin;
	}
	[[nodiscard]] const char* GetName() const noexcept override { return "Element_Yin"; }
	[[nodiscard]] bool HasModule() const noexcept override { return true; }
	[[nodiscard]] DeployTarget GetTarget() const noexcept override { return DeployTarget::Focus; }

	// 只挂到当前聚焦的这发弹。gfx 和 rg 留给模块，池不够时再造阴环。
	void Apply(DeployContext& ctx) override
	{
		const AttackStepRecord rec = AttackStepRecordMake::ElementYin(markCount_, extraDamage_);
		if (ctx.TryRecordOnly(rec))
		{
			return;
		}
		AttackStandby& s = ctx.standby;
		if (s.host == nullptr || s.gfx == nullptr || s.rg == nullptr)
		{
			const float marks = markCount_;
			const float damage = extraDamage_;
			ctx.EnqueueOnEmptyPit([marks, damage](DeployContext& c)
			{
				AttackNodeStep_Element_Yin(marks, damage).Apply(c);
			});
			return;
		}
		// 主体就绪后把阴模块装上，并记下这条步骤。等待队列不能拷贝 Graphics 和渲染图。
		s.host->AddModule<Module_Element_Yin>(markCount_, extraDamage_, *s.gfx, *s.rg);
		ctx.Record(rec);
	}

	static std::unique_ptr<AttackNodeStep_Element_Yin> Make(float markCount, float extraDamage)
	{
		return std::make_unique<AttackNodeStep_Element_Yin>(markCount, extraDamage);
	}

private:
	// 这次命中要加上的标记个数。
	float markCount_{ 0.0f };
	// 每触发一次扣掉的血量。
	float extraDamage_{ 0.0f };
};

// 把阳模块挂到当前这发弹上。重复和复活重放时走这里。
class AttackNodeStep_Element_Yang final : public IAttackNodeStep
{
public:
	// markCount 这次命中加上的标记数。areaDamage 每触发一次的范围伤害。radius 是水平半径。
	AttackNodeStep_Element_Yang(float markCount, float areaDamage, float radius) noexcept
		:
		markCount_(markCount),
		areaDamage_(areaDamage),
		radius_(radius)
	{}

	[[nodiscard]] ModuleNodeLabel GetModuleNodeLabel() const noexcept override
	{
		return ModuleNodeLabel::Element_Yang;
	}
	[[nodiscard]] const char* GetName() const noexcept override { return "Element_Yang"; }
	[[nodiscard]] bool HasModule() const noexcept override { return true; }
	[[nodiscard]] DeployTarget GetTarget() const noexcept override { return DeployTarget::Focus; }

	// 只挂到当前聚焦的这发弹。gfx 和 rg 留给模块，池不够时再造阳环。
	void Apply(DeployContext& ctx) override
	{
		const AttackStepRecord rec = AttackStepRecordMake::ElementYang(markCount_, areaDamage_, radius_);
		if (ctx.TryRecordOnly(rec))
		{
			return;
		}
		AttackStandby& s = ctx.standby;
		if (s.host == nullptr || s.gfx == nullptr || s.rg == nullptr)
		{
			const float marks = markCount_;
			const float damage = areaDamage_;
			const float radius = radius_;
			ctx.EnqueueOnEmptyPit([marks, damage, radius](DeployContext& c)
			{
				AttackNodeStep_Element_Yang(marks, damage, radius).Apply(c);
			});
			return;
		}
		// 主体就绪后把阳模块装上，并记下这条步骤。等待队列不能拷贝 Graphics 和渲染图。
		s.host->AddModule<Module_Element_Yang>(markCount_, areaDamage_, radius_, *s.gfx, *s.rg);
		ctx.Record(rec);
	}

	static std::unique_ptr<AttackNodeStep_Element_Yang> Make(float markCount, float areaDamage, float radius)
	{
		return std::make_unique<AttackNodeStep_Element_Yang>(markCount, areaDamage, radius);
	}

private:
	// 这次命中要加上的标记个数。
	float markCount_{ 0.0f };
	// 范围内每个还活着的敌人要扣的血量。
	float areaDamage_{ 0.0f };
	// 水平作用半径。
	float radius_{ 0.0f };
};

// 按快照 Apply 一条 Step。
// Revive 与扫描相同（挂一层模块并 recordOnly）。Passive 仍跳过。Other_Repeat 由 Node 展开，配方不重放。
inline void ApplyAttackStepRecord(DeployContext& ctx, const AttackStepRecord& rec)
{
	switch (rec.label)
	{
	// ————————————————————————————————————————————————————
	// Kind —— Core
	// ————————————————————————————————————————————————————
	case ModuleNodeLabel::Core_Ball:	AttackNodeStep_Core_Ball::Make(DirectX::XMFLOAT3{ rec.a, rec.b, rec.c }, rec.flag)->Apply(ctx);		break;
	// ————————————————————————————————————————————————————
	// Kind —— Spawn
	// ————————————————————————————————————————————————————
	case ModuleNodeLabel::Spawn_Ball:	AttackNodeStep_Spawn_Ball::Make(DirectX::XMFLOAT3{ rec.a, rec.b, rec.c }, rec.flag)->Apply(ctx);	break;
	// ————————————————————————————————————————————————————
	// Kind —— Attribute
	// ————————————————————————————————————————————————————
	case ModuleNodeLabel::Attribute_LifetimeRate:	AttackNodeStep_Attribute_LifetimeRate::Make(rec.a)->Apply(ctx);	break;
	case ModuleNodeLabel::Attribute_SpeedRate:		AttackNodeStep_Attribute_SpeedRate::Make(rec.a)->Apply(ctx);	break;
	case ModuleNodeLabel::Attribute_SizeRate:		AttackNodeStep_Attribute_SizeRate::Make(rec.a)->Apply(ctx);		break;
	case ModuleNodeLabel::Attribute_DamageRate:		AttackNodeStep_Attribute_DamageRate::Make(rec.a)->Apply(ctx);	break;
	// ————————————————————————————————————————————————————
	// Kind —— Rule
	// ————————————————————————————————————————————————————
	case ModuleNodeLabel::Rule_Orbit:	AttackNodeStep_Rule_Orbit::Make(rec.a, rec.b)->Apply(ctx);	break;
	case ModuleNodeLabel::Rule_Return:	AttackNodeStep_Rule_Return::Make()->Apply(ctx);				break;
	case ModuleNodeLabel::Rule_Child:	AttackNodeStep_Rule_Child::Make()->Apply(ctx);				break;
	case ModuleNodeLabel::Rule_Revive:	AttackNodeStep_Rule_Revive::Make()->Apply(ctx);				break;
	// ————————————————————————————————————————————————————
	// Kind —— Element
	// ————————————————————————————————————————————————————
	case ModuleNodeLabel::Element_Yin:	AttackNodeStep_Element_Yin::Make(rec.a, rec.b)->Apply(ctx);				break;
	case ModuleNodeLabel::Element_Yang:	AttackNodeStep_Element_Yang::Make(rec.a, rec.b, rec.c)->Apply(ctx);		break;
	// ————————————————————————————————————————————————————
	// Kind —— Passive
	// ————————————————————————————————————————————————————
	case ModuleNodeLabel::Passive_DamageFix:	break;
	// ————————————————————————————————————————————————————
	// Kind —— Other
	// ————————————————————————————————————————————————————
	case ModuleNodeLabel::Other_Repeat:	break;
	// ————————————————————————————————————————————————————
	// Kind —— Fusion
	// ————————————————————————————————————————————————————
	case ModuleNodeLabel::Fusion:	break;
	// ————————————————————————————————————————————————————
	// Kind —— Ultra
	// ————————————————————————————————————————————————————
	case ModuleNodeLabel::Ultra:	break;
	// ————————————————————————————————————————————————————
	case ModuleNodeLabel::Count:	break;
	default:						break;
	}
}
