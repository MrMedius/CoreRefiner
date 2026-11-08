#pragma once

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

// 一位图。1 是亮，0 是灭。改图前由调用方 PushUndo，撤销和重做在栈里移动。
class IconSheet
{
public:
	// 宽和高各自的上限。
	static constexpr int kMaxSide = 1024;
	// 撤销栈最多保留的份数。
	static constexpr std::size_t kMaxUndo = 64;

	// 新建一张 16×16、全灭的图。
	IconSheet()
		:
		width_(16),
		height_(16),
		cells_(16 * 16, 0)
	{}

	// 当前宽度，单位是格。
	[[nodiscard]] int Width() const noexcept { return width_; }
	// 当前高度，单位是格。
	[[nodiscard]] int Height() const noexcept { return height_; }

	// 读取一格。越界当灭。
	// x 从左往右，y 从上往下。
	[[nodiscard]] bool Get(int x, int y) const noexcept
	{
		if (!Contains_(x, y))
		{
			return false;
		}
		return cells_[Index_(x, y)] != 0;
	}

	// 写入一格。越界则不动。on 为真写成亮。
	void Set(int x, int y, bool on) noexcept
	{
		if (!Contains_(x, y))
		{
			return;
		}
		cells_[Index_(x, y)] = on ? 1 : 0;
	}

	// 在四边插入空行或空列。负数当 0。
	// 结果任一边超过 kMaxSide，或四边都是 0，则整张图不动并返回 false。
	[[nodiscard]] bool Expand(int left, int right, int up, int down)
	{
		if (left < 0) { left = 0; }
		if (right < 0) { right = 0; }
		if (up < 0) { up = 0; }
		if (down < 0) { down = 0; }
		if (left == 0 && right == 0 && up == 0 && down == 0)
		{
			return false;
		}

		const long long newW = static_cast<long long>(width_) + left + right;
		const long long newH = static_cast<long long>(height_) + up + down;
		if (newW < 1 || newH < 1 || newW > kMaxSide || newH > kMaxSide)
		{
			return false;
		}

		const int width = static_cast<int>(newW);
		const int height = static_cast<int>(newH);
		std::vector<std::uint8_t> next(static_cast<std::size_t>(width) * static_cast<std::size_t>(height), 0);
		for (int y = 0; y < height_; ++y)
		{
			for (int x = 0; x < width_; ++x)
			{
				const std::size_t dst = static_cast<std::size_t>(y + up) * static_cast<std::size_t>(width)
					+ static_cast<std::size_t>(x + left);
				next[dst] = cells_[Index_(x, y)];
			}
		}
		width_ = width;
		height_ = height;
		cells_ = std::move(next);
		return true;
	}

	// 按 ratioW:ratioH 只补较短的一边，多出的空格平分到两侧，原图留在中间。
	// 奇数多出来的一格给右或下。已经符合，或补完会超过 kMaxSide，则不动并返回 false。
	[[nodiscard]] bool FitAspect(int ratioW, int ratioH)
	{
		if (ratioW <= 0 || ratioH <= 0 || width_ <= 0 || height_ <= 0)
		{
			return false;
		}

		const double target = static_cast<double>(ratioW) / static_cast<double>(ratioH);
		const double current = static_cast<double>(width_) / static_cast<double>(height_);
		int newW = width_;
		int newH = height_;
		if (current < target)
		{
			newW = static_cast<int>(std::lround(static_cast<double>(height_) * target));
			if (newW < width_)
			{
				newW = width_;
			}
		}
		else if (current > target)
		{
			newH = static_cast<int>(std::lround(static_cast<double>(width_) / target));
			if (newH < height_)
			{
				newH = height_;
			}
		}

		if (newW == width_ && newH == height_)
		{
			return false;
		}
		if (newW > kMaxSide || newH > kMaxSide)
		{
			return false;
		}

		const int addW = newW - width_;
		const int addH = newH - height_;
		return Expand(addW / 2, addW - addW / 2, addH / 2, addH - addH / 2);
	}

	// 全部写成灭。宽高不变。
	void Clear() noexcept
	{
		for (std::uint8_t& cell : cells_)
		{
			cell = 0;
		}
	}

