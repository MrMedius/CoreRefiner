#pragma once
#include "Bindable.h"
#include <memory>
#include <vector>
#include <string>

class Graphics;

namespace Bind
{
	class Texture;

	class DynamicTexture : public CloningBindable
	{
	public:
		DynamicTexture(Graphics& gfx, std::vector<std::string> paths, UINT slot = 0u);
		// 记下要显示的帧。frame 是帧号，张数不够时由 Bind 取模。
		void SetFrame(int frame) noexcept { frame_ = frame; }
		void Bind(Graphics& gfx) noxnd override;
		std::unique_ptr<CloningBindable> Clone() const noexcept override;
	private:
		// 当前要绑定的帧号。没有人改它时保持 0。
		int frame_ = 0;
		std::vector<std::shared_ptr<Bind::Texture>> frames;
	};
}
