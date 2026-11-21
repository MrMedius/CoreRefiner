#pragma once
#include "AttackBase.h"
#include "Ring_Shape.h"

#include <memory>

class Graphics;
namespace Rgph
{
	class RenderGraph;
}

// 阳的攻击环。自己画一只扩散的白环，不挂模块。
class Ring_Yang : public AttackBase
{
public:
	Ring_Yang(Graphics& gfx, Rgph::RenderGraph& rg);
	~Ring_Yang() override;

	// pos 是圆心。dir 留给圆心上那个敌人的击退，这一步只存下来。
	void SpawnAt(XMFLOAT3 pos, XMFLOAT3 dir) override;
	void OnEnable(void) override;
	void Update(float dt) override;
	void Submit(void) override;

protected:
	// 这一步不扣血、不击退。
	void ApplyOwnHit(CharacterBase* other) override;

private:
	// 按 age_ 和 outer_ 写下环的位置、缩放和透明度。还在播放返回 true。
	bool ApplyRing_();
	// age 对应的外沿世界半径。淡出阶段停在目标半径。
	float OuterAt_(float age) const;

	// 这一只白环。
	std::unique_ptr<Ring_Shape> ring_;
	// 这一次播放的圆心。
	XMFLOAT3 origin_{ 0.0f, 0.0f, 0.0f };
	// SpawnAt 传入的方向。圆心上那个敌人以后用它击退。
	XMFLOAT3 hitDir_{ 0.0f, 0.0f, 0.0f };
	// 从 SpawnAt 起算的秒数。
	float age_{ 0.0f };
	// 上一帧外沿的世界半径。
	float prevOuter_{ 0.0f };
	// 这一帧外沿的世界半径。
	float outer_{ 0.0f };
	// 这次要扩到的世界半径。
	float targetRadius_{ 3.0f };

	// 从 0 扩到目标半径用的秒数。
	static constexpr float kExpandSeconds_{ 0.3f };
	// 扩完之后淡出用的秒数。淡出时外沿不动。
	static constexpr float kFadeSeconds_{ 0.25f };
	// 出生时写回的目标半径。模型外径是 1。
	static constexpr float kDefaultRadius_{ 3.0f };
	// 波浪幅度。
	static constexpr float kAmplitude_{ 0.35f };
};
