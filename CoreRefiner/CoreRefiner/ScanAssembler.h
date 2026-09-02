#pragma once

#include "AttackDeployer.h"
#include "ModuleField.h"
#include "ScanWave.h"
#include "XMath.h"

#include <cmath>
#include <utility>
#include <vector>

/**
 * @brief One independent scan/assemble run with its own DeployContext and wave.
 */
struct ScanSession
{
	DeployContext ctx{};
	ScanWave wave{};
	bool active{ false };
	bool pendingFire{ false };
	IModuleNode* lastSource{ nullptr };
	std::vector<Attack*> committedShots;
	std::size_t appliedTokenCount{ 0 };
	DirectX::XMFLOAT3 spawnPos{ 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 aimVel{ 0.0f, 0.0f, 0.0f };
};

/**
 * @brief One FireRoots submission produced by a finished scan session.
 */
struct FireBatch
{
	std::vector<Attack*> roots;
	DirectX::XMFLOAT3 pos{ 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 vel{ 0.0f, 0.0f, 0.0f };
};

/**
 * @brief Parallel scan sessions driving independent DeployContexts.
 * @note FireRoots ownership remains with ModuleWorkbench via TakeAllPendingFires().
 * @note Session mutation during Update uses indices so Detach push_back cannot dangle refs.
 */
class ScanAssembler
{
public:
	static constexpr std::size_t kMaxSessions = 8;
	static constexpr std::size_t kMaxAppliedTokens = 10;

	[[nodiscard]] bool IsSessionActive() const noexcept
	{
		for (const ScanSession& session : sessions_)
		{
			if (session.active)
			{
				return true;
			}
		}
		return false;
	}

	/**
	 * @brief Invoke @p fn for every alive scan wave (multi-ring draw).
	 */
	template<typename Fn>
	void ForEachAliveWave(Fn&& fn) const
	{
		for (const ScanSession& session : sessions_)
		{
			if (session.wave.alive)
			{
				fn(session.wave);
			}
		}
	}

	[[nodiscard]] bool CanStart(const ModuleField& field) const noexcept
	{
		if (sessions_.size() >= kMaxSessions)
		{
			return false;
		}
		const IModuleNode* core = field.GetCore();
		return core != nullptr && core->IsReady();
	}

	void Begin(
		ModuleField& field,
		Graphics& gfx,
		Rgph::RenderGraph& rg,
		DirectX::XMFLOAT3 spawnPos,
		DirectX::XMFLOAT3 aimVel,
		Player* player)
	{
		if (!CanStart(field))
		{
			return;
		}

		IModuleNode* core = field.GetCore();
		if (core == nullptr)
		{
			return;
		}

		EnsureSessionCapacity_();

		ScanSession session{};
		session.ctx.standby.gfx = &gfx;
		session.ctx.standby.rg = &rg;
		session.ctx.standby.spawnPos = spawnPos;
		session.ctx.standby.player = player;
		session.spawnPos = spawnPos;
		session.aimVel = aimVel;
		session.pendingFire = false;
		session.committedShots.clear();
		session.appliedTokenCount = 0;

		const std::size_t shotsBefore = session.ctx.shots.size();
		core->ApplyTo(session.ctx);
		++session.appliedTokenCount;
		core->StartCooldown();
		session.lastSource = core;

		sessions_.push_back(std::move(session));
		const std::size_t index = sessions_.size() - 1;

		if (sessions_[index].ctx.shots.size() > shotsBefore
			|| sessions_[index].appliedTokenCount >= kMaxAppliedTokens)
		{
			EndSessionWithShots_(index);
			return;
		}

		sessions_[index].wave.Start(
			core,
			core->GetLocalPos(),
			core->GetScanMaxRadius(),
			core->GetScanExpandSpeed());
		sessions_[index].active = true;
	}

	/**
	 * @brief Drop all sessions without firing; Deactivate any held Attack* back to pool.
	 */
	void Reset()
	{
		for (ScanSession& session : sessions_)
		{
			DiscardSessionAttacks_(session);
		}
		sessions_.clear();
	}

