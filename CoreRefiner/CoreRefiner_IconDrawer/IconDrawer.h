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
	// 原点往左的格数。左是负方向，这个数本身不为负。原点固定在 (0,0)。
	[[nodiscard]] int Left() const noexcept { return left_; }
	// 原点往右的格数。右是正方向。宽等于左加右。
	[[nodiscard]] int Right() const noexcept { return width_ - left_; }
	// 原点往上的格数。上是负方向，这个数本身不为负。
	[[nodiscard]] int Up() const noexcept { return up_; }
	// 原点往下的格数。下是正方向。高等于上加下。
	[[nodiscard]] int Down() const noexcept { return height_ - up_; }

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
	// 左、上的插入数加进负方向的格数，右、下加进正方向。原点仍是 (0,0)。
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
		left_ += left;
		up_ += up;
		cells_ = std::move(next);
		return true;
	}

	// 这组方向格数能否替换当前图。四个数都要大于等于 0。
	// 左加右、上加下要在 1 到 kMaxSide 之间。与现在相同则返回 false。
	[[nodiscard]] bool CanSetSides(int left, int right, int up, int down) const noexcept
	{
		if (left < 0 || right < 0 || up < 0 || down < 0)
		{
			return false;
		}
		const long long newW = static_cast<long long>(left) + right;
		const long long newH = static_cast<long long>(up) + down;
		if (newW < 1 || newH < 1 || newW > kMaxSide || newH > kMaxSide)
		{
			return false;
		}
		return left != left_ || right != Right() || up != up_ || down != Down();
	}

	// 按四个方向的格数重排。原点保持 (0,0)。
	// 同一坐标上的旧格子留下，新伸出的写成灭，缩掉的丢掉。
	// 条件与 CanSetSides 相同。失败时整张图不动。
	[[nodiscard]] bool SetSides(int left, int right, int up, int down)
	{
		if (!CanSetSides(left, right, up, down))
		{
			return false;
		}
		const int width = left + right;
		const int height = up + down;
		std::vector<std::uint8_t> next(static_cast<std::size_t>(width) * static_cast<std::size_t>(height), 0);
		const int oldRight = Right();
		const int oldDown = Down();
		const int x0 = -left_ > -left ? -left_ : -left;
		const int x1 = oldRight < right ? oldRight : right;
		const int y0 = -up_ > -up ? -up_ : -up;
		const int y1 = oldDown < down ? oldDown : down;
		for (int y = y0; y < y1; ++y)
		{
			for (int x = x0; x < x1; ++x)
			{
				const std::size_t dst = static_cast<std::size_t>(y + up) * static_cast<std::size_t>(width)
					+ static_cast<std::size_t>(x + left);
				const std::size_t src = static_cast<std::size_t>(y + up_) * static_cast<std::size_t>(width_)
					+ static_cast<std::size_t>(x + left_);
				next[dst] = cells_[src];
			}
		}
		left_ = left;
		up_ = up;
		width_ = width;
		height_ = height;
		cells_ = std::move(next);
		return true;
	}

	// 按 ratioW:ratioH 能否补齐。已经符合，或补完会超过 kMaxSide，则返回 false。
	[[nodiscard]] bool CanFitAspect(int ratioW, int ratioH) const noexcept
	{
		int newW = 0;
		int newH = 0;
		return MeasureFit_(ratioW, ratioH, newW, newH);
	}

	// 按 ratioW:ratioH 只补较短的一边，多出的空格平分到两侧，原图留在中间。
	// 奇数多出来的一格给右或下。已经符合，或补完会超过 kMaxSide，则不动并返回 false。
	[[nodiscard]] bool FitAspect(int ratioW, int ratioH)
	{
		int newW = 0;
		int newH = 0;
		if (!MeasureFit_(ratioW, ratioH, newW, newH))
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

	// 把当前宽高、四个方向的格数和各位压进撤销栈，并丢掉重做记录。超过 kMaxUndo 时丢掉最旧的一份。
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

	// 撤销栈里是否还有旧图。空栈时界面不应改画布。
	[[nodiscard]] bool CanUndo() const noexcept
	{
		return !undo_.empty();
	}

	// 重做栈里是否还有可回到的图。空栈时界面不应改画布。
	[[nodiscard]] bool CanRedo() const noexcept
	{
		return !redo_.empty();
	}

	// 核对初始方向格数、位、扩展、裁剪、16:9 和撤销。失败返回 false。
	[[nodiscard]] static bool SelfCheck()
	{
		IconSheet sheet;
		if (sheet.Width() != 16 || sheet.Height() != 16 || sheet.Get(0, 0))
		{
			return false;
		}
		if (sheet.Left() != 0 || sheet.Up() != 0 || sheet.Right() != 16 || sheet.Down() != 16)
		{
			return false;
		}
		sheet.Set(0, 0, true);
		if (!sheet.Get(0, 0) || sheet.Get(-1, 0))
		{
			return false;
		}

		sheet.PushUndo();
		if (!sheet.Expand(2, 0, 0, 0) || sheet.Width() != 18 || sheet.Left() != 2 || sheet.Right() != 16)
		{
			return false;
		}
		if (!sheet.Get(2, 0) || sheet.Get(0, 0))
		{
			return false;
		}
		if (!sheet.Undo() || sheet.Width() != 16 || sheet.Left() != 0 || !sheet.Get(0, 0))
		{
			return false;
		}
		if (!sheet.Redo() || sheet.Width() != 18 || sheet.Left() != 2 || !sheet.Get(2, 0))
		{
			return false;
		}

		IconSheet framed;
		framed.Set(0, 0, true);
		framed.PushUndo();
		if (!framed.SetSides(2, 16, 0, 16) || framed.Width() != 18 || framed.Left() != 2 || framed.Right() != 16 || !framed.Get(2, 0))
		{
			return false;
		}
		if (!framed.Undo() || framed.Left() != 0 || framed.Right() != 16 || !framed.Get(0, 0))
		{
			return false;
		}
		if (framed.SetSides(0, 16, 0, 16))
		{
			return false;
		}
		if (!framed.SetSides(0, 10, 0, 16) || framed.Width() != 10 || framed.Right() != 10 || !framed.Get(0, 0))
		{
			return false;
		}
		if (framed.SetSides(-1, 16, 0, 16) || framed.SetSides(0, 0, 0, 16) || framed.SetSides(0, 1025, 0, 16) || framed.Width() != 10)
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
		if (wide.Left() != 6 || wide.Right() != 22 || wide.Up() != 0 || wide.Down() != 16)
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
	// 一份撤销记录。width、height 是当时的宽高。left、up 是当时原点往左、往上的格数。cells 是当时的各位。
	struct Snapshot
	{
		int width{ 0 };
		int height{ 0 };
		int left{ 0 };
		int up{ 0 };
		std::vector<std::uint8_t> cells;
	};

	// 算出补齐 ratioW:ratioH 后的宽高。newW、newH 写出结果。
	// 比例不合法、已经符合，或补完会超过 kMaxSide，则返回 false。
	[[nodiscard]] bool MeasureFit_(int ratioW, int ratioH, int& newW, int& newH) const noexcept
	{
		if (ratioW <= 0 || ratioH <= 0 || width_ <= 0 || height_ <= 0)
		{
			return false;
		}
		const double target = static_cast<double>(ratioW) / static_cast<double>(ratioH);
		const double current = static_cast<double>(width_) / static_cast<double>(height_);
		newW = width_;
		newH = height_;
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
		return newW <= kMaxSide && newH <= kMaxSide;
	}

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
		return Snapshot{ width_, height_, left_, up_, cells_ };
	}

	// 用一份记录换掉当前图。
	void Restore_(const Snapshot& shot)
	{
		width_ = shot.width;
		height_ = shot.height;
		left_ = shot.left;
		up_ = shot.up;
		cells_ = shot.cells;
	}

	// 宽，单位是格。等于左加右。
	int width_{ 16 };
	// 高，单位是格。等于上加下。
	int height_{ 16 };
	// 原点往左的格数。原点固定在 (0,0)，落在缓冲区的这一列。
	int left_{ 0 };
	// 原点往上的格数。原点落在缓冲区的这一行。
	int up_{ 0 };
	// 行优先的各位。0 是灭，1 是亮。
	std::vector<std::uint8_t> cells_;
	// 改图前压入的旧图。最旧的在前面。
	std::vector<Snapshot> undo_;
	// 撤销时放下的图，供重做。新的 PushUndo 会清空它。
	std::vector<Snapshot> redo_;
};
