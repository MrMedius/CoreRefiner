#pragma once

// 攻击留在目标身上的一种标记。计数存在目标身上，伤害和特效留在对应的攻击模块里。
enum class AttackMark : unsigned char
{
	Yin,
	Yang,
	Count
};

// 一种标记还剩多少个。加上个数和消耗分成两次调用。
struct AttackMarkPile
{
	// 把 amount 加进剩余个数。amount 不是正数时计数不动。
	constexpr void Add(int amount) noexcept
	{
		if (amount <= 0)
		{
			return;
		}
		count_ += amount;
	}

	// 判断够不够一轮。perTrigger 是一轮要扣掉的个数。
	// 够就扣掉一轮并返回 1，不够返回 0，计数不动。阈值不是正数时同样不动。
	constexpr int Trigger(int perTrigger) noexcept
	{
		if (perTrigger <= 0 || count_ < perTrigger)
		{
			return 0;
		}
		count_ -= perTrigger;
		return 1;
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
	// mark 是标记种类。amount 是这次加上的个数。只加个数，不消耗。
	// 种类超出范围，或 amount 不是正数时，计数不动。
	constexpr void Add(AttackMark mark, int amount) noexcept
	{
		const int index = static_cast<int>(mark);
		if (index < 0 || index >= static_cast<int>(AttackMark::Count))
		{
			return;
		}
		piles_[index].Add(amount);
	}

	// mark 是标记种类。perTrigger 是一轮要扣掉的个数。
	// 这一格够一轮就扣掉并返回 1，不够返回 0。种类超出范围时返回 0。
	constexpr int Trigger(AttackMark mark, int perTrigger) noexcept
	{
		const int index = static_cast<int>(mark);
		if (index < 0 || index >= static_cast<int>(AttackMark::Count))
		{
			return 0;
		}
		return piles_[index].Trigger(perTrigger);
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