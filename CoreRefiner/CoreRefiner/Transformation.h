#pragma once
#include <DirectXMath.h>
#include "XMath.h"
#include "Math.h"

struct TransInfo
{
	DirectX::XMFLOAT3 position{ 0,0,0 };
	DirectX::XMFLOAT3 rotation{ 0,0,0 };
	DirectX::XMFLOAT3 scale{ 1,1,1 };

	// Build S*R*T matrix from stored TRS (local space when used under a parent).
	// `rotation` is interpreted as radians by XMMatrixRotationRollPitchYaw.
	// Name kept for callers; hierarchical world is ObjectBase::GetWorldMatrix().
	inline DirectX::XMFLOAT4X4 GetWorldMatrix() const
	{
		using namespace DirectX;

		const XMMATRIX S = XMMatrixScaling(scale.x, scale.y, scale.z);
		const XMMATRIX R = XMMatrixRotationRollPitchYaw(rotation.x, rotation.y, rotation.z);
		const XMMATRIX T = XMMatrixTranslation(position.x, position.y, position.z);

		XMFLOAT4X4 out{};
		XMStoreFloat4x4(&out, S * R * T);
		return out;
	}
};

class Transformation
{
public:
	Transformation() noexcept = default;

	void SetPosition(float x, float y, float z) noexcept;
	void SetPosition(DirectX::XMFLOAT3 position) noexcept;

	void SetRotationRad(float roll, float pitch, float yaw) noexcept;
	void SetRotationDegreeToRad(float roll, float pitch, float yaw) noexcept;
	// Store rotation components as-is (no unit conversion).
	void SetRotationRaw(float roll, float pitch, float yaw) noexcept;
	void SetRotationRaw(DirectX::XMFLOAT3 rotation) noexcept;

	void SetScale(float x, float y, float z) noexcept;
	void SetScale(float size) noexcept;
	void SetScale(DirectX::XMFLOAT3 scale) noexcept;

	void Translate(float x, float y, float z) noexcept;
	void RotateRad(float roll, float pitch, float yaw) noexcept;
	void RotateDegreeToRad(float roll, float pitch, float yaw) noexcept;
	// Add to rotation components as-is (no unit conversion).
	void RotateRaw(float roll, float pitch, float yaw) noexcept;

	// Multiply scale (render-oriented).
	void Scale(float x, float y, float z) noexcept;
	void Scale(float size) noexcept;
	// Add to scale components (matches legacy ObjectBase::Scale).
	void AddScale(float x, float y, float z) noexcept;

	DirectX::XMMATRIX GetTransformXM() const noexcept;
	// Local S*R*T matrix (same as GetTransformXM; name clarifies hierarchy use).
	DirectX::XMMATRIX GetLocalMatrix() const noexcept;
	DirectX::XMFLOAT3 GetPosition() const noexcept;
	// Wrapped radian-oriented read (render helpers).
	DirectX::XMFLOAT3 GetRotation() const noexcept;
	// Raw stored rotation (no wrap / no unit assumption).
	DirectX::XMFLOAT3 GetRotationRaw() const noexcept;
	DirectX::XMFLOAT3 GetScale() const noexcept;

	TransInfo& GetInfo() noexcept;
	const TransInfo& GetInfo() const noexcept;
	void SetInfo(const TransInfo& info) noexcept;

private:
	TransInfo info;
};
