#include "Enemy.h"
#include "ObjectCodex.h"
#include "ColliderComponentBase.h"
#include "Collision3D.h"
#include "XMath.h"

#include <algorithm>

/**
 * @brief 复用基类场地/对弹，再解算敌人互挤。对玩家不写位置、不消速度（Player 调 ShoveXZ）。
 */
void Enemy::MapItemCollide()
{
	Character::MapItemCollide();

	auto* selfCol = GetComponent<ColliderComponentBase>();
	if (selfCol == nullptr || !selfCol->IsEnabled())
	{
		return;
	}
	if (selfCol->GetCollideType() != Collider3D::CollideType::Box)
	{
		return;
	}

	auto Position = GetPosition();
	auto CollHalf = selfCol->GetCollisionSize();
	const float OwnerTop = Position.y + CollHalf.y;
	const float OwnerBottom = Position.y - CollHalf.y;
	const float OwnerRight = Position.x + CollHalf.x;
	const float OwnerLeft = Position.x - CollHalf.x;
	const float OwnerBack = Position.z + CollHalf.z;
	const float OwnerFront = Position.z - CollHalf.z;
	const float OwnerOldTop = PositionOld.y + CollHalf.y;
	const float OwnerOldBottom = PositionOld.y - CollHalf.y;
	const float OwnerOldRight = PositionOld.x + CollHalf.x;
	const float OwnerOldLeft = PositionOld.x - CollHalf.x;
	const float OwnerOldBack = PositionOld.z + CollHalf.z;
	const float OwnerOldFront = PositionOld.z - CollHalf.z;

	auto setPosComponent = [this](char axis, float value)
	{
		auto p = GetPosition();
		switch (axis)
		{
		case 'x': p.x = value; break;
		case 'y': p.y = value; break;
		case 'z': p.z = value; break;
		default: break;
		}
		SetPosition(p);
	};

	std::vector<Character*> mapCharacters;
	for (auto tag : kCharacterTags)
	{
		auto found = ObjectCodex::FindActiveObjectsByTag<Character>(tag);
		mapCharacters.reserve(mapCharacters.size() + found.size());
		mapCharacters.insert(mapCharacters.end(), found.begin(), found.end());
	}

	for (auto* c : mapCharacters)
	{
		auto* cCol = c->GetComponent<ColliderComponentBase>();
		if (this == c || GetIsDeath() || c->GetIsDeath() ||
			c->GetTag() == character_Player ||
			cCol == nullptr || !cCol->IsEnabled())
		{
			continue;
		}

		if (!CollisionSystem::IsOverlap(selfCol->GetVolume(), cCol->GetVolume()))
		{
			continue;
		}

		const auto ItemPosition = c->GetPosition();
		const auto ItemCollHalf = cCol->GetCollisionSize();
		const float ItemTop = ItemPosition.y + ItemCollHalf.y;
		const float ItemBottom = ItemPosition.y - ItemCollHalf.y;
		const float ItemRight = ItemPosition.x + ItemCollHalf.x;
		const float ItemLeft = ItemPosition.x - ItemCollHalf.x;
		const float ItemBack = ItemPosition.z + ItemCollHalf.z;
		const float ItemFront = ItemPosition.z - ItemCollHalf.z;

		const float push_rate = 0.2f;
		const float push_rate_half = push_rate / 2.0f;

		if (OwnerOldBottom >= ItemTop && OwnerBottom <= ItemTop)
		{
			MoveVelocity.y = 0.0f;
			setPosComponent('y', ItemTop + CollHalf.y + 0.001f);

			const float UpOrDown = (c->GetPosition().z - PositionOld.z > 0.0f) ? 1.0f : -1.0f;
			const float RightOrLeft = (c->GetPosition().x - PositionOld.x > 0.0f) ? 1.0f : -1.0f;
			c->CalculateMoveVelocity(GetMoveAccel() * push_rate * RightOrLeft, 0.0f, GetMoveAccel() * push_rate * UpOrDown);

			OnFloor = true;
			continue;
		}
		if (OwnerOldTop <= ItemBottom && OwnerTop >= ItemBottom)
		{
			MoveVelocity.y = 0.0f;
			setPosComponent('y', ItemBottom - CollHalf.y - 0.1f);
			continue;
		}
		if (OwnerOldRight <= ItemLeft && OwnerRight >= ItemLeft)
		{
			setPosComponent('x', ItemLeft - CollHalf.x - 0.1f);
			const float UpOrDown = (c->GetPosition().z - PositionOld.z > 0.0f) ? 1.0f : -1.0f;
			c->CalculateMoveVelocity(GetMoveAccel() * push_rate, 0.0f, GetMoveAccel() * push_rate_half * UpOrDown);
			continue;
		}
		if (OwnerOldLeft >= ItemRight && OwnerLeft <= ItemRight)
		{
			setPosComponent('x', ItemRight + CollHalf.x + 0.1f);
			const float UpOrDown = (c->GetPosition().z - PositionOld.z > 0.0f) ? 1.0f : -1.0f;
			c->CalculateMoveVelocity(-GetMoveAccel() * push_rate, 0.0f, GetMoveAccel() * push_rate_half * UpOrDown);
			continue;
		}
		if (OwnerOldBack <= ItemFront && OwnerBack >= ItemFront)
		{
			setPosComponent('z', ItemFront - CollHalf.z - 0.1f);
			const float RightOrLeft = (c->GetPosition().x - PositionOld.x > 0.0f) ? 1.0f : -1.0f;
			c->CalculateMoveVelocity(GetMoveAccel() * push_rate_half * RightOrLeft, 0.0f, GetMoveAccel() * push_rate);
			continue;
		}
		if (OwnerOldFront >= ItemBack && OwnerFront <= ItemBack)
		{
			setPosComponent('z', ItemBack + CollHalf.z + 0.1f);
			const float RightOrLeft = (c->GetPosition().x - PositionOld.x > 0.0f) ? 1.0f : -1.0f;
			c->CalculateMoveVelocity(GetMoveAccel() * push_rate_half * RightOrLeft, 0.0f, -GetMoveAccel() * push_rate);
			continue;
		}

		/**
		 * @brief 扫边未命中（已叠着 / 斜向挤入 / 出生重叠）时用 MTV 兜底。
		 *        双方各自 MapItemCollide，每边只推一半深度。
		 */
		DirectX::XMFLOAT3 n{};
		float depth = 0.0f;
		if (CollisionSystem::TrySeparate(selfCol->GetVolume(), cCol->GetVolume(), n, depth))
		{
			const float push = (std::max)(depth, 0.0f) * 0.5f;
			if (push > 0.0f)
			{
				SetPosition(V(GetPosition()) + V(n) * push);
				selfCol->SyncFromOwner();
			}
			const float vn = Dot(V(MoveVelocity), V(n));
			if (vn < 0.0f)
			{
				MoveVelocity = (V(MoveVelocity) - V(n) * vn).ToFloat3();
			}
		}
	}
}
