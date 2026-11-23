#pragma once

// 攻击留在目标身上的一种标记。计数存在目标身上，伤害和特效留在对应的攻击模块里。
enum class AttackMark : unsigned char
{
	Yin,
	Yang,
	Count
};

// 一种标记还剩多少个。满 perTrigger 个就消耗掉那么多个，并记一次触发。
struct AttackMarkPile
{
	// amount 是这次加上的个数。perTrigger 是每满多少个消耗一次。
	// 个数或阈值不是正数时不改计数，返回 0。
	// 返回这次触发了几次，余数留在计数里。
	constexpr int Add(int amount, int perTrigger) noexcept
	{
		if (amount <= 0 || perTrigger <= 0)
		{
			return 0;
		}
		count_ += amount;
		const int triggers = count_ / perTrigger;
		count_ %= perTrigger;
		return triggers;
	}

	// 清掉这一格的剩余个数。
	constexpr void Clear() noexcept
	{
		count_ = 0;
	}

	// 还没被消耗的个数。
	[[nodiscard]] constexpr int Get() const noexcept
	{
		return count_;
	}

private:
	// 剩余个数。
	int count_{ 0 };
};

// 一个目标身上的全部攻击标记。每种标记一格，计数为 0 也留着这一格。
struct AttackMarkSet
{
	// mark 是标记种类。amount 是这次加上的个数。perTrigger 是每满多少个消耗一次。
	// 返回这次触发了几次。种类超出范围时不改计数，返回 0。
	constexpr int Add(AttackMark mark, int amount, int perTrigger) noexcept
	{
		const int index = static_cast<int>(mark);
		if (index < 0 || index >= static_cast<int>(AttackMark::Count))
		{
			return 0;
		}
		return piles_[index].Add(amount, perTrigger);
	}

	// 把每一种标记的剩余个数清零。
	constexpr void Clear() noexcept
	{
		for (AttackMarkPile& pile : piles_)
		{
			pile.Clear();
		}
	}

	// 读取一种标记还剩多少个。种类超出范围时返回 0。
	[[nodiscard]] constexpr int Get(AttackMark mark) const noexcept
	{
		const int index = static_cast<int>(mark);
		if (index < 0 || index >= static_cast<int>(AttackMark::Count))
		{
			return 0;
		}
		return piles_[index].Get();
	}

private:
	// 每种标记一格。下标是 AttackMark。
	AttackMarkPile piles_[static_cast<int>(AttackMark::Count)]{};
};