	/**
	 * @brief Advance active waves only. Does not erase; TakeAllPendingFires owns erase.
	 */
	void Update(float dt, ModuleField& field)
	{
		const std::size_t count = sessions_.size();
		for (std::size_t i = 0; i < count; ++i)
		{
			if (!sessions_[i].active || !sessions_[i].wave.alive)
			{
				continue;
			}

			const float radiusBefore = sessions_[i].wave.radius;
			const bool exhausted = sessions_[i].wave.Expand(dt);
			const float radiusAfter = sessions_[i].wave.radius;

			if (TryHitAndChain_(i, field, radiusBefore, radiusAfter))
			{
				continue;
			}

			if (exhausted)
			{
				ForceCommit_(i);
			}
		}
	}

	void ForceCommit()
	{
		const std::size_t count = sessions_.size();
		for (std::size_t i = 0; i < count; ++i)
		{
			ForceCommit_(i);
		}
	}

	/**
	 * @brief Abort all active scans: commit assembled shots for fire, cool down Ready nodes.
	 * @note Does not discard Attack*; caller should TakeAllPendingFires → FireRoots.
	 * @note Ready nodes enter cooldown so a Flush chain cannot resume the same frame.
	 */
	void ForceFinish(ModuleField& field)
	{
		ForceCommit();
		field.ForEach([](IModuleNode& node)
		{
			//if (node.IsReady())
			{
				node.StartCooldown();
			}
		});
	}

	/**
	 * @brief Take every pendingFire session as a FireBatch and erase those sessions.
	 * @note Also drops finished sessions with no pending fire (!active && !pendingFire).
	 */
	[[nodiscard]] std::vector<FireBatch> TakeAllPendingFires()
	{
		std::vector<FireBatch> batches;
		std::vector<ScanSession> keep;
		keep.reserve(sessions_.size());

		for (ScanSession& session : sessions_)
		{
			if (session.pendingFire)
			{
				FireBatch batch{};
				batch.pos = session.spawnPos;
				batch.vel = session.aimVel;
				batch.roots.reserve(session.committedShots.size());
				for (Attack* root : session.committedShots)
				{
					if (root != nullptr)
					{
						batch.roots.push_back(root);
					}
				}
				session.committedShots.clear();
				session.pendingFire = false;
				if (!batch.roots.empty())
				{
					batches.push_back(std::move(batch));
				}
				continue;
			}

			if (!session.active)
			{
				continue;
			}

			keep.push_back(std::move(session));
		}

		sessions_ = std::move(keep);
		return batches;
	}

private:
	void EnsureSessionCapacity_()
	{
		const std::size_t want = kMaxSessions + kMaxSessions;
		if (sessions_.capacity() < want)
		{
			sessions_.reserve(want);
		}
	}

	void ForceCommit_(std::size_t sessionIndex)
	{
		if (sessionIndex >= sessions_.size())
		{
			return;
		}
		if (!sessions_[sessionIndex].active)
		{
			return;
		}
		EndSessionWithShots_(sessionIndex);
	}

	void EndSessionWithShots_(std::size_t sessionIndex)
	{
		if (sessionIndex >= sessions_.size())
		{
			return;
		}

		ScanSession& session = sessions_[sessionIndex];
		session.wave.Stop();
		session.active = false;
		session.lastSource = nullptr;
		session.ctx.FlushStandby();

		session.committedShots.clear();
		for (Attack* root : session.ctx.shots)
		{
			if (root != nullptr)
			{
				session.committedShots.push_back(root);
			}
		}
		session.ctx.shots.clear();
		session.pendingFire = !session.committedShots.empty();
	}

