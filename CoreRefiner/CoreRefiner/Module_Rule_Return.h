#pragma once
#include "IModule.h"
#include "AttackBase_Moduled.h"
#include "AttackManager.h"
#include "InputCodex.h"
#include "Player.h"
#include "XMath.h"
#include "Marker.h"
#include "ObjectCodex.h"
#include "RenderGraph.h"

// 只作用于主体：开火时出现在鼠标世界坐标，速度指向当时的玩家。
// launchPosLocked 时不改位置（留给 Revive），标志仍在当前落点弹出；仍把速度改成朝向玩家。
// 与绕玩家 Orbit 同时存在时，由 Orbit 从该起点边转到公转半径。
class Module_Rule_Return : public IModule
{
public:
	Module_Rule_Return(AttackBase* owner, Graphics* gfx, Rgph::RenderGraph* rg, Player* player) noexcept
		:
		IModule(owner),
		gfx_(gfx),
		rg_(rg),
		player_(player)
	{}

	void OnSpawn() override
	{
		AttackBase* owner = GetOwner();
		if (owner == nullptr || player_ == nullptr || !player_->IsActive())
		{
			return;
		}

		// 发射点锁住时不再改位置。标志仍按这一发真正的落点弹出。
		const auto* moduled = dynamic_cast<const AttackBase_Moduled*>(owner);
		const bool locked = moduled != nullptr && moduled->IsLaunchPosLocked();
		XMFLOAT3 markerPos = owner->GetWorldPosition();
		bool showMarker = locked;
		if (!locked && gfx_ != nullptr)
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
				markerPos = world;
				showMarker = true;
			}
		}

		const Vec3 toPlayer = V(player_->GetWorldPosition()) - V(owner->GetWorldPosition());
		XMFLOAT3 dir{ toPlayer.x, 0.0f, toPlayer.z };
		if (NormalizeXZ(dir))
		{
			owner->SetMoveAccel((V(dir) * AttackManager::kAimSpeed).ToFloat3());
		}
		if (showMarker)
		{
			SpawnMarker_(markerPos);
		}
	}

	void OnRecycle() override {}

private:
	// 在这一发的落点弹出回场节点的标志。普通发射用鼠标落点，重生用锁住的当前位置。
	void SpawnMarker_(XMFLOAT3 pos) const
	{
		if (gfx_ == nullptr || rg_ == nullptr)
		{
			return;
		}
		Marker* marker = ObjectCodex::SpawnPooled<Marker>(effect_Marker, *gfx_, *rg_);
		if (marker == nullptr)
		{
			return;
		}
		marker->SetLogo(ModuleNodeLabel::Rule_Return);
		marker->SpawnAt(pos, false);
		ObjectCodex::Handoff(effect_Marker, marker);
	}

	Graphics* gfx_{ nullptr };
	// 造标志和把技术接到渲染图时用。
	Rgph::RenderGraph* rg_{ nullptr };
	Player* player_{ nullptr };
};
