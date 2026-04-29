#include "PointLight.h"
#include "imgui/imgui.h"
#include "Camera.h"
#include "Math.h"

PointLight::PointLight(Graphics& gfx, DirectX::XMFLOAT3 pos, float radius) 
	:
	mesh(gfx, radius),
	cbuf(gfx)
{
	home = {
		pos,
		{ 1.0f,1.0f,1.0f },
		{ 1.0f,1.0f,1.0f },
		0.75f,
		0.3f,
		0.03f,
		0.00001f,
	};
	Reset();
	pCamera = std::make_shared<Camera>(gfx, "Light", cbData.pos, 0.0f, PI / 2.0f, true);
}

void PointLight::Update(float dt, DirectX::XMFLOAT3 pos) noexcept
{
	const float zBias = 10.0f;    // always bigger than player's Z
	const float speed = 20.0f;    // chase speed
	const float dist2Threshold = 50.0f;

	// always bigger than player's Z
	const float targetX = pos.x;
	const float targetY = pos.y;
	const float targetZ = pos.z + zBias;

	const float dx = targetX - cbData.pos.x;
	const float dz = targetZ - cbData.pos.z;

	if (dx * dx + dz * dz > dist2Threshold)
	{
		const float angle = atan2f(dz, dx);
		cbData.pos.x += cosf(angle) * speed * dt;
		cbData.pos.z += sinf(angle) * speed * dt;
	}
	else
	{
		if (cbData.pos.z <= targetZ)
			cbData.pos.z = targetZ;
	}

	if (cbData.pos.y - targetY > 25.0f)
	{
		cbData.pos.y = targetY + 25.0f;
	}

	pCamera->SetPos(cbData.pos);
}

void PointLight::SpawnControlWindow() noexcept
{
	bool dirtyPos = false;
	const auto d = [&dirtyPos](bool dirty) {dirtyPos = dirtyPos || dirty;};

	if (ImGui::Begin("Light"))
	{
		ImGui::Text("Position");
		d(ImGui::SliderFloat("X", &cbData.pos.x, -60.0f, 60.0f, "%.1f"));
		d(ImGui::SliderFloat("Y", &cbData.pos.y, -60.0f, 60.0f, "%.1f"));
		d(ImGui::SliderFloat("Z", &cbData.pos.z, -60.0f, 60.0f, "%.1f"));

		if (dirtyPos)
		{
			pCamera->SetPos(cbData.pos);
		}

		ImGui::Text("Intensity/Color");
		ImGui::SliderFloat("Intensity", &cbData.diffuseIntensity, 0.01f, 2.0f, "%.2f", 2);
		ImGui::ColorEdit3("Diffuse Color", &cbData.diffuseColor.x);
		ImGui::ColorEdit3("Ambient", &cbData.ambient.x);

		ImGui::Text("Falloff");
		ImGui::SliderFloat("Constant", &cbData.attConst, 0.05f, 10.0f, "%.2f", 4);
		ImGui::SliderFloat("Linear", &cbData.attLin, 0.0001f, 4.0f, "%.4f", 8);
		ImGui::SliderFloat("Quadratic", &cbData.attQuad, 0.0000001f, 10.0f, "%.7f", 10);

		if (ImGui::Button("Reset"))
		{
			Reset();
		}
	}
	ImGui::End();
}

void PointLight::Reset() noexcept
{
	cbData = home;
}

void PointLight::Submit(size_t channels) const noxnd
{
	mesh.SetPos(cbData.pos);
	mesh.Submit(channels);
}

void PointLight::Bind(Graphics& gfx, DirectX::FXMMATRIX view) const noexcept
{
	auto dataCopy = cbData;
	const auto pos = DirectX::XMLoadFloat3(&cbData.pos);
	DirectX::XMStoreFloat3(&dataCopy.pos, DirectX::XMVector3Transform(pos, view));
	cbuf.Update(gfx, dataCopy);
	cbuf.Bind(gfx);
}

void PointLight::LinkTechniques(Rgph::RenderGraph& rg)
{
	mesh.LinkTechniques(rg);
}

std::shared_ptr<Camera> PointLight::ShareCamera() const noexcept
{
	return pCamera;
}