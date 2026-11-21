#include "Ring_Yang.h"

#include "Channels.h"
#include "RenderGraph.h"

Ring_Yang::Ring_Yang(Graphics& gfx, Rgph::RenderGraph& rg)
	:
	Attack(attack_Ring_Yang)
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

Ring_Yang::~Ring_Yang() = default;

void Ring_Yang::SpawnAt(XMFLOAT3 pos, XMFLOAT3 dir)
{
	origin_ = pos;
	hitDir_ = dir;
	age_ = 0.0f;
	prevOuter_ = 0.0f;
	outer_ = 0.0f;
	targetRadius_ = kDefaultRadius_;
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

void Ring_Yang::OnEnable(void)
{
	age_ = 0.0f;
	prevOuter_ = 0.0f;
	outer_ = 0.0f;
}

void Ring_Yang::ApplyOwnHit(Character* other)
{
	// 还没有波前命中。
	(void)other;
}

float Ring_Yang::OuterAt_(float age) const
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

bool Ring_Yang::ApplyRing_()
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

void Ring_Yang::Update(float dt)
{
	prevOuter_ = outer_;
	age_ += dt;
	outer_ = OuterAt_(age_);
	if (age_ >= kExpandSeconds_ + kFadeSeconds_)
	{
		RequestDisable();
	}
	ObjectBase::Update(dt);
}

void Ring_Yang::Submit(void)
{
	if (ApplyRing_() && ring_ != nullptr)
	{
		ring_->Submit(Chan::main);
	}
	ObjectBase::Submit();
}