	bool ApplyHit_(std::size_t sessionIndex, IModuleNode& node)
	{
		if (sessionIndex >= sessions_.size())
		{
			return false;
		}

		ScanSession& session = sessions_[sessionIndex];
		const std::size_t shotsBefore = session.ctx.shots.size();
		node.ApplyTo(session.ctx);
		++session.appliedTokenCount;

		session.wave.Stop();
		node.StartCooldown();
		session.lastSource = &node;

		const bool flushedShots = session.ctx.shots.size() > shotsBefore;

		/**
		 * Spawn_Ball Flush (core or ModuleNode_Spawn_Ball): park previous roots, keep new parent,
		 * reset token count, continue scan from this node. Fill-only hits just chain.
		 */
		if (flushedShots)
		{
			DetachFlushedShotsAsPending_(sessionIndex);
			sessions_[sessionIndex].appliedTokenCount = 1;
		}

		ScanSession& after = sessions_[sessionIndex];
		if (after.appliedTokenCount >= kMaxAppliedTokens)
		{
			EndSessionWithShots_(sessionIndex);
			return true;
		}

		after.wave.Start(
			&node,
			node.GetLocalPos(),
			node.GetScanMaxRadius(),
			node.GetScanExpandSpeed());
		after.active = true;
		after.pendingFire = false;
		return true;
	}

	/**
	 * @brief Park ctx.shots on a pendingFire sibling; leave standby on sessions_[index].
	 */
	void DetachFlushedShotsAsPending_(std::size_t sessionIndex)
	{
		if (sessionIndex >= sessions_.size())
		{
			return;
		}

		EnsureSessionCapacity_();

		ScanSession& session = sessions_[sessionIndex];
		ScanSession fireOnly{};
		fireOnly.spawnPos = session.spawnPos;
		fireOnly.aimVel = session.aimVel;
		fireOnly.active = false;
		fireOnly.committedShots.clear();
		for (Attack* root : session.ctx.shots)
		{
			if (root != nullptr)
			{
				fireOnly.committedShots.push_back(root);
			}
		}
		session.ctx.shots.clear();
		fireOnly.pendingFire = !fireOnly.committedShots.empty();
		if (fireOnly.pendingFire)
		{
			sessions_.push_back(std::move(fireOnly));
		}
	}

	bool TryHitAndChain_(
		std::size_t sessionIndex,
		ModuleField& field,
		float radiusBefore,
		float radiusAfter)
	{
		if (sessionIndex >= sessions_.size())
		{
			return false;
		}

		IModuleNode* best = nullptr;
		float bestAbs = 1.0e9f;

		const ScanWave& wave = sessions_[sessionIndex].wave;
		IModuleNode* const source = wave.source;

		field.ForEach([&](IModuleNode& node)
		{
			if (!node.IsReady())
			{
				return;
			}
			if (&node == source)
			{
				return;
			}

			const float dist = (V(node.GetLocalPos()) - V(wave.center)).Length();
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

		ApplyHit_(sessionIndex, *best);
		return true;
	}

	std::vector<ScanSession> sessions_;

	/**
	 * @brief Deactivate roots held by a session (committed, shots, standby parent).
	 * @note Children under parent are cascaded by Attack::Deactivate.
	 */
	static void DiscardSessionAttacks_(ScanSession& session)
	{
		auto deactivate = [](Attack* attack)
		{
			if (attack != nullptr && attack->IsActive())
			{
				attack->Deactivate();
			}
		};

		for (Attack* root : session.committedShots)
		{
			deactivate(root);
		}
		session.committedShots.clear();

		for (Attack* root : session.ctx.shots)
		{
			deactivate(root);
		}
		session.ctx.shots.clear();

		deactivate(session.ctx.standby.parent);
		for (Attack* child : session.ctx.standby.children)
		{
			deactivate(child);
		}
		session.ctx.standby.parent = nullptr;
		session.ctx.standby.children.clear();
		session.ctx.standby.host = nullptr;
		session.ctx.standby.parked = false;

		session.wave.Stop();
		session.active = false;
		session.pendingFire = false;
		session.lastSource = nullptr;
		session.appliedTokenCount = 0;
	}
};
