#pragma once

#include "Attack.h"
#include "Graphics.h"
#include "RenderGraph.h"
#include "Player.h"

#include <cstddef>
#include <functional>
#include <tuple>
#include <utility>
#include <vector>

enum class DeployTarget : unsigned char
{
	Focus,
	ShotRoot,
};

struct AttackStandby
{
	Graphics* gfx{ nullptr };
	Rgph::RenderGraph* rg{ nullptr };
	DirectX::XMFLOAT3 spawnPos{ 0.0f, 0.0f, 0.0f };

	Attack* parent{ nullptr };
	std::vector<Attack*> children;
	Attack* host{ nullptr };
	Player* player{ nullptr };
	/** @brief 当前 parent 是停放克隆，FlushStandby 不得把它当根弹发射。 */
	bool parked{ false };
};

struct DeployContext
{
	AttackStandby standby;
	std::vector<Attack*> shots;
	/** @brief 当前未 Flush 的流水线 Step；FlushStandby 时封到根弹。 */
	std::vector<AttackStepRecord> recipe;
	// 已完成配方组在 recipe 中的起点，不含当前组。
	// 组 i 的范围是 [recipeGroupStarts[i], 下一组起点)；最后一组的终点是 recipeGroupStart。
	std::vector<std::size_t> recipeGroupStarts;
	// 当前组起点。Flush 后归 0，本组从新配方开头继续记。
	std::size_t recipeGroupStart{ 0 };
	// 等待 host 填入后再执行的安装。没有父弹时也先入队，新根弹生成后 Drain。
	std::vector<std::function<void(DeployContext&)>> pitQueue;
	// 扫到 Revive 之后为 true：后续 Step 只 Record，不改当前树。
	// 由 Rule_Revive 的 Apply 置位；新 Context 从 false 开始。
	bool recordOnly{ false };

	/**
	 * @brief 记下一条已成功 Apply 的 Step。
	 */
	void Record(const AttackStepRecord& rec)
	{
		recipe.push_back(rec);
	}

	/**
	 * @brief Revive 之后只记账；调用方若得到 true 应立刻 return。
	 */
	[[nodiscard]] bool TryRecordOnly(const AttackStepRecord& rec)
	{
		if (!recordOnly)
		{
			return false;
		}
		Record(rec);
		return true;
	}

	void FlushStandby()
	{
		if (standby.parent != nullptr)
		{
			standby.parent->SealAssembledRecipe(std::move(recipe));
			if (!standby.parked)
			{
				shots.push_back(standby.parent);
			}
		}
		recipe.clear();
		recipeGroupStarts.clear();
		recipeGroupStart = 0;
		standby.parent = nullptr;
		standby.children.clear();
		standby.host = nullptr;
		standby.parked = false;
		recordOnly = false;
		pitQueue.clear();
	}

	
	// 把安装动作挂到等待队列；Core / Spawn 生成主体后 DrainPitQueue 再跑。
	void EnqueueOnEmptyPit(std::function<void(DeployContext&)> job)
	{
		pitQueue.push_back(std::move(job));
	}

	// 结束上一组（没有新记录则不记），从 recipe 末尾开一组。
	// 出球在组中途 Flush 时，上面的归零会让本组改从新配方开头继续。
	void BeginRecipeGroup()
	{
		if (recipe.size() > recipeGroupStart)
		{
			recipeGroupStarts.push_back(recipeGroupStart);
		}
		recipeGroupStart = recipe.size();
	}

	//对当前 host 执行坑队列（先移出再跑，避免 Drain 中再次入队套娃）。
	void DrainPitQueue()
	{
		auto jobs = std::move(pitQueue);
		pitQueue.clear();
		for (auto& job : jobs)
		{
			if (job)
			{
				job(*this);
			}
		}
	}
};

// 打 standby.host。
// host 空则入队，等 Core / Spawn 生成主体后再装。没有父弹时也入队。
template <typename T, typename... Args>
T* AddToFocus(DeployContext& ctx, const AttackStepRecord& rec, Args&&... args)
{
	if (ctx.TryRecordOnly(rec))
	{
		return nullptr;
	}
	if (ctx.standby.host == nullptr)
	{
		ctx.EnqueueOnEmptyPit(
			[captured = std::make_tuple(std::forward<Args>(args)...), rec](DeployContext& c) mutable
			{
				std::apply([&c, rec](auto&&... a)
				{
					AddToFocus<T>(c, rec, std::forward<decltype(a)>(a)...);
				}, std::move(captured));
			});
		return nullptr;
	}
	T* raw = ctx.standby.host->AddModule<T>(std::forward<Args>(args)...);
	if (raw != nullptr)
	{
		ctx.Record(rec);
	}
	return raw;
}
