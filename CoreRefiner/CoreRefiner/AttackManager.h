#pragma once
#include "Graphics.h"
#include "RenderGraph.h"
#include "Player.h"
#include "Attack.h"
#include "GameRulesCodex.h"

#include <deque>
#include <functional>
#include <vector>

class AttackManager
{
public:
	AttackManager(Graphics& gfx, Rgph::RenderGraph& rg);
	~AttackManager() = default;
	void Update(float dt);
	void Submit(void);
	void Reset(void);

	// 把组装好的根弹按顺序排进连发队列。放出时读玩家当下的位置和准星。
	// roots 是这一批根弹。
	void QueueRoots(const std::vector<Attack*>& roots);

	// 每颗弹真正出生后调用。atk 是这颗弹。出生会清掉数值修正，仓库加成放在这里加。
	void SetOnLaunched(std::function<void(Attack&)> cb) { onLaunched_ = std::move(cb); }

	// 把已组装、未走连发队列的弹纳入更新列表（停放转活体；不 SpawnAt）。
	void AdoptLive(Attack* attack);

	// 瞄准平面上的基准水平速度（再乘 speed.Final()）。
	static constexpr float kAimSpeed{ 0.05f };

	// 屏幕像素 → 指定高度水平面的世界坐标。
	[[nodiscard]] static bool TryScreenToWorldXZ(
		Graphics& gfx,
		float sx,
		float sy,
		float targetY,
		DirectX::XMFLOAT3& outWorld);

	// Mouse aim on player Y plane → horizontal shot velocity.
	[[nodiscard]] bool TryGetAimVelocity(
		DirectX::XMFLOAT3 playerPos,
		DirectX::XMFLOAT3& outVel) const;

private:
	Graphics& gfx;
	Rgph::RenderGraph& rg;

	std::vector<Attack*> attacks;

	// 缓存的玩家。空或已失效时，放出前重新查找。
	Player* pPlayer{ nullptr };
	// 出生后对树上每一颗还活着的弹调用。
	std::function<void(Attack&)> onLaunched_{};

	// 排队中的一颗根弹。root 还没出生。tree 是入队时拍平的整棵树。
	struct PendingShot
	{
		Attack* root{ nullptr };
		// 根弹和子弹。公转弹出生后会卸父子，加成仍按这份名单加。
		std::vector<Attack*> tree;
	};

	// 还没放出的根弹，队首先放。
	std::deque<PendingShot> pendingShots_;
	// 离上一颗放出过了多少秒。空队列时封顶在一个间隔，所以下一颗不用等。
	float sinceLastFire_{ GameRulesCodex::FireInterval };

	// 只收养「父弹卸下时打了标记」的弹，不收组装中的主体。
	void AdoptUnparentedAttacks_();
	// 取出队首放出一颗。队列空时什么都不做。找不到玩家时回收这一颗。
	void ReleaseNextShot_();
	// 把 atk 和它的子弹拍进 out。入队时调用，公转卸下父子后仍能按这份名单加仓库加成。
	static void CollectAttackTree(Attack* atk, std::vector<Attack*>& out);
};
