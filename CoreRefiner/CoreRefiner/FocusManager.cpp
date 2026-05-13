#include "FocusManager.h"
#include <algorithm>

namespace Ui
{
	void FocusManager::SetTabOrder(std::vector<FocusHandle> order)
	{
		tabOrder_.clear();
		for (FocusHandle h : order)
		{
			if (h == kInvalidFocusHandle)
				continue;
			if (std::find(tabOrder_.begin(), tabOrder_.end(), h) == tabOrder_.end())
				tabOrder_.push_back(h);
		}

		if (focused_ != kInvalidFocusHandle &&
			std::find(tabOrder_.begin(), tabOrder_.end(), focused_) == tabOrder_.end())
		{
			focused_ = kInvalidFocusHandle;
		}
	}

	void FocusManager::RegisterTabStop(FocusHandle h)
	{
		if (h == kInvalidFocusHandle)
			return;
		if (std::find(tabOrder_.begin(), tabOrder_.end(), h) != tabOrder_.end())
			return;
		tabOrder_.push_back(h);
	}

	void FocusManager::UnregisterTabStop(FocusHandle h)
	{
		//tabOrder_.erase(std::remove(tabOrder_.begin(), tabOrder_.end(), h), tabOrder_.end());
		std::erase(tabOrder_, h);
		if (focused_ == h)
			focused_ = kInvalidFocusHandle;
	}

	void FocusManager::ClearTabOrder() noexcept
	{
		tabOrder_.clear();
		focused_ = kInvalidFocusHandle;
	}

	void FocusManager::ClearFocus() noexcept
	{
		focused_ = kInvalidFocusHandle;
	}

	void FocusManager::RequestFocus(FocusHandle h) noexcept
	{
		if (h == kInvalidFocusHandle)
		{
			focused_ = kInvalidFocusHandle;
			return;
		}
		focused_ = h;
	}

	std::ptrdiff_t FocusManager::FindIndex(FocusHandle h) const noexcept
	{
		const auto it = std::find(tabOrder_.begin(), tabOrder_.end(), h);
		if (it == tabOrder_.end())
			return -1;
		return static_cast<std::ptrdiff_t>(it - tabOrder_.begin());
	}

	void FocusManager::FocusAtIndex(std::size_t index) noexcept
	{
		if (tabOrder_.empty())
			return;
		index %= tabOrder_.size();
		focused_ = tabOrder_[index];
	}

	void FocusManager::FocusNext() noexcept
	{
		if (tabOrder_.empty())
			return;

		const std::ptrdiff_t idx = FindIndex(focused_);
		if (idx < 0)
		{
			FocusAtIndex(0u);
			return;
		}
		const std::size_t next = static_cast<std::size_t>(idx) + 1u;
		FocusAtIndex(next);
	}

	void FocusManager::FocusPrev() noexcept
	{
		if (tabOrder_.empty())
			return;

		const std::ptrdiff_t idx = FindIndex(focused_);
		if (idx < 0)
		{
			FocusAtIndex(tabOrder_.size() - 1u);
			return;
		}
		const std::size_t cur = static_cast<std::size_t>(idx);
		const std::size_t prev = (cur == 0u) ? (tabOrder_.size() - 1u) : (cur - 1u);
		FocusAtIndex(prev);
	}

	void FocusManager::ApplyNavigation(const UiInputFrame& frame)
	{
		if (frame.navigation.tabNext)
			FocusNext();
		if (frame.navigation.tabPrev)
			FocusPrev();
	}
}