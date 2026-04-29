#include "Transformation.h"
#include "Math.h"

namespace dx = DirectX;

void Transformation::SetPosition(float x, float y, float z) noexcept
{
    info.position = { x,y,z };
}

void Transformation::SetRotationRad(float roll, float pitch, float yaw) noexcept
{
    info.rotation = { roll, pitch, yaw };
}

void Transformation::SetRotationDegreeToRad(float roll, float pitch, float yaw) noexcept
{
    info.rotation = { to_rad(roll), to_rad(pitch), to_rad(yaw) };
}

void Transformation::SetScale(float x, float y, float z) noexcept
{
    info.scale = { x,y,z };
}

void Transformation::SetScale(float size) noexcept
{
    SetScale(size, size, size);
}

void Transformation::Translate(float x, float y, float z) noexcept
{
     info.position.x += x;
     info.position.y += y;
     info.position.z += z;
}

void Transformation::RotateRad(float roll, float pitch, float yaw) noexcept
{
     info.rotation.x += roll;
     info.rotation.y += pitch;
     info.rotation.z += yaw;
}

void Transformation::RotateDegreeToRad(float roll, float pitch, float yaw) noexcept
{
     info.rotation.x += to_rad(roll);
     info.rotation.y += to_rad(pitch);
     info.rotation.z += to_rad(yaw);
}

void Transformation::Scale(float x, float y, float z) noexcept
{
     info.scale.x *= x;
     info.scale.y *= y;
     info.scale.z *= z;
}

void Transformation::Scale(float size) noexcept
{
    Scale(size, size, size);
}

dx::XMMATRIX Transformation::GetTransformXM() const noexcept
{
	auto& [position, rotation, scale] = info;

    return
        dx::XMMatrixScaling(scale.x, scale.y, scale.z) *
        dx::XMMatrixRotationRollPitchYaw(rotation.x, rotation.y, rotation.z) *
        dx::XMMatrixTranslation(position.x, position.y, position.z);
}

dx::XMFLOAT3 Transformation::GetPosition() const noexcept
{
    return info.position;
}
dx::XMFLOAT3 Transformation::GetRotation() const noexcept
{
    return
    {
        wrap_angle(info.rotation.x),
        wrap_angle(info.rotation.y),
        wrap_angle(info.rotation.z)
    };
}
dx::XMFLOAT3 Transformation::GetScale() const noexcept
{
    return info.scale;
}