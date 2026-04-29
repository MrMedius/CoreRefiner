#include "Camera.h"
#include "imgui/imgui.h"
#include "Math.h"

namespace dx = DirectX;

Camera::Camera(Graphics& gfx, std::string name, DirectX::XMFLOAT3 homePos, float homePitch, float homeYaw, bool tethered) noexcept 
	:
	name(std::move(name)),
	proj(gfx, 1.0f, 9.0f / 16.0f, 0.5f, 400.0f), 
	indicator(gfx),
	tethered(tethered)
{
	transBase.position = { homePos.x, homePos.y, homePos.z };
	transBase.rotation = { homePitch, homeYaw, 0.0f };

	if (tethered)
	{
		trans.SetPosition(homePos.x, homePos.y, homePos.z);
		trans.SetRotationRad(homePitch, homeYaw, 0.0f);
		indicator.SetPos(trans.GetPosition());
		proj.SetPos(trans.GetPosition());
	}
	Reset(gfx);
}

void Camera::BindToGraphics(Graphics& gfx) const
{
	gfx.SetCamera(GetMatrix());
	gfx.SetProjection(proj.GetMatrix());
}

DirectX::XMMATRIX Camera::GetMatrix() const noexcept
{
	return dx::XMMatrixInverse(nullptr, trans.GetTransformXM());
}

DirectX::XMMATRIX Camera::GetProjection() const noexcept
{
	return proj.GetMatrix();
}

void Camera::SpawnControlWidgets(Graphics& gfx) noexcept
{
	auto pos = trans.GetPosition();
	auto rot = trans.GetRotation();

	if (!tethered)
	{
		ImGui::Text("Position");
		ImGui::SliderFloat("X", &pos.x, -80.0f, 80.0f);
		ImGui::SliderFloat("Y", &pos.y, -80.0f, 80.0f);
		ImGui::SliderFloat("Z", &pos.z, -80.0f, 80.0f);
	}
	ImGui::Text("Orientation");
	ImGui::SliderAngle("Pitch", &rot.x, -89.0f, 89.0f);
	ImGui::SliderAngle("Yaw", &rot.y, -180.0f, 180.0f);

	proj.RenderWidgets(gfx);
	ImGui::Checkbox("Camera Indicator", &enableCameraIndicator);
	ImGui::Checkbox("Frustum Indicator", &enableFrustumIndicator);

	if (ImGui::Button("Reset")) Reset(gfx);

	trans.SetPosition(pos.x, pos.y, pos.z);
	trans.SetRotationRad(rot.x, rot.y, 0.0f);

	indicator.SetRotation(rot);
	proj.SetRotation(rot);

	indicator.SetPos(pos);
	proj.SetPos(pos);
}

void Camera::Reset(Graphics& gfx) noexcept 
{
	const auto homePos = transBase.position;
	const auto homeRot = transBase.rotation;

	if (!tethered)
	{
		trans.SetPosition(homePos.x, homePos.y, homePos.z);
	}
	trans.SetRotationRad(homeRot.x, homeRot.y, homeRot.z);

	indicator.SetPos(trans.GetPosition());
	proj.SetPos(trans.GetPosition());

	indicator.SetRotation(homeRot);
	proj.SetRotation(homeRot);

	proj.Reset(gfx);
}

void Camera::Rotate(float dx, float dy) noexcept
{
	auto rot = trans.GetRotation();

	rot.y += dx * rotationSpeed;
	rot.x += dy * rotationSpeed;
	constexpr float limit = 0.995f * DirectX::XM_PIDIV2;
	rot.x = std::clamp(rot.x, -limit, limit);
	trans.SetRotationRad(rot.x, rot.y, 0.0f);

	const dx::XMFLOAT3 angles = { rot.x,rot.y,rot.z };
	indicator.SetRotation(angles);
	proj.SetRotation(angles);
}

void Camera::Translate(DirectX::XMFLOAT3 translation) noexcept
{
	if (!tethered)
	{
		const auto rot = trans.GetRotation();

		auto move = DirectX::XMFLOAT3{ 
			translation.x * travelSpeed,
			translation.y * travelSpeed, 
			translation.z * travelSpeed 
		};

		DirectX::XMVECTOR local = DirectX::XMLoadFloat3(&move);
		DirectX::XMMATRIX rotMat = DirectX::XMMatrixRotationRollPitchYaw(rot.x, rot.y, rot.z);
		DirectX::XMVECTOR worldDelta = DirectX::XMVector3TransformNormal(local, rotMat);

		DirectX::XMFLOAT3 d;
		DirectX::XMStoreFloat3(&d, worldDelta);
		trans.Translate(d.x, d.y, d.z);

		const auto newPos = trans.GetPosition();
		indicator.SetPos(newPos);
		proj.SetPos(newPos);
	}
}

const DirectX::XMFLOAT3& Camera::GetPos() const noexcept
{
	return trans.GetPosition();
}

void Camera::SetPos(const DirectX::XMFLOAT3& pos) noexcept
{
	this->trans.SetPosition(pos.x, pos.y, pos.z);
	indicator.SetPos(pos);
	proj.SetPos(pos);
}

void Camera::SetRot(const DirectX::XMFLOAT3& rot) noexcept
{
	this->trans.SetRotationRad(rot.x, rot.y, rot.z);
	indicator.SetRotation(rot);
	proj.SetRotation(rot);
}

const std::string& Camera::GetName() const noexcept
{
	return name;
}

void Camera::LinkTechniques(Rgph::RenderGraph& rg)
{
	indicator.LinkTechniques(rg);
	proj.LinkTechniques(rg);
}

void Camera::Submit(size_t channels) const
{
	if (enableCameraIndicator)
	{
		indicator.Submit(channels);
	}
	if (enableFrustumIndicator)
	{
		proj.Submit(channels);
	}
}