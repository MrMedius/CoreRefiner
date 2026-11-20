#include "Effect_Yang.h"

#include "Channels.h"
#include "Effect_Ring_Shape.h"
#include "RenderGraph.h"

Effect_Yang::Effect_Yang(Graphics& gfx, Rgph::RenderGraph& rg)
	:
	Effect(effect_Yang)
{
	ring_ = std::make_unique<Effect_Ring_Shape>(gfx);
	ring_->LinkTechniques(rg);
	ring_->SetColor({ 1.0f, 1.0f, 1.0f });
	ring_->SetGradient(true);
	ring_->SetInnerWave(true);
	ring_->SetOuterWave(true);
}

Effect_Yang::~Effect_Yang() = default;

void Effect_Yang::SpawnAt(XMFLOAT3 pos, bool flip)
{
	// 环不看翻转。
	(void)flip;
	origin_ = pos;
	age_ = 0.0f;
	if (ring_ != nullptr)
	{
		ring_->SetAmplitude(kAmplitude_);
	}
}

void Effect_Yang::OnEnable(void)
{
	age_ = 0.0f;
}

void Effect_Yang::OnCollide(Character* other)
{
	// 环不挡碰撞。
	(void)other;
}

bool Effect_Yang::ApplyRing_()
{
	if (ring_ == nullptr || age_ <= 0.0f)
	{
		return false;
	}

	// radius 是这一帧的世界半径。opacity 是整只环的透明度。
	float radius = 0.0f;
	float opacity = 0.0f;
	if (age_ < kExpandSeconds_)
	{
		const float progress = age_ / kExpandSeconds_;
		radius = kRadius_ * progress;
		opacity = 1.0f;
	}
	else if (age_ < kExpandSeconds_ + kFadeSeconds_)
	{
		const float fade = (age_ - kExpandSeconds_) / kFadeSeconds_;
		radius = kRadius_;
		opacity = 1.0f - fade;
	}
	else
	{
		return false;
	}

	ring_->SetPosition(origin_);
	ring_->SetScale({ radius, radius, radius });
	ring_->SetOpacity(opacity);
	return true;
}

void Effect_Yang::Update(float dt)
{
	age_ += dt;
	(void)ApplyRing_();
	if (age_ >= kExpandSeconds_ + kFadeSeconds_)
	{
		RequestDisable();
	}
	ObjectBase::Update(dt);
}

void Effect_Yang::Submit(void)
{
	if (ApplyRing_() && ring_ != nullptr)
	{
		ring_->Submit(Chan::main);
	}
	ObjectBase::Submit();
}
