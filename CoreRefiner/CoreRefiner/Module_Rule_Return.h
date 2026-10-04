#pragma once
#include "IModule.h"
#include "Attack.h"
#include "AttackManager.h"
#include "InputCodex.h"
#include "Player.h"
#include "XMath.h"

// 只作用于主体：开火时出现在鼠标世界坐标，速度指向当时的玩家。
// launchPosLocked 时不改位置（留给 Revive）；仍把速度改成朝向玩家。
// 与绕玩家 Orbit 同时存在时，由 Orbit 从该起点边转到公转半径。
class Module_Rule_Return : public IModule
{
public:
	Module_Rule_Return(Attack* owner, Graphics* gfx, Player* player) noexcept
		:
		IModule(owner),
		gfx_(gfx),
		player_(player)
	{}

	[[nodiscard]] bool HasModuleNodeLabel() const noexcept override { return true; }
	[[nodiscard]] ModuleNodeLabel GetModuleNodeLabel() const noexcept override
	{
		return ModuleNodeLabel::Rule_Return;
	}

	void OnSpawn() override
	{
		Attack* owner = GetOwner();
		if (owner == nullptr || player_ == nullptr || !player_->IsActive())
		{
			return;
		}

		if (!owner->IsLaunchPosLocked() && gfx_ != nullptr)
		{
			const auto mouse = InputCodex::Get().MousePos();
			XMFLOAT3 world{};
			if (AttackManager::TryScreenToWorldXZ(
				*gfx_,
				static_cast<float>(mouse.first),
				static_cast<float>(mouse.second),
				player_->GetWorldPosition().y,
				world))
			{
				owner->SetLocalPosition(world);
			}
		}

		const Vec3 toPlayer = V(player_->GetWorldPosition()) - V(owner->GetWorldPosition());
		XMFLOAT3 dir{ toPlayer.x, 0.0f, toPlayer.z };
		if (NormalizeXZ(dir))
		{
			owner->SetMoveAccel((V(dir) * AttackManager::kAimSpeed).ToFloat3());
		}
	}

	void OnRecycle() override {}

private:
	Graphics* gfx_{ nullptr };
	Player* player_{ nullptr };
};
