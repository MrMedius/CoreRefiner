#pragma once
#include <DirectXMath.h>
#include <string>
#include "Transformation.h"
#include "Projection.h"
#include "CameraIndicator.h"

class Graphics;
namespace Rgph
{
	class RenderGraph;
}

class Camera
{
public:
	Camera(Graphics& gfx, std::string name, DirectX::XMFLOAT3 homePos = { 0.0f,0.0f,0.0f }, float homePitch = 0.0f, float homeYaw = 0.0f, bool tethered = false) noexcept;
	void BindToGraphics(Graphics& gfx) const;
	DirectX::XMMATRIX GetMatrix() const noexcept;
	DirectX::XMMATRIX GetProjection() const noexcept;
	void SpawnControlWidgets(Graphics& gfx) noexcept;
	void Reset(Graphics& gfx) noexcept;
	void Rotate(float dx, float dy) noexcept;
	void Translate(DirectX::XMFLOAT3 translation) noexcept;
	const DirectX::XMFLOAT3& GetPos() const noexcept;
	void SetPos(const DirectX::XMFLOAT3& pos) noexcept;
	void SetRot(const DirectX::XMFLOAT3& rot) noexcept;
	const std::string& GetName() const noexcept;
	void LinkTechniques(Rgph::RenderGraph& rg);
	void Submit(size_t channel) const;
private:
	bool tethered;
	std::string name;
	TransInfo transBase;
	Transformation trans;
	static constexpr float travelSpeed = 20.0f;
	static constexpr float rotationSpeed = 0.004f;
	bool enableCameraIndicator = false;
	bool enableFrustumIndicator = false;
	Projection proj;
	CameraIndicator indicator;
};