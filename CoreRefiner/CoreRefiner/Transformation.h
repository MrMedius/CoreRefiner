#pragma once
#include <DirectXMath.h>
#include "XMath.h"
#include "Math.h"

struct TransInfo
{
    DirectX::XMFLOAT3 position{ 0,0,0 };
    DirectX::XMFLOAT3 rotation{ 0,0,0 };
    DirectX::XMFLOAT3 scale{ 1,1,1 };

    inline DirectX::XMFLOAT4X4 GetWorldMatrix()
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
    void SetRotationRad(float roll, float pitch, float yaw) noexcept;
    void SetRotationDegreeToRad(float roll, float pitch, float yaw) noexcept;

    void SetScale(float x, float y, float z) noexcept;
    void SetScale(float size) noexcept;
    void Translate(float x, float y, float z) noexcept;
    void RotateRad(float roll, float pitch, float yaw) noexcept;
    void RotateDegreeToRad(float roll, float pitch, float yaw) noexcept;
    void Scale(float x, float y, float z) noexcept;
    void Scale(float size) noexcept;

    DirectX::XMMATRIX GetTransformXM() const noexcept;
    DirectX::XMFLOAT3 GetPosition() const noexcept;
    DirectX::XMFLOAT3 GetRotation() const noexcept;
    DirectX::XMFLOAT3 GetScale() const noexcept;

private:
    TransInfo info;
};