#pragma once

#include "AttackDeployer.h"
#include "ModuleField.h"
#include "ScanWave.h"

#include <cmath>
#include <utility>
#include <vector>

/**
 * @brief Visual scan session: Apply field nodes into DeployContext via expanding waves.
 * @note Visual scan session driving DeployContext; FireRoots is owned by UI_Game.
 */
class ScanAssembler
{
public:
	static constexpr float kDefaultMaxRadius = 140.0f;
	static constexpr float kDefaultExpandSpeed = 100.0f;

	[[nodiscard]] bool IsSessionActive() const noexcept { return sessionActive_; }

	[[nodiscard]] bool HasPendingFire() const noexcept { return pendingFire_; }

	[[nodiscard]] const ScanWave& GetWave() const noexcept { return wave_; }

	[[nodiscard]] bool CanStart(const ModuleField& field) const noexcept
	{
		if (sessionActive_)
		{
			return false;
		}
		const FieldModuleNode* core = field.GetCore();
		return core != nullptr && core->IsReady();
	}

	/**
	 * @brief Start session: reset ctx, Apply core Spawn, cool core, open first wave.
	 */
	void Begin(
		ModuleField& field,
		Graphics& gfx,
		Rgph::RenderGraph& rg,
		DirectX::XMFLOAT3 spawnPos)
	{
		if (!CanStart(field))
		{
			return;
		}

		FieldModuleNode* core = field.GetCore();
		if (core == nullptr)
		{
			return;
		}

		ctx_ = {};
		ctx_.standby.gfx = &gfx;
		ctx_.standby.rg = &rg;
		ctx_.standby.spawnPos = spawnPos;
		pendingFire_ = false;
		committedShots_.clear();

		const std::size_t shotsBefore = ctx_.shots.size();
		core->ApplyTo(ctx_);
		core->StartCooldown();
		lastSource_ = core;

		if (ctx_.shots.size() > shotsBefore)
		{
			EndSessionWithShots_();
			return;
		}

		wave_.Start(core, core->GetLocalPos(), kDefaultMaxRadius, kDefaultExpandSpeed);
		sessionActive_ = true;
	}

	/**
	 * @brief Expand wave; on hit Apply+swap source; on exhaust or Spawn-flush end session.
	 */
	void Update(float dt, ModuleField& field)
	{
		if (!sessionActive_ || !wave_.alive)
		{
			return;
		}

		const float radiusBefore = wave_.radius;
		const bool exhausted = wave_.Expand(dt);
		const float radiusAfter = wave_.radius;

		if (TryHitAndChain_(field, radiusBefore, radiusAfter))
		{
			return;
		}

		if (exhausted)
		{
			ForceCommit();
		}
	}

	/**
	 * @brief Wave exhausted or external stop: FlushStandby and mark pending fire.
	 */
	void ForceCommit()
	{
		if (!sessionActive_)
		{
			return;
		}
		wave_.Stop();
		ctx_.FlushStandby();
		EndSessionWithShots_();
	}

	/**
	 * @brief Take ownership of committed roots (clears pending flag).
	 */
	std::vector<Attack*> TakeShots()
	{
		pendingFire_ = false;
		std::vector<Attack*> out;
		out.reserve(committedShots_.size());
		for (Attack* root : committedShots_)
		{
			if (root != nullptr)
			{
				out.push_back(root);
			}
		}
		committedShots_.clear();
		return out;
	}

private:
	void EndSessionWithShots_()
	{
		wave_.Stop();
		sessionActive_ = false;
		lastSource_ = nullptr;

		committedShots_.clear();
		for (Attack* root : ctx_.shots)
		{
			if (root != nullptr)
			{
				committedShots_.push_back(root);
			}
		}
		ctx_.shots.clear();
		pendingFire_ = !committedShots_.empty();
	}

	/**
	 * @brief Apply node; if shots grew (Spawn flush), end session; else cool+new wave.
	 * @return true if session ended or chain restarted (caller should skip exhaust).
	 */
	bool ApplyHit_(FieldModuleNode& node)
	{
		const std::size_t shotsBefore = ctx_.shots.size();
		node.ApplyTo(ctx_);

		wave_.Stop();
		node.StartCooldown();
		lastSource_ = &node;

		if (ctx_.shots.size() > shotsBefore)
		{
			EndSessionWithShots_();
			return true;
		}

		wave_.Start(&node, node.GetLocalPos(), kDefaultMaxRadius, kDefaultExpandSpeed);
		return true;
	}

	/**
	 * @brief First Ready node whose disk is crossed by the ring this frame.
	 */
	bool TryHitAndChain_(ModuleField& field, float radiusBefore, float radiusAfter)
	{
		FieldModuleNode* best = nullptr;
		float bestAbs = 1.0e9f;

		const float cx = wave_.center.x;
		const float cy = wave_.center.y;

		field.ForEach([&](FieldModuleNode& node)
		{
			if (!node.IsReady())
			{
				return;
			}
			if (&node == wave_.source)
			{
				return;
			}

			const float dx = node.GetLocalPos().x - cx;
			const float dy = node.GetLocalPos().y - cy;
			const float dist = std::sqrt(dx * dx + dy * dy);
			const float hitR = node.GetHitRadius();

			const float inner = radiusBefore - hitR;
			const float outer = radiusAfter + hitR;
			if (dist < inner || dist > outer)
			{
				return;
			}

			const float absDelta = std::fabs(dist - radiusAfter);
			if (absDelta < bestAbs)
			{
				bestAbs = absDelta;
				best = &node;
			}
		});

		if (best == nullptr)
		{
			return false;
		}

		ApplyHit_(*best);
		return true;
	}

	DeployContext ctx_{};
	ScanWave wave_{};
	bool sessionActive_{ false };
	bool pendingFire_{ false };
	FieldModuleNode* lastSource_{ nullptr };
	std::vector<Attack*> committedShots_;
};
