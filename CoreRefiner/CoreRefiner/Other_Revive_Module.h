#pragma once
#include "IProjectileModule.h"
#include "Attack.h"

/**
 * @brief 挂在主体 A 上：A 开火；A 玩法死亡时在消失点放出停放克隆 B（不复制本模块）。
 * @note 扫描中途 Reset / 池回收走直接 Deactivate，此时 owner 仍 Active，只丢弃克隆不放弹。
 */
class Other_Revive_Module : public IProjectileModule
{
public:
	Other_Revive_Module(Attack* owner, Attack* clone) noexcept
		:
		IProjectileModule(owner),
		clone_(clone)
	{}

	[[nodiscard]] bool HasModuleNodeLabel() const noexcept override { return true; }
	[[nodiscard]] ModuleNodeLabel GetModuleNodeLabel() const noexcept override
	{
		return ModuleNodeLabel::Other_Revive;
	}

	void OnSpawn() override
	{
		armed_ = true;
	}

	void OnDisable() override
	{
		Attack* owner = GetOwner();
		if (clone_ == nullptr || owner == nullptr || !armed_ || owner->IsActive())
		{
			DiscardClone_();
			return;
		}

		ReleaseClone_(owner);
	}

	void OnRecycle() override
	{
		DiscardClone_();
		armed_ = false;
	}

private:
	/**
	 * @brief 在 A 的消失点激活 B，锁生成点，交给 AdoptLive；不把 Revive 装到 B 上。
	 */
	void ReleaseClone_(Attack* owner)
	{
		Attack* b = clone_;
		clone_ = nullptr;
		if (b == nullptr)
		{
			return;
		}

		const DirectX::XMFLOAT3 pos = owner->GetWorldPosition();
		DirectX::XMFLOAT3 dir = owner->GetMoveVelocity();
		if (dir.x == 0.0f && dir.y == 0.0f && dir.z == 0.0f)
		{
			dir = owner->GetMoveAccel();
		}

		b->SetLaunchPosLocked(true);
		owner->AdoptLive(b);
		b->Activate();
		b->SetReviveParked(false);
		b->SpawnAt(pos, dir);
	}

	/** @brief 未开火或非玩法死亡：清停放标记并回收 B。 */
	void DiscardClone_()
	{
		if (clone_ == nullptr)
		{
			return;
		}
		Attack* b = clone_;
		clone_ = nullptr;
		b->SetReviveParked(false);
		b->Deactivate();
	}

	Attack* clone_{ nullptr };
	bool armed_{ false };
};
