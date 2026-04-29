#pragma once
#include "Bindable.h"
#include "Drawable.h"
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
		void Bind(Graphics& gfx) noxnd override;
		void InitializeParentReference(const Drawable& parent) noexcept override;
		std::unique_ptr<CloningBindable> Clone() const noexcept override;
	private:
		const Drawable* pParent = nullptr;
		std::vector<std::shared_ptr<Bind::Texture>> frames;
	};
}