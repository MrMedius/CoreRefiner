#pragma once
#include "Drawable.h"
#include <DirectXMath.h>
#include <memory>
#include <string>

// Debug capsule gizmo: outer hemispheres (uniform scale) + cylinder rails (non-uniform).
// Only the collision surface is drawn — no inner half-sphere inside the capsule.
class CapsuleWireframe
{
public:
	CapsuleWireframe(Graphics& gfx, DirectX::XMFLOAT3 color, std::string tag = "");
	// Link all child drawables into the render graph.
	void LinkTechniques(Rgph::RenderGraph& rg);
	// Submit from hemisphere centers and radius (world space).
	void DoSubmit(DirectX::XMFLOAT3 pointA, DirectX::XMFLOAT3 pointB, float radius);

private:
	// Outer hemisphere wire: unit mesh radius 0.5, dome toward local +Y.
	// Uniform scale only — avoids stretching that a shared non-uniform matrix would cause.
	class HemisphereWire : public Drawable
	{
	public:
		HemisphereWire(Graphics& gfx, DirectX::XMFLOAT3 color, std::string tag);
		// Place hemisphere at center; local +Y aligns with outwardAxis (away from other end).
		void DoSubmit(DirectX::XMFLOAT3 center, DirectX::XMFLOAT3 outwardAxis, float radius);
		DirectX::XMMATRIX GetTransformXM() const noexcept override;
	private:
		struct PSColorConstant
		{
			DirectX::XMFLOAT3 color;
			float padding;
		};
		DirectX::XMFLOAT3 center_{};
		DirectX::XMFLOAT3 axisX_{ 1, 0, 0 };
		DirectX::XMFLOAT3 axisY_{ 0, 1, 0 };
		DirectX::XMFLOAT3 axisZ_{ 0, 0, 1 };
		float scale_{ 1.0f };
	};

	// Two end rings + four rails; unit mesh radius 0.5, segment length 1.
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

	std::unique_ptr<HemisphereWire> endA_;
	std::unique_ptr<HemisphereWire> endB_;
	std::unique_ptr<CylinderWire> cylinder_;
};
