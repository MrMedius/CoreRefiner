#pragma once
#include "Graphics.h"
#include "RenderGraph.h"
#include "Player.h"
#include "AttackBase.h"
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
	void QueueRoots(const std::vector<AttackBase*>& roots);

	// 每颗弹真正出生后调用。atk 是这颗弹。出生会清掉数值修正，仓库加成放在这里加。
	void SetOnLaunched(std::function<void(AttackBase&)> cb) { onLaunched_ = std::move(cb); }

	// 把这颗弹放进 attacks。attack 是要更新的攻击。空指针和已经在名单里的不放。
	void AdoptLive(AttackBase* attack);

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

	// 已启动的攻击。根弹由连发放出时放进来。同一次出生的子弹和交出的白环由 ClaimAttack 放进来。
	std::vector<AttackBase*> attacks;

	// 缓存的玩家。空或已失效时，放出前重新查找。
	Player* pPlayer{ nullptr };
	// 出生后对树上每一颗还活着的弹调用。
	std::function<void(AttackBase&)> onLaunched_{};

	// 排队中的一颗根弹。root 还没出生。tree 是入队时拍平的整棵树。
	struct PendingShot
	{
		AttackBase* root{ nullptr };
		// 根弹和同一次出生、还没交出的子弹。加成按这份名单加。
		std::vector<AttackBase*> tree;
	};

	// 还没放出的根弹，队首先放。
	std::deque<PendingShot> pendingShots_;
	// 离上一颗放出过了多少秒。空队列时封顶在一个间隔，所以下一颗不用等。
	float sinceLastFire_{ GameRulesCodex::FireInterval };

	// 取走所有待认领的 AttackBase，放进 attacks。已经停用或已经在名单里的不放。
	void ClaimAttack();
	// 取出队首放出一颗。队列空时什么都不做。找不到玩家时回收这一颗。
	void ReleaseNextShot_();
	// 把 atk 和同一次出生、还没交出的子弹拍进 out。入队时调用，出生后仍按这份名单加仓库加成。
	static void CollectAttackTree(AttackBase* atk, std::vector<AttackBase*>& out);
};
