#include "RingYang.h"

#include "Channels.h"
#include "EnemyBase.h"
#include "ObjectCodex.h"
#include "RenderGraph.h"

#include <cmath>

RingYang::RingYang(Graphics& gfx, Rgph::RenderGraph& rg)
	:
	AttackBase(attack_Ring_Yang)
{
	// 这一步没有伤害。base 是基础值，ResetMods 清掉加算、倍率和固定补正。
	Stats().damage.base = 0.0f;
	Stats().damage.ResetMods();

	ring_ = std::make_unique<Ring_Shape>(gfx);
	ring_->LinkTechniques(rg);
	ring_->SetColor({ 1.0f, 1.0f, 1.0f });
	ring_->SetGradient(true);
	ring_->SetInnerWave(true);
	ring_->SetOuterWave(true);
	ring_->SetAmplitude(kAmplitude_);
}

RingYang::~RingYang() = default;

void RingYang::SpawnAt(XMFLOAT3 pos, XMFLOAT3 dir)
{
	origin_ = pos;
	hitDir_ = dir;
	age_ = 0.0f;
	prevOuter_ = 0.0f;
	outer_ = 0.0f;
	targetRadius_ = kDefaultRadius_;
	hitEnemies_.clear();
	Stats().damage.base = 0.0f;
	Stats().damage.ResetMods();
	SetPosition(pos);
	ResetMoveVelocity();
	SetMoveAccel({ 0.0f, 0.0f, 0.0f });
	if (ring_ != nullptr)
	{
		ring_->SetAmplitude(kAmplitude_);
	}
}

void RingYang::OnEnable(void)
{
	age_ = 0.0f;
	prevOuter_ = 0.0f;
	outer_ = 0.0f;
	hitEnemies_.clear();
}

void RingYang::ApplyOwnHit(CharacterBase* other)
{
	auto* enemy = dynamic_cast<EnemyBase*>(other);
	if (enemy == nullptr || enemy->GetIsDeath())
	{
		return;
	}

	// damage 是这次要扣的血。为 0 时只播放，不进入受伤，不击退。
	const float damage = Stats().damage.Final();
	if (damage == 0.0f)
	{
		return;
	}
	for (EnemyBase* hit : hitEnemies_)
	{
		if (hit == enemy)
		{
			return;
		}
	}
	hitEnemies_.push_back(enemy);

	enemy->SetIsHurt(true);
	enemy->CalculateHpCurrent(-damage);

	const XMFLOAT3 pos = enemy->GetWorldPosition();
	float hx = pos.x - origin_.x;
	float hz = pos.z - origin_.z;
	const float fromCenterSq = hx * hx + hz * hz;
	if (fromCenterSq <= 1.0e-8f)
	{
		// 人就在圆心上。用出生时传入的水平方向。接近 0 时用世界 +Z。
		hx = hitDir_.x;
		hz = hitDir_.z;
		const float dirSq = hx * hx + hz * hz;
		if (dirSq <= 1.0e-8f)
		{
			hx = 0.0f;
			hz = 1.0f;
		}
		else
		{
			const float inv = 1.0f / std::sqrt(dirSq);
			hx *= inv;
			hz *= inv;
		}
	}
	else
	{
		const float inv = 1.0f / std::sqrt(fromCenterSq);
		hx *= inv;
		hz *= inv;
	}
	enemy->CalculateMoveVelocity(hx * kKnockbackHorizontal_, kKnockbackUp_, hz * kKnockbackHorizontal_);
}

void RingYang::SweepWavefront_()
{
	// 伤害为 0 只播放。外沿没往外走（淡出）不再产生新的命中。
	if (Stats().damage.Final() == 0.0f || !(outer_ > prevOuter_))
	{
		return;
	}

	for (Object_Type_Tag tag : kCharacterTags)
	{
		for (EnemyBase* enemy : ObjectCodex::FindActiveObjectsByTag<EnemyBase>(tag))
		{
			if (enemy == nullptr || enemy->GetIsDeath())
			{
				continue;
			}

			const XMFLOAT3 pos = enemy->GetWorldPosition();
			const float dx = pos.x - origin_.x;
			const float dz = pos.z - origin_.z;
			const float distSq = dx * dx + dz * dz;
			// atCenter 是站在圆心上的那个人。距离为 0，普通比较会漏掉他。
			const bool atCenter = distSq <= 1.0e-8f;
			const float dist = atCenter ? 0.0f : std::sqrt(distSq);
			const bool crossed = prevOuter_ < dist && dist <= outer_;
			const bool centerLeave = atCenter && prevOuter_ <= 0.0f && outer_ > prevOuter_;
			if (!crossed && !centerLeave)
			{
				continue;
			}
			ApplyOwnHit(enemy);
		}
	}
}

float RingYang::OuterAt_(float age) const
{
	if (age <= 0.0f)
	{
		return 0.0f;
	}
	if (age < kExpandSeconds_)
	{
		return targetRadius_ * (age / kExpandSeconds_);
	}
	return targetRadius_;
}

bool RingYang::ApplyRing_()
{
	if (ring_ == nullptr || age_ <= 0.0f)
	{
		return false;
	}

	// opacity 是整只环的透明度。扩大时保持 1，淡出时从 1 收到 0。
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

void RingYang::Update(float dt)
{
	prevOuter_ = outer_;
	age_ += dt;
	outer_ = OuterAt_(age_);
	SweepWavefront_();
	if (age_ >= kExpandSeconds_ + kFadeSeconds_)
	{
		RequestDisable();
	}
	ObjectBase::Update(dt);
}

void RingYang::Submit(void)
{
	if (ApplyRing_() && ring_ != nullptr)
	{
		ring_->Submit(Chan::main);
	}
	ObjectBase::Submit();
}
