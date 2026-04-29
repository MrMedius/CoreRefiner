#include "Mesh.h"

namespace dx = DirectX;

Mesh::Mesh(Graphics& gfx, const Material& mat, const aiMesh& mesh, float scale) noxnd
	:
	Drawable(gfx, mat, mesh, scale)
{}

void Mesh::Submit(size_t channels, dx::FXMMATRIX accumulatedTranform) const noxnd
{
	dx::XMStoreFloat4x4(&transform, accumulatedTranform);
	Drawable::Submit(channels);
}
dx::XMMATRIX Mesh::GetTransformXM() const noexcept
{
	return dx::XMLoadFloat4x4(&transform);
}