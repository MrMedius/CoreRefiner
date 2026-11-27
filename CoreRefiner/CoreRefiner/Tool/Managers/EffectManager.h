#pragma once
#include "Graphics.h"

#include <vector>

class EffectBase;
namespace Rgph
{
	class RenderGraph;
}

// 已开始播放的特效。池里预热、还没交出的不在这里。白环是攻击，不在这里。
class EffectManager
{
public:
	EffectManager(Graphics& gfx, Rgph::RenderGraph& rg);
	~EffectManager() = default;

	void Update(float dt);
	void Submit();
	// 离开战斗时把还在播放的特效收回池里。指针留着，下次再复用。
	void Reset();

private:
	// 取走所有待认领的 EffectBase，放进 effects。已经停用或已经在名单里的不放。
	void ClaimEffect();

	Graphics& gfx_;
	Rgph::RenderGraph& rg_;
	// 正在播放的特效。播完后从这里拿掉。
	std::vector<EffectBase*> effects;
	// 阴环预热的个数。
	static constexpr int kPoolCount_{ 8 };
};