	// 把当前宽高和各位压进撤销栈，并丢掉重做记录。超过 kMaxUndo 时丢掉最旧的一份。
	void PushUndo()
	{
		redo_.clear();
		undo_.push_back(Capture_());
		if (undo_.size() > kMaxUndo)
		{
			undo_.erase(undo_.begin());
		}
	}

	// 回到上一次压栈时的图。没有记录则不动并返回 false。
	[[nodiscard]] bool Undo()
	{
		if (undo_.empty())
		{
			return false;
		}
		redo_.push_back(Capture_());
		if (redo_.size() > kMaxUndo)
		{
			redo_.erase(redo_.begin());
		}
		Restore_(undo_.back());
		undo_.pop_back();
		return true;
	}

	// 重新走到撤销前的图。没有记录则不动并返回 false。
	[[nodiscard]] bool Redo()
	{
		if (redo_.empty())
		{
			return false;
		}
		undo_.push_back(Capture_());
		if (undo_.size() > kMaxUndo)
		{
			undo_.erase(undo_.begin());
		}
		Restore_(redo_.back());
		redo_.pop_back();
		return true;
	}

	// 核对初始尺寸、位、扩展、16:9 和撤销。失败返回 false。
	[[nodiscard]] static bool SelfCheck()
	{
		IconSheet sheet;
		if (sheet.Width() != 16 || sheet.Height() != 16 || sheet.Get(0, 0))
		{
			return false;
		}
		sheet.Set(0, 0, true);
		if (!sheet.Get(0, 0) || sheet.Get(-1, 0))
		{
			return false;
		}

		sheet.PushUndo();
		if (!sheet.Expand(2, 0, 0, 0) || sheet.Width() != 18 || !sheet.Get(2, 0) || sheet.Get(0, 0))
		{
			return false;
		}
		if (!sheet.Undo() || sheet.Width() != 16 || !sheet.Get(0, 0))
		{
			return false;
		}
		if (!sheet.Redo() || sheet.Width() != 18 || !sheet.Get(2, 0))
		{
			return false;
		}

		IconSheet wide;
		if (wide.FitAspect(1, 1))
		{
			return false;
		}
		wide.Set(0, 0, true);
		if (!wide.FitAspect(16, 9) || wide.Width() != 28 || wide.Height() != 16 || !wide.Get(6, 0))
		{
			return false;
		}

		IconSheet full;
		full.width_ = kMaxSide;
		full.height_ = 1;
		full.cells_.assign(static_cast<std::size_t>(kMaxSide), 0);
		if (full.Expand(0, 1, 0, 0))
		{
			return false;
		}
		return full.Width() == kMaxSide;
	}

private:
	// 一份撤销记录。width、height 是当时的宽高，cells 是当时的各位。
	struct Snapshot
	{
		int width{ 0 };
		int height{ 0 };
		std::vector<std::uint8_t> cells;
	};

	// 格子下标。调用前须确认在图内。
	[[nodiscard]] std::size_t Index_(int x, int y) const noexcept
	{
		return static_cast<std::size_t>(y) * static_cast<std::size_t>(width_) + static_cast<std::size_t>(x);
	}

	// 这一格是否在图内。
	[[nodiscard]] bool Contains_(int x, int y) const noexcept
	{
		return x >= 0 && y >= 0 && x < width_ && y < height_;
	}

	// 抄下当前图。
	[[nodiscard]] Snapshot Capture_() const
	{
		return Snapshot{ width_, height_, cells_ };
	}

	// 用一份记录换掉当前图。
	void Restore_(const Snapshot& shot)
	{
		width_ = shot.width;
		height_ = shot.height;
		cells_ = shot.cells;
	}

	// 宽，单位是格。
	int width_{ 16 };
	// 高，单位是格。
	int height_{ 16 };
	// 行优先的各位。0 是灭，1 是亮。
	std::vector<std::uint8_t> cells_;
	// 改图前压入的旧图。最旧的在前面。
	std::vector<Snapshot> undo_;
	// 撤销时放下的图，供重做。新的 PushUndo 会清空它。
	std::vector<Snapshot> redo_;
};
