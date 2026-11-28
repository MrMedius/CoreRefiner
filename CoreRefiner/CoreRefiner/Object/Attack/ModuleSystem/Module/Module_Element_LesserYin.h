#pragma once
#include "Ball.h"
#include "IModule.h"
#include "AttackBase.h"
#include "EnemyBase.h"
#include "Graphics.h"
#include "ObjectCodex.h"
#include "RenderGraph.h"
#include "RingLesserYin.h"

// 命中时给敌人加阴标记。加上之后不少于 2 个，就在他脚下放出特效 1。
// 不扫描周围，也不清标记。出生时给宿主追加一条黑色条纹，外层按视线变淡，并打开里面的缩小球。
class Module_Element_LesserYin : public IModule
{
public:
	// markCount 是每次给予的阴标记个数。damageRate 是特效 1 先存着的倍率。radius 是环要扩到的水平半径。
	// gfx 和 rg 在池不够时用来再造特效 1。
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

	// 给被打中的敌人加阴标记。加上之后不少于 2 个，就放出特效 1。不清周围的标记，不扣血。
	void OnHit(CharacterBase* other) override
	{
		EnemyBase* enemy = dynamic_cast<EnemyBase*>(other);
		if (enemy == nullptr || markCount_ <= 0)
		{
			return;
		}
		enemy->Marks().Add(AttackMark::Yin, markCount_);
		if (enemy->Marks().Get(AttackMark::Yin) >= kMarkTrigger_)
		{
			SpawnRing_(*enemy);
		}
	}

private:
	// 在这只敌人脚下取出特效 1。目标、半径和倍率交给环，再交给特效管理器。池里没有就再造。
	void SpawnRing_(EnemyBase& enemy) const
	{
		RingLesserYin* ring = ObjectCodex::SpawnPooled<RingLesserYin>(effect_Ring_LesserYin, gfx_, rg_);
		if (ring == nullptr)
		{
			return;
		}
		ring->Play(&enemy, radius_, damageRate_);
		ObjectCodex::Handoff(effect_Ring_LesserYin, ring);
	}

	// 每次给予的阴标记个数。
	int markCount_{ 0 };
	// 交给特效 1 存着的倍率。这一步还不扣血。
	float damageRate_{ 0.0f };
	// 特效 1 要扩到的水平半径。
	float radius_{ 0.0f };
	// 对象池扩容时传给新的特效 1。
	Graphics& gfx_;
	// 对象池扩容时传给新的特效 1。
	Rgph::RenderGraph& rg_;

	// 加上这次之后，阴标记不少于这个数才放环。不消耗标记。
	static constexpr int kMarkTrigger_{ 2 };
	// 少阴的条纹颜色编号，黑色。
	static constexpr std::uint32_t kColorId_{ 2u };
	// 小于 1 时宿主外层按视线变淡。具体数值不决定轮廓有多实。别的球保持 1。
	static constexpr float kOuterOpacity_{ 0.8f };
};
