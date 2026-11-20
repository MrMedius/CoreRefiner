#pragma once
#include "Effect.h"
#include "Effect_Ring_Shape.h"

#include <memory>

class Graphics;
namespace Rgph
{
	class RenderGraph;
}

// 阳的白环。渐淡和两道波浪都开，先扩大再淡出。
class Effect_Yang : public Effect
{
public:
	Effect_Yang(Graphics& gfx, Rgph::RenderGraph& rg);
	~Effect_Yang() override;

	// pos 是播放中心。flip 这只环不用。
	void SpawnAt(XMFLOAT3 pos, bool flip) override;
	void OnEnable(void) override;
	void Update(float dt) override;
	void Submit(void) override;
	void OnCollide(Character* other) override;

private:
	// 按当前进度写下半径和透明度。播放中返回 true。
	bool ApplyRing_();

	// 这一只白环。
	std::unique_ptr<Effect_Ring_Shape> ring_;
	// 这一次播放的中心。
	XMFLOAT3 origin_{ 0.0f, 0.0f, 0.0f };
	// 从 SpawnAt 起算的秒数。
	float age_{ 0.0f };

	// 从 0 扩到目标半径用的秒数。
	static constexpr float kExpandSeconds_{ 0.3f };
	// 扩完之后淡出用的秒数。计划没写死，先用这段。
	static constexpr float kFadeSeconds_{ 0.25f };
	// 外沿扩到的世界半径。模型外径是 1。
	static constexpr float kRadius_{ 3.0f };
	// 波浪幅度。看着效果再改这个数。
	static constexpr float kAmplitude_{ 0.35f };
};
