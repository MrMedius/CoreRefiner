#pragma once

#include <functional>
#include <string>
#include <vector>

namespace Ui
{
	struct UiOption
	{
		std::string label;
	};

	class UiOptionList
	{
	public:
		void SetOptions(std::vector<UiOption> options);
		void AddOptions(std::vector<UiOption> options);
		void AddOption(UiOption option);
		void EraseOptions(std::vector<int> indices);
		void EraseOption(int index);
		void ClearOptions() noexcept;

		[[nodiscard]] const std::vector<UiOption>& GetOptions() const noexcept { return options_; }
		[[nodiscard]] std::size_t Count() const noexcept { return options_.size(); }
		[[nodiscard]] int GetSelectedIndex() const noexcept { return selectedIndex_; }
		[[nodiscard]] std::string GetLabel(const int index) const;
		[[nodiscard]] std::string GetSelectedLabel() const;

		void SetSelectedIndex(int index, bool notify = true) noexcept;

		void SetOnSelectionChanged(std::function<void(int index, const std::string& label)> cb)
		{
			onSelectionChanged_ = std::move(cb);
		}

	private:
		void NormalizeSelection_() noexcept;

		std::vector<UiOption> options_;
		int selectedIndex_ = 0;
		std::function<void(int index, const std::string& label)> onSelectionChanged_;
	};
}
