#include "DynamicTexture.h"
#include "Texture.h"
#include <cassert>

namespace Bind
{
	DynamicTexture::DynamicTexture(Graphics& gfx, std::vector<std::string> paths, UINT slot)
	{
		frames.reserve(paths.size());
		for (auto& p : paths)
		{
			frames.push_back(Bind::Texture::Resolve(gfx, p, slot));
		}
		assert(!frames.empty());
	}

	std::unique_ptr<CloningBindable> DynamicTexture::Clone() const noexcept
	{
		return std::make_unique<DynamicTexture>(*this);
	}

	void DynamicTexture::Bind(Graphics& gfx) noxnd
	{
		const int n = (int)frames.size();
		if (n <= 0) return;

		// idx 是落在现有帧里的下标。frame_ 由拥有者写入，负数先绕回正数。
		int idx = frame_ % n;
		if (idx < 0) idx += n;

		frames[(size_t)idx]->Bind(gfx);
	}
}
