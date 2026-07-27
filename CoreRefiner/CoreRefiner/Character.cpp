#include <math.h>
#include "Character.h"
#include "Environment.h"
#include "Attack.h"
#include "ObjectCodex.h"
#include "Collision.h"

void Character::MapItemCollide(void)
{
	// 上下左右の当たり方向を検出するため、oldを用いたチェックを行う
	auto Position = GetPosition();
	auto CollHalf = GetCollisionSize();
	float OwnerTop =	Position.y + CollHalf.y; // プレイヤーの上端
	float OwnerBottom =	Position.y - CollHalf.y; // プレイヤーの下端
	float OwnerRight =	Position.x + CollHalf.x; // プレイヤーの右端
	float OwnerLeft =	Position.x - CollHalf.x; // プレイヤーの左端
	float OwnerBack =	Position.z + CollHalf.z; // プレイヤーの後端
	float OwnerFront =	Position.z - CollHalf.z; // プレイヤーの前端
	float OwnerOldTop =		PositionOld.y + CollHalf.y;	// プレイヤーの古い位置の上端
	float OwnerOldBottom =	PositionOld.y - CollHalf.y;	// プレイヤーの古い位置の下端
	float OwnerOldRight =	PositionOld.x + CollHalf.x;	// プレイヤーの古い位置の右端
	float OwnerOldLeft =	PositionOld.x - CollHalf.x;	// プレイヤーの古い位置の左端
	float OwnerOldBack =	PositionOld.z + CollHalf.z;	// プレイヤーの古い位置の後端
	float OwnerOldFront =	PositionOld.z - CollHalf.z;	// プレイヤーの古い位置の前端
	float ItemTop, ItemBottom, ItemRight, ItemLeft, ItemBack, ItemFront;

	/**
	 * @brief Write a single position axis through the host Transformation API.
	 */
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

	/*------------------------------------------------------------------------------
	   MapEnvironment
	------------------------------------------------------------------------------*/
	OnFloor = false; //先ずは床にいないを想定する

	std::vector<Environment*> mapEnvironment;
	for (auto tag : {
		environment_Field
		}) {
		// 全てのマップオブジェクトを探す
		auto found = ObjectCodex::FindActiveObjectsByTag<Environment>(tag);
		// 全てのマップオブジェクトをマップオブジェクトをして整理する
		mapEnvironment.reserve(mapEnvironment.size() + found.size());
		mapEnvironment.insert(mapEnvironment.end(), found.begin(), found.end());
	}

	// ループでコリジョン判断
	for (auto e : mapEnvironment)
	{
		if (!e->GetCollisionOnOff()) continue;

		bool isCollide = CollisionSystem::IsOverlap(GetBoxCollider(), e->GetBoxCollider());

		if (isCollide)
		{
			auto ItemPosition = e->GetPosition();
			auto ItemCollHalf = e->GetCollisionSize();
			ItemTop =	 ItemPosition.y + ItemCollHalf.y;	// ブロックの上端
			ItemBottom = ItemPosition.y - ItemCollHalf.y;	// ブロックの下端
			ItemRight =	 ItemPosition.x + ItemCollHalf.x;	// ブロックの右端
			ItemLeft =	 ItemPosition.x - ItemCollHalf.x;	// ブロックの左端
			ItemBack =	 ItemPosition.z + ItemCollHalf.z;	// ブロックの後端
			ItemFront =	 ItemPosition.z - ItemCollHalf.z;	// ブロックの前端

			// 環境要素の当たり処理を呼び出す
			e->OnCollide(this);

			// 上から下に当たった(乗った)
			if (OwnerOldBottom >= ItemTop && OwnerBottom <= ItemTop)
			{
				// 下とブロックの上を比べ、下がoldの時は上側にあり、現在は下側にある場合
				// 着地
				MoveVelocity.y = 0.0f;
				// プレイヤーの場所を固定(地面に引っかからないように)
				setPosComponent('y', ItemTop + CollHalf.y + 0.001f);
				// 地面に乗っている
				OnFloor = true;
				continue;
			}
			// 下から上に当たった
			if (OwnerOldTop <= ItemBottom && OwnerTop >= ItemBottom)
			{
				// 上とブロックの下を比べ、上がoldの時は下側にあり、現在は上側にある場合
				// 止める
				MoveVelocity.y = 0.0f;
				// プレイヤーの場所を固定
				setPosComponent('y', ItemBottom - CollHalf.y - 0.1f);
				continue;
			}
			// 左から右に当たった
			if (OwnerOldRight <= ItemLeft && OwnerRight >= ItemLeft)
			{
				setPosComponent('x', ItemLeft - CollHalf.x - 0.1f); // 場所を固定
				continue;
			}
			// 右から左に当たった
			if (OwnerOldLeft >= ItemRight && OwnerLeft <= ItemRight)
			{
				setPosComponent('x', ItemRight + CollHalf.x + 0.1f); // 場所を固定
				continue;
			}
			// 後から前に当たった
			if (OwnerOldBack <= ItemFront && OwnerBack >= ItemFront)
			{
				setPosComponent('z', ItemFront - CollHalf.z - 0.1f); // 場所を固定
				continue;
			}
			// 前から後に当たった
			if (OwnerOldFront >= ItemBack && OwnerFront <= ItemBack)
			{
				setPosComponent('z', ItemBack + CollHalf.z + 0.1f); // 場所を固定
				continue;
			}
		}
	}


	/*------------------------------------------------------------------------------
	   MapCharacter
	------------------------------------------------------------------------------*/
	std::vector<Character*> mapCharacters;
	for (auto tag : {
		character_Player,
		character_Enemy_T,
		}) {
		// 全てのキャラクターを探す
		auto found = ObjectCodex::FindActiveObjectsByTag<Character>(tag);
		// 全てのキャラクターを整理する
		mapCharacters.reserve(mapCharacters.size() + found.size());
		mapCharacters.insert(mapCharacters.end(), found.begin(), found.end());
	}

	// ループでコリジョン判断
	for (auto c : mapCharacters)
	{
		if (this == c || this->IsDeath || c->IsDeath || !this->OnCollision || !c->OnCollision) continue;

		bool isCollide = CollisionSystem::IsOverlap(GetBoxCollider(), c->GetBoxCollider());

		if (isCollide)
		{
			auto ItemPosition = c->GetPosition();
			auto ItemCollHalf = c->GetCollisionSize();
			ItemTop =	 ItemPosition.y + ItemCollHalf.y;	// キャラクターの上端
			ItemBottom = ItemPosition.y - ItemCollHalf.y;	// キャラクターの下端
			ItemRight =	 ItemPosition.x + ItemCollHalf.x;	// キャラクターの右端
			ItemLeft =	 ItemPosition.x - ItemCollHalf.x;	// キャラクターの左端
			ItemBack =	 ItemPosition.z + ItemCollHalf.z;	// キャラクターの後端
			ItemFront =	 ItemPosition.z - ItemCollHalf.z;	// キャラクターの前端

			// 押し返す割合
			float push_rate = 0.2f;
			float push_rate_half = push_rate / 2.0f;

			// 上から下に当たった(乗った)
			if (OwnerOldBottom >= ItemTop && OwnerBottom <= ItemTop)
			{
				// 下とブロックの上を比べ、下がoldの時は上側にあり、現在は下側にある場合
				// 着地
				MoveVelocity.y = 0.0f;
				// 場所を固定(地面に引っかからないように)

				setPosComponent('y', ItemTop + CollHalf.y + 0.001f);

				float UpOrDown = (c->GetPosition().z - PositionOld.z > 0.0f) ? 1.0f : -1.0f;
				float RightOrLeft = (c->GetPosition().x - PositionOld.x > 0.0f) ? 1.0f : -1.0f;
				c->CalculateMoveVelocity(MoveAccel* push_rate* RightOrLeft, 0.0f, MoveAccel* push_rate* UpOrDown);

				// 地面に乗っている
				OnFloor = true;
				continue;
			}
			// 下から上に当たった
			if (OwnerOldTop <= ItemBottom && OwnerTop >= ItemBottom)
			{
				// 上とブロックの下を比べ、上がoldの時は下側にあり、現在は上側にある場合
				// 止める
				MoveVelocity.y = 0.0f;
				// プレイヤーの場所を固定
				setPosComponent('y', ItemBottom - CollHalf.y - 0.1f);
				continue;
			}
			// 左から右に当たった
			if (OwnerOldRight <= ItemLeft && OwnerRight >= ItemLeft)
			{
				setPosComponent('x', ItemLeft - CollHalf.x - 0.1f); // 場所を固定

				float UpOrDown = (c->GetPosition().z - PositionOld.z > 0.0f) ? 1.0f : -1.0f;
				c->CalculateMoveVelocity(MoveAccel* push_rate, 0.0f, MoveAccel* push_rate_half * UpOrDown);

				continue;
			}
			// 右から左に当たった
			if (OwnerOldLeft >= ItemRight && OwnerLeft <= ItemRight)
			{
				setPosComponent('x', ItemRight + CollHalf.x + 0.1f); // 場所を固定

				float UpOrDown = (c->GetPosition().z - PositionOld.z > 0.0f) ? 1.0f : -1.0f;
				c->CalculateMoveVelocity(-MoveAccel * push_rate, 0.0f, MoveAccel* push_rate_half * UpOrDown);

				continue;
			}
			// 後から前に当たった
			if (OwnerOldBack <= ItemFront && OwnerBack >= ItemFront)
			{
				setPosComponent('z', ItemFront - CollHalf.z - 0.1f); // 場所を固定

				float RightOrLeft = (c->GetPosition().x - PositionOld.x > 0.0f) ? 1.0f : -1.0f;
				c->CalculateMoveVelocity(MoveAccel * push_rate_half * RightOrLeft, 0.0f, MoveAccel * push_rate);

				continue;
			}
			// 前から後に当たった
			if (OwnerOldFront >= ItemBack && OwnerFront <= ItemBack)
			{
				setPosComponent('z', ItemBack + CollHalf.z + 0.1f); // 場所を固定

				float RightOrLeft = (c->GetPosition().x - PositionOld.x > 0.0f) ? 1.0f : -1.0f;
				c->CalculateMoveVelocity(MoveAccel * push_rate_half * RightOrLeft, 0.0f, -MoveAccel * push_rate);

				continue;
			}
		}
	}

	/*------------------------------------------------------------------------------
	   MapAttack_Player
	------------------------------------------------------------------------------*/
	if (this->Tag != character_Player)
	{
		std::vector<Attack*> mapAttack_P;
		for (auto tag : {
			attack_Ball,
			}) {
			// 全てのキャラクターを探す
			auto found = ObjectCodex::FindActiveObjectsByTag<Attack>(tag);
			// 全てのキャラクターを整理する
			mapAttack_P.reserve(mapAttack_P.size() + found.size());
			mapAttack_P.insert(mapAttack_P.end(), found.begin(), found.end());
		}

		// ループでコリジョン判断
		for (auto a : mapAttack_P)
		{
			if (!a->GetCollisionOnOff()) continue;

			bool isCollide = CollisionSystem::IsOverlap(GetBoxCollider(), a->GetBoxCollider());

			if (isCollide)
			{
				a->OnCollide(this);
			}
		}
	}
}