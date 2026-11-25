#include "RingYin.h"

#include "Channels.h"
#include "Ring_Shape.h"
#include "RenderGraph.h"

#include <random>

RingYin::RingYin(Graphics& gfx, Rgph::RenderGraph& rg)
	:
	EffectBase(effect_Ring_Yin)
{
	for (auto& ring : rings_)
	{
		ring = std::make_unique<Ring_Shape>(gfx);
		ring->LinkTechniques(rg);
		ring->SetColor({ 0.0f, 0.0f, 0.0f });
		ring->SetGradient(true);
		ring->SetInnerWave(false);
		ring->SetOuterWave(false);
		ring->SetAmplitude(0.0f);
	}
}

RingYin::~RingYin() = default;

void RingYin::SpawnAt(XMFLOAT3 pos, bool flip)
{
	// 环不看翻转。朝向只绕 forward 掀起。
	(void)flip;
	origin_ = pos;
	age_ = 0.0f;

	// tilt 是这次绕模型 +Z 掀起的角度，范围 -60 到 60 度。三只环共用。
	static std::mt19937 rng{ std::random_device{}() };
	std::uniform_real_distribution<float> tilt(-kTiltDegrees_, kTiltDegrees_);
	for (auto& ring : rings_)
	{
		if (ring != nullptr)
		{
			const float roll = XMConvertToRadians(tilt(rng));
			// x 是 pitch，y 是 yaw，z 是绕 forward 的 roll。
			ring->SetRotationRad(0.0f, 0.0f, roll);
		}
	}
}

void RingYin::OnEnable(void)
{
	age_ = 0.0f;
}

void RingYin::OnCollide(CharacterBase* other)
{
	// 环不挡碰撞。
	(void)other;
}

bool RingYin::ApplyRing_(std::size_t index)
{
	// local 是这一圈自己的播放秒数。还没出发时是负数。
	const float local = age_ - kStagger_ * static_cast<float>(index);
	auto& ring = rings_[index];
	if (ring == nullptr || local <= 0.0f || local >= kExpandSeconds_)
	{
		return false;
	}

	// progress 是 0 到 1 的放大进度。
	const float progress = local / kExpandSeconds_;
	const float radius = kRadius_ * progress;
	ring->SetPosition(origin_);
	ring->SetScale({ radius, radius, radius });
	ring->SetOpacity(1.0f - progress);
	return true;
}

void RingYin::Update(float dt)
{
	age_ += dt;
	for (std::size_t i = 0; i < rings_.size(); ++i)
	{
		(void)ApplyRing_(i);
	}

	// 最后一圈也扩完，就收回对象池。
	const float doneAt = kStagger_ * static_cast<float>(rings_.size() - 1u) + kExpandSeconds_;
	if (age_ >= doneAt)
	{
		RequestDisable();
	}
	ObjectBase::Update(dt);
}

void RingYin::Submit(void)
{
	for (std::size_t i = 0; i < rings_.size(); ++i)
	{
		if (ApplyRing_(i) && rings_[i] != nullptr)
		{
			rings_[i]->Submit(Chan::main);
		}
	}
	ObjectBase::Submit();
}
