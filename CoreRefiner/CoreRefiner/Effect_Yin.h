#pragma once
#include "Effect.h"
#include "Effect_Ring_Shape.h"

#include <array>
#include <memory>

class Graphics;
namespace Rgph
{
	class RenderGraph;
}

// 阴的三重环。黑色，只做外浓内淡，错开出发。
class Effect_Yin : public Effect
{
public:
	Effect_Yin(Graphics& gfx, Rgph::RenderGraph& rg);
	~Effect_Yin() override;

	// pos 是播放中心。flip 这只环不用。
	void SpawnAt(XMFLOAT3 pos, bool flip) override;
	void OnEnable(void) override;
	void Update(float dt) override;
	void Submit(void) override;
	void OnCollide(Character* other) override;

private:
	// 按这一圈已经播放的时间写下缩放和透明度。还没开始或已经结束就返回 false。
	bool ApplyRing_(std::size_t index);

	// 三只环。下标越大，出发越晚。
	std::array<std::unique_ptr<Effect_Ring_Shape>, 3> rings_{};
	// 这一次播放的中心。
	XMFLOAT3 origin_{ 0.0f, 0.0f, 0.0f };
	// 从 SpawnAt 起算的秒数。
	float age_{ 0.0f };

	// 后一圈比前一圈晚出发的秒数。
	static constexpr float kStagger_{ 0.08f };
	// 一只环从 0 扩到目标半径用的秒数。计划没有指定，先用这段方便看清错开。
	static constexpr float kExpandSeconds_{ 0.5f };
	// 外沿扩到的世界半径。模型外径是 1，所以缩放就是这个半径。
	static constexpr float kRadius_{ 5.0f };
	// 绕 forward 掀起的最大角度。每次生成在正负这个值之间抽一个。
	static constexpr float kTiltDegrees_{ 60.0f };
};
