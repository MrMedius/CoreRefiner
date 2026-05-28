#pragma once
#include "UiInputFrame.h"

namespace Ui
{
	namespace Input
	{
		// check the pointer coordinates within the valid logic surface
		[[nodiscard]] inline bool ClientPointValid(const UiPointerPayload& p) noexcept
		{
			return p.insideLogicalSurface;
		}
	}

	// Single-area pointer press tracking: Pressed visual and "release within range triggers click" semantics.
	// Pressed visual is tracking && primaryDown, unrelated to the current Hover.
	class UiPointerPressTracker
	{
	public:
		void Reset() noexcept
		{
			tracking_ = false;
			primaryDownLastFrame_ = false;
		}

		//Tracking begins when a pressed edge is detected within the control's hit area.
		//primaryDown: indicates whether the primary key is pressed in this frame.
		//isOver: indicates whether the pointer falls within the control's hit area.
		void TryBeginPress(const bool primaryDown, const bool isOver) noexcept
		{
			if (primaryDown && !primaryDownLastFrame_ && isOver)
				tracking_ = true;
		}

		// The primary key release ends a press tracking session; if tracking was in progress, it returns true.
		// primaryDown: checks if the primary key is still pressed in the current frame.
		[[nodiscard]] bool EndPress(const bool primaryDown) noexcept
		{
			if (primaryDown || !tracking_)
				return false;

			tracking_ = false;
			return true;
		}

		// End tracking when the primary key is released; returns true if released within the hit zone.
		// primaryDown: Whether the primary key is still pressed in this frame.
		// releaseInside : Whether the pointer is within the hit zone when released.
		[[nodiscard]] bool TryCompletePress(const bool primaryDown, const bool releaseInside) noexcept
		{
			if (!EndPress(primaryDown))
				return false;

			return releaseInside;
		}

		// Tracking is canceled when the primary key is released (no click semantics, such as when a Slider drag ends).
		void CancelIfReleased(const bool primaryDown) noexcept
		{
			if (!primaryDown)
				tracking_ = false;
		}

		// Check the Pressed visual be displayed
		[[nodiscard]] bool ShouldShowPressed(const bool primaryDown) const noexcept
		{
			return tracking_ && primaryDown;
		}

		// Check the press that started from this control
		[[nodiscard]] bool IsTracking() const noexcept { return tracking_; }

		// Check the device remain in tracking mode while the main button is pressed (e.g., Slider dragging)
		[[nodiscard]] bool IsHoldActive(const bool primaryDown) const noexcept
		{
			return tracking_ && primaryDown;
		}

		// Synchronization at the end of each frame is used for edge detection in the next frame.
		void SyncFrame(const bool primaryDown) noexcept
		{
			primaryDownLastFrame_ = primaryDown;
		}

		[[nodiscard]] bool PrimaryDownLastFrame() const noexcept { return primaryDownLastFrame_; }

	private:
		bool tracking_ = false;
		bool primaryDownLastFrame_ = false;
	};

	// Indexed pointer press tracking (Dropdown list items, etc.)
	class UiIndexedPointerPressTracker
	{
	public:
		void Reset() noexcept
		{
			pressedIndex_ = -1;
			primaryDownLastFrame_ = false;
		}

		// Record the index when a down - edge is detected within the hit area of ​​a list item.
		// index: Hit item index, must be >= 0.
		void TryBeginPress(const bool primaryDown, const int index) noexcept
		{
			if (primaryDown && !primaryDownLastFrame_ && index >= 0)
				pressedIndex_ = index;
		}

		// Check the Pressed visual be displayed at the specified index
		[[nodiscard]] bool ShouldShowPressed(const int index, const bool primaryDown) const noexcept
		{
			return pressedIndex_ == index && primaryDown;
		}

		// The tracing ends when the primary key is released; if the release occurs within a range on the same index, it returns true and writes the index.
		// releaseIndex: is the index obtained by hit - test at the time of release.
		// releaseInside: indicates whether the release still occurs within a valid surface.
		[[nodiscard]] bool TryCompletePress(
			const bool primaryDown,
			const int releaseIndex,
			const bool releaseInside,
			int& completedIndex) noexcept
		{
			if (primaryDown || pressedIndex_ < 0)
				return false;

			const int startedIndex = pressedIndex_;
			pressedIndex_ = -1;

			if (releaseInside && releaseIndex >= 0 && releaseIndex == startedIndex)
			{
				completedIndex = startedIndex;
				return true;
			}


		return false;
		}

		void SyncFrame(const bool primaryDown) noexcept
		{
			primaryDownLastFrame_ = primaryDown;
		}

		[[nodiscard]] bool PrimaryDownLastFrame() const noexcept { return primaryDownLastFrame_; }

		[[nodiscard]] int PressedIndex() const noexcept { return pressedIndex_; }

	private:
		int pressedIndex_ = -1;
		bool primaryDownLastFrame_ = false;
	};
}