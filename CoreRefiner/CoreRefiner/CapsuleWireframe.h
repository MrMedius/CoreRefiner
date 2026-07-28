#pragma once
#include "Drawable.h"
#include "SphereWireframe.h"
#include <DirectXMath.h>
#include <memory>
#include <string>

/**
 * @brief Debug capsule gizmo: sphere ends (uniform scale) + cylinder rails (non-uniform).
 * @note Ends cannot share one non-uniform matrix with the cylinder or hemispheres stretch.
 */
class CapsuleWireframe
{
public:
	CapsuleWireframe(Graphics& gfx, DirectX::XMFLOAT3 color, std::string tag = "");
	/** @brief Link all child drawables into the render graph. */
	void LinkTechniques(Rgph::RenderGraph& rg);
	/** @brief Submit from hemisphere centers and radius (world space). */
	void DoSubmit(DirectX::XMFLOAT3 pointA, DirectX::XMFLOAT3 pointB, float radius);

private:
	/** @brief Two end rings + four rails; unit mesh radius 0.5, segment length 1. */
	class CylinderWire : public Drawable
	{
	public:
		CylinderWire(Graphics& gfx, DirectX::XMFLOAT3 color, std::string tag);
		void DoSubmit(DirectX::XMFLOAT3 pointA, DirectX::XMFLOAT3 pointB, float radius);
		DirectX::XMMATRIX GetTransformXM() const noexcept override;
	private:
		struct PSColorConstant
		{
			DirectX::XMFLOAT3 color;
			float padding;
		};
		DirectX::XMFLOAT3 mid_{};
		DirectX::XMFLOAT3 axisX_{ 1, 0, 0 };
		DirectX::XMFLOAT3 axisY_{ 0, 1, 0 };
		DirectX::XMFLOAT3 axisZ_{ 0, 0, 1 };
		float scaleXZ_{ 1.0f };
		float scaleY_{ 1.0f };
	};

	SphereWireframe endA_;
	SphereWireframe endB_;
	std::unique_ptr<CylinderWire> cylinder_;
};
