#pragma once
#include "EffectBase.h"
#include "EnemyBase.h"
#include "Ring_Shape.h"
#include "Channels.h"
#include "RenderGraph.h"

#include <memory>

// 少阴的特效 1。外圈黑、内圈白，带波浪。从脚下扩开再淡出。
// 这一步只记住受击目标并显示。不检测敌人，不生成火焰，不扣血。
class RingLesserYin : public EffectBase
{
public:
	// gfx 和 rg 用来造环。以后生成火焰时也从这里拿。这一步还不取别的对象。
	RingLesserYin(Graphics& gfx, Rgph::RenderGraph& rg)
		:
		EffectBase(effect_Ring_LesserYin),
		gfx_(gfx),
		rg_(rg)
	{
		ring_ = std::make_unique<Ring_Shape>(gfx);
		ring_->LinkTechniques(rg);
		// 内圈白，外圈黑。两边都有颜色。播放淡出写在整体透明度上。
		ring_->SetInnerColor({ 1.0f, 1.0f, 1.0f, 1.0f });
		ring_->SetOuterColor({ 0.0f, 0.0f, 0.0f, 1.0f });
		ring_->SetInnerWave(true);
		ring_->SetOuterWave(true);
		ring_->SetAmplitude(kAmplitude_);
	}

	// 不从这里开始播。调用方用 Play。
	void SpawnAt(XMFLOAT3 pos, bool flip) override
	{
		(void)pos;
		(void)flip;
	}

	// target 是这一轮要记住的受击敌人。radius 是要扩到的水平半径。damageRate 是结算时的倍率，这一步先存着。
	// 圆心取播放当时目标的脚下，之后不跟着他走。
	void Play(EnemyBase* target, float radius, float damageRate)
	{
		target_ = target;
		damageRate_ = damageRate;
		targetRadius_ = radius > 0.0f ? radius : 0.0f;
		age_ = 0.0f;
		outer_ = 0.0f;
		origin_ = { 0.0f, 0.0f, 0.0f };
		playing_ = false;
		if (target_ == nullptr || targetRadius_ <= 0.0f)
		{
			RequestDisable();
			return;
		}
		origin_ = target_->GetWorldPosition();
		SetPosition(origin_);
		playing_ = true;
	}

	// 出池时停掉上一轮，并忘掉旧目标。避免刚拿出来还没 Play 就按旧数据扩。
	void OnEnable(void) override
	{
		playing_ = false;
		age_ = 0.0f;
		outer_ = 0.0f;
		target_ = nullptr;
		damageRate_ = 0.0f;
		targetRadius_ = 0.0f;
		origin_ = { 0.0f, 0.0f, 0.0f };
	}

	// 不走角色碰撞。以后的检测写在自己的更新里。
	void OnCollide(CharacterBase* other) override
	{
		(void)other;
	}

	// 播放中推进半径。目标关掉或死亡就清空指针，环仍然扩到结束。
	void Update(float dt) override
	{
		if (playing_)
		{
			age_ += dt;
			outer_ = OuterAt_(age_);
			WatchTarget_();
			if (age_ >= kExpandSeconds_ + kFadeSeconds_)
			{
				playing_ = false;
				RequestDisable();
			}
		}
		ObjectBase::Update(dt);
	}

	// 还在播放就把环交出去画。
	void Submit(void) override
	{
		if (playing_ && ApplyRing_() && ring_ != nullptr)
		{
			ring_->Submit(Chan::main);
		}
		ObjectBase::Submit();
	}

private:
	// 目标已经不在场上，就把指针清空。环的播放不因此停下。
	void WatchTarget_()
	{
		if (target_ == nullptr)
		{
			return;
		}
		if (!target_->IsActive() || target_->GetIsDeath())
		{
			target_ = nullptr;
		}
	}

	// age 对应的外沿世界半径。淡出阶段停在目标半径。
	float OuterAt_(float age) const
	{
		if (age <= 0.0f || targetRadius_ <= 0.0f)
		{
			return 0.0f;
		}
		if (age < kExpandSeconds_)
		{
			return targetRadius_ * (age / kExpandSeconds_);
		}
		return targetRadius_;
	}

	// 按年龄写下环的位置、缩放和播放透明度。还在播放返回 true。
	bool ApplyRing_()
	{
		if (ring_ == nullptr || age_ <= 0.0f)
		{
			return false;
		}
		float opacity = 0.0f;
		if (age_ < kExpandSeconds_)
		{
			opacity = 1.0f;
		}
		else if (age_ < kExpandSeconds_ + kFadeSeconds_)
		{
			const float fade = (age_ - kExpandSeconds_) / kFadeSeconds_;
			opacity = 1.0f - fade;
		}
		else
		{
			return false;
		}
		ring_->SetPosition(origin_);
		ring_->SetScale({ outer_, outer_, outer_ });
		ring_->SetOpacity(opacity);
		return true;
	}

	// 这一只环。内圈白、外圈黑写在构造里。
	std::unique_ptr<Ring_Shape> ring_;
	// 正在播这一轮。出池或播完都会清掉。
	bool playing_{ false };
	// 从 Play 起算的秒数。
	float age_{ 0.0f };
	// 这一帧外沿的世界半径。
	float outer_{ 0.0f };
	// 这次要扩到的世界半径。
	float targetRadius_{ 0.0f };
	// 播放当时目标脚下的圆心。之后不跟着目标移动。
	XMFLOAT3 origin_{ 0.0f, 0.0f, 0.0f };
	// 这一轮记住的受击敌人。关掉或死亡后变成空。
	EnemyBase* target_{ nullptr };
	// 结算时个数乘上的倍率。这一步还不扣血。
	float damageRate_{ 0.0f };
	// 以后池不够时用来再造火焰。这一步还不取对象。
	Graphics& gfx_;
	Rgph::RenderGraph& rg_;

	// 从 0 扩到目标半径用的秒数。
	static constexpr float kExpandSeconds_{ 0.35f };
	// 扩完之后淡出用的秒数。淡出时外沿不动。
	static constexpr float kFadeSeconds_{ 0.25f };
	// 波浪起伏的幅度。和阳环相同。
	static constexpr float kAmplitude_{ 0.5f };
};
