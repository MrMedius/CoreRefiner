#pragma once
#include "Graphics.h"

namespace Rgph
{
	class RenderGraph;
}

// 阴环的对象池。只更新和提交正在播放的。阳环是攻击，不在这里。
class EffectManager
{
public:
	EffectManager(Graphics& gfx, Rgph::RenderGraph& rg);
	~EffectManager() = default;

	void Update(float dt);
	void Submit();
	// 离开战斗时把阴环收回池里。指针留着，下次再复用。
	void Reset();

private:
	Graphics& gfx_;
	Rgph::RenderGraph& rg_;
	// 阴环预热的个数。
	static constexpr int kPoolCount_{ 8 };
};
