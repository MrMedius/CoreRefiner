#pragma once
#include "Bindable.h"
#include "ConstantBuffers.h"

class Camera;

namespace Bind
{
	class ShadowCameraCbuf : public Bindable
	{
	protected:
		struct Transform
		{
			DirectX::XMMATRIX ViewProj;
		};
	public:
		ShadowCameraCbuf(Graphics& gfx, UINT slot = 1u);
		void Bind(Graphics& gfx) noxnd override;
		void Update(Graphics& gfx);
		void SetCamera(const Camera* pCamera) noexcept;
	private:
		std::unique_ptr<VertexConstantBuffer<Transform>> pVcbuf;
		const Camera* pCamera = nullptr;
	};
}