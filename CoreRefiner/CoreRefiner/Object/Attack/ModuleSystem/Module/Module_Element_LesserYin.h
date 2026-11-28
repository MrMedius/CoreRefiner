#pragma once
#include "Ball.h"
#include "IModule.h"
#include "AttackBase.h"
#include "EnemyBase.h"
#include "Graphics.h"
#include "RenderGraph.h"

// 命中时只给敌人加阴标记，这一步不消耗，也不放特效。
// 出生时给宿主追加一条黑色条纹，外层按视线变淡，并打开里面的缩小球。
class Module_Element_LesserYin : public IModule
{
public:
	// markCount 是每次给予的阴标记个数。damageRate 是吸收后个数乘上的倍率。radius 是吸收的水平半径。
	// 倍率和半径这一步先存着。gfx 和 rg 留给以后池不够时再造吸收特效。
	Module_Element_LesserYin(AttackBase* owner, float markCount, float damageRate, float radius, Graphics& gfx, Rgph::RenderGraph& rg) noexcept
		:
		IModule(owner),
		markCount_(static_cast<int>(markCount)),
		damageRate_(damageRate),
		radius_(radius),
		gfx_(gfx),
		rg_(rg)
	{}

	// 给宿主球追加一条黑色条纹，打开外层的视线变淡，并画出里面的缩小球。别的球不打开内层。
	void OnSpawn() override
	{
		if (Ball* ball = dynamic_cast<Ball*>(GetOwner()))
		{
			ball->AppendColorId(kColorId_);
			ball->SetOuterOpacity(kOuterOpacity_);
			ball->SetInnerVisible(true);
		}
	}

	// 只给被打中的敌人加阴标记。不消耗，不扣血。
	void OnHit(CharacterBase* other) override
	{
		EnemyBase* enemy = dynamic_cast<EnemyBase*>(other);
		if (enemy == nullptr || markCount_ <= 0)
		{
			return;
		}
		enemy->Marks().Add(AttackMark::Yin, markCount_);
	}

private:
	// 每次给予的阴标记个数。
	int markCount_{ 0 };
	// 吸收到的阴标记个数乘上的倍率。这一步还不用。
	float damageRate_{ 0.0f };
	// 吸收周围阴标记的水平半径。这一步还不用。
	float radius_{ 0.0f };
	// 对象池扩容时传给以后的吸收特效。这一步还不取对象。
	Graphics& gfx_;
	// 对象池扩容时传给以后的吸收特效。这一步还不取对象。
	Rgph::RenderGraph& rg_;

	// 少阴的条纹颜色编号，黑色。
	static constexpr std::uint32_t kColorId_{ 2u };
	// 小于 1 时宿主外层按视线变淡。具体数值不决定轮廓有多实。别的球保持 1。
	static constexpr float kOuterOpacity_{ 0.8f };
};
