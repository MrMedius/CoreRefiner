#include "ShadowCameraCbuf.h"
#include "Camera.h"

namespace dx = DirectX;

namespace Bind
{
	ShadowCameraCbuf::ShadowCameraCbuf(Graphics& gfx, UINT slot)
		:
		pVcbuf{ std::make_unique<VertexConstantBuffer<Transform>>(gfx,slot) }
	{}
	void ShadowCameraCbuf::Bind(Graphics& gfx) noxnd
	{
		pVcbuf->Bind(gfx);
	}
	void ShadowCameraCbuf::Update(Graphics& gfx)
	{
		const auto pos = pCamera->GetPos();
		const Transform t{
			dx::XMMatrixTranspose(
				dx::XMMatrixTranslation(-pos.x,-pos.y,-pos.z)
			)
		};
		pVcbuf->Update(gfx, t);
	}
	void ShadowCameraCbuf::SetCamera(const Camera* p) noexcept
	{
		pCamera = p;
	}
}