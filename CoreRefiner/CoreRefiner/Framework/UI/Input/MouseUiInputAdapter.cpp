#include "MouseUiInputAdapter.h"

#include "InputCodex.h"
#include "imgui/imgui.h"

namespace Ui
{
	UiInputFrame MouseUiInputAdapter::BuildFrame(bool respectImGuiCapture) const
	{
		UiInputFrame frame{};

		if (respectImGuiCapture && ImGui::GetCurrentContext() != nullptr && ImGui::GetIO().WantCaptureMouse)
			return frame;

		const InputCodex& in = InputCodex::Get();

		frame.pointer.logicalX = static_cast<float>(in.MouseX());
		frame.pointer.logicalY = static_cast<float>(in.MouseY());
		frame.pointer.insideLogicalSurface = in.MouseInWindow();
		frame.pointer.primaryDown = in.MouseLeftPressed();
		frame.pointer.primaryPressed = in.MouseLeftTriggered();
		frame.pointer.primaryReleased = in.MouseLeftReleased();

		return frame;
	}
}