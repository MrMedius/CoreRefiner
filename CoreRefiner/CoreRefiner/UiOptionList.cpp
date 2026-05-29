#include "UiOptionList.h"

#include <algorithm>

namespace Ui
{
	void UiOptionList::SetOptions(std::vector<UiOption> options)
	{
		options_ = std::move(options);
		NormalizeSelection_();
	}

	void UiOptionList::AddOptions(std::vector<UiOption> options)
	{
		if (options.empty())
			return;
		options_.insert(
			options_.end(),
			std::make_move_iterator(options.begin()),
			std::make_move_iterator(options.end()));
		NormalizeSelection_();
	}

	void UiOptionList::AddOption(const UiOption option)
	{
		options_.push_back(option);
		NormalizeSelection_();
	}

	void UiOptionList::EraseOptions(std::vector<int> indices)
	{
		if (indices.empty() || options_.empty())
			return;
		std::sort(indices.begin(), indices.end());
		indices.erase(std::unique(indices.begin(), indices.end()), indices.end());
		for (auto it = indices.rbegin(); it != indices.rend(); ++it)
		{
			const int idx = *it;
			if (idx < 0 || idx >= static_cast<int>(options_.size()))
				continue;
			options_.erase(options_.begin() + idx);
		}
		NormalizeSelection_();
	}

	void UiOptionList::EraseOption(const int index)
	{
		EraseOptions(std::vector<int>{ index });
	}

	void UiOptionList::ClearOptions() noexcept
	{
		options_.clear();
		selectedIndex_ = -1;
	}

	void UiOptionList::NormalizeSelection_() noexcept
	{
		if (options_.empty())
		{
			selectedIndex_ = -1;
			return;
		}

		if (selectedIndex_ < 0)
			selectedIndex_ = 0;
		else
			selectedIndex_ = std::clamp(selectedIndex_, 0, static_cast<int>(options_.size()) - 1);
	}

	void UiOptionList::SetSelectedIndex(const int index, const bool notify) noexcept
	{
		if (options_.empty())
		{
			selectedIndex_ = -1;
			return;
		}

		const int clamped = std::clamp(index, 0, static_cast<int>(options_.size()) - 1);
		if (selectedIndex_ == clamped)
			return;
		selectedIndex_ = clamped;
		if (notify && onSelectionChanged_)
			onSelectionChanged_(selectedIndex_, options_[static_cast<size_t>(selectedIndex_)].label);
	}

	std::string UiOptionList::GetLabel(const int index) const
	{
		if (index < 0 || index >= static_cast<int>(options_.size()))
			return {};
		return options_[static_cast<size_t>(index)].label;
	}

	std::string UiOptionList::GetSelectedLabel() const
	{
		return GetLabel(selectedIndex_);
	}
}
