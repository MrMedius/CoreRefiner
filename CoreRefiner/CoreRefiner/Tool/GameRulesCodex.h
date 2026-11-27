#pragma once

#include <cstddef>
#include <cstdint>

// 全局写死的规则。这一局会变的钱、经验、语言仍在 GameStatsCodex。
class GameRulesCodex
{
public:
	GameRulesCodex() = delete;

	// 买卖。卖价是造价的一半，不单列。
	static constexpr int ForgeCost = 3;          // 锻造花费。没有奥义时按下锻造扣的钱
	static constexpr int PunchCost = 1;          // 打孔花费。已有奥义时容量加 1 扣的钱
	static constexpr int RefineOpCost = 1;       // 炼成花费。升级、融合、进化各扣一次
	static constexpr int SellConfirmPrice = 16;  // 卖出确认线。卖价达到这个数才弹确认
	static constexpr int DefaultBuyPrice = 4;    // 默认造价。卖价是它的一半

	// Node 与奥义。空奥义的四项用默认属性。
	static constexpr float DefaultStat = 10.0f;  // 默认属性。冷却、扫描、碰撞，以及空奥义的四项
	static constexpr int UltraBornCapacity = 3;  // 奥义出生容量
	static constexpr std::size_t UltraNameMaxCodepoints = 20; // 奥义名字上限，按码点计
	static constexpr unsigned RainbowFrames = 16u; // 彩虹帧数
	static constexpr float RainbowPeriod = 31.0f; // 彩虹色相周期。16×16 上 x+y 最大是 30，除以 31 斜向走完一圈

	// 扫描。
	static constexpr std::size_t MaxAppliedTokens = 10; // 配方上限。一次扫描最多套用的条数
	// 连发。同一批根弹按入队顺序，每隔这么多秒放一颗。
	static constexpr float FireInterval = 0.08f;

	// 测试。正式以后把初始金钱改回 0。
	static constexpr int TestStartCurrency = 10000; // 测试初始金钱
};
