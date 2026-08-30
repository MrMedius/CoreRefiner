#pragma once
#include "Graphics.h"
#include "RenderGraph.h"
#include "Player.h"
#include "Attack.h"

#include <vector>

class AttackManager
{
public:
	AttackManager(Graphics& gfx, Rgph::RenderGraph& rg);
	~AttackManager() = default;
	void Update(float dt);
	void Submit(void);
	void Reset(void);

	/**
	 * @brief Spawn assembled roots into the live attack list.
	 */
	void FireRoots(
		const std::vector<Attack*>& roots,
		DirectX::XMFLOAT3 pos,
		DirectX::XMFLOAT3 vel);

	/**
	 * @brief Mouse aim on player Y plane → horizontal shot velocity.
	 */
	[[nodiscard]] bool TryGetAimVelocity(
		DirectX::XMFLOAT3 playerPos,
		DirectX::XMFLOAT3& outVel) const;

private:
	Graphics& gfx;
	Rgph::RenderGraph& rg;

	std::vector<Attack*> attacks;
	int playerRemote{ 0 };

	Player* pPlayer;

	/** @brief 只收养「父弹卸下时打了标记」的弹，不收组装中的主体。 */
	void AdoptUnparentedAttacks_();
};
