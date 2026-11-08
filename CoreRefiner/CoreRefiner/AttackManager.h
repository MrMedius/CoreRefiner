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

	// 把组装好的根弹按顺序排进连发队列。
	// roots 是这一批根弹。pos、vel 保留给调用方，放出时改读玩家当下的位置和准星。
	// onFired 在每一颗真正 SpawnAt 之后调用，参数是这颗根弹。出生会清掉数值修正，加成放在这里加。
	void FireRoots(
		const std::vector<Attack*>& roots,
		DirectX::XMFLOAT3 pos,
		DirectX::XMFLOAT3 vel,
		const std::function<void(Attack*)>& onFired);

	// 把已组装、未走 FireRoots 的弹纳入更新列表（停放转活体；不 SpawnAt）。
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
	int playerRemote{ 0 };

	Player* pPlayer;

	// 排队中的一颗根弹。root 还没出生。onFired 在它出生后调用。
	// colliderRestore 是排队前碰撞开着的弹，放出后重新打开。
	struct PendingShot
	{
		Attack* root{ nullptr };
		std::function<void(Attack*)> onFired;
		std::vector<Attack*> colliderRestore;
	};

	// 还没放出的根弹，队首先放。
	std::deque<PendingShot> pendingShots_;
	// 离上一颗放出过了多少秒。一开始等于连发间隔，所以队列空着时第一颗不用等。
	float sinceLastFire_{ GameRulesCodex::FireInterval };

	// 只收养「父弹卸下时打了标记」的弹，不收组装中的主体。
	void AdoptUnparentedAttacks_();
	// 取出队首放出一颗。队列空时什么都不做。找不到玩家时回收这一颗。
	void ReleaseNextShot_();
	// 排队期间关掉这棵树上开着的碰撞，并把它们记进 restore。敌人不会提前打爆还没射出的弹。
	void MuteColliders_(Attack* attack, std::vector<Attack*>& restore);
};
