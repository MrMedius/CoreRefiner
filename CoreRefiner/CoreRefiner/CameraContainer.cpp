#include "CameraContainer.h"
#include "imgui/imgui.h"
#include "Camera.h"
#include "Graphics.h"
#include "RenderGraph.h"
#include "InputCodex.h"
#include "XMath.h"

namespace dx = DirectX;

CameraContainer::CameraContainer(Graphics& gfx)
{
	AddCamera(std::make_unique<Camera>(gfx, "Main Camera", dx::XMFLOAT3{ 0.0f,30.0f,0.0f }, PI / 3.0f, 0.0f));
	AddCamera(std::make_unique<Camera>(gfx, "Observer", dx::XMFLOAT3{ 0.0f,10.0f,0.0f }, PI / 180.0f * 13.0f, PI / 180.0f * 61.0f));
}

void CameraContainer::Bind( Graphics& gfx )
{
	gfx.SetCamera((*this)->GetMatrix());
}

void CameraContainer::AddCamera( std::shared_ptr<Camera> pCam )
{
	cameras.push_back( std::move( pCam ) );
}

Camera* CameraContainer::operator->() 
{
	return &GetActiveCamera();
}

CameraContainer::~CameraContainer()
{}

void CameraContainer::Update(float dt, DirectX::XMFLOAT3 pos,Window* wnd) noexcept
{
	// Main Camera Follow
	FollowTarget = Lerp(V(FollowTarget), V(pos), FollowLerp).ToFloat3();
	constexpr float yaw = 0.0f;
	const float sx = sinf(FollowPitch);
	const float cx = cosf(FollowPitch);
	const float sy = sinf(yaw);
	const float cy = cosf(yaw);

	const Vec3 camPos = V(FollowTarget) + Vec3{
		sy * cx * FollowRadius,
		sx * FollowRadius,
		-cy * cx * FollowRadius
	};
	const dx::XMFLOAT3 camRot = { FollowPitch, yaw, 0.0f };
	cameras[0]->SetPos(camPos.ToFloat3());
	cameras[0]->SetRot(camRot);

	// Screen Effects
	if (ShakeFrames > 0) ScreenShake();
	if (FrozeFrames > 0) ScreenFroze();

#ifdef _DEBUG
	auto& input = InputCodex::Get();

	while (const auto e = input.Get().ReadKey())
	{
		if (!e->IsPress())
		{
			continue;
		}

		switch (e->GetCode())
		{
		case VK_ESCAPE:
			if (wnd->CursorEnabled())
			{
				wnd->DisableCursor();
				input.Get().EnableRawMouse();
			}
			else
			{
				wnd->EnableCursor();
				input.Get().DisableRawMouse();
			}
			break;
		}
	}

	if (!wnd->CursorEnabled() && controlled != 0)
	{
		if (input.Get().KeyPressed(KK_UP))	  GetControlledCamera().Translate({   0,  0, dt });
		if (input.Get().KeyPressed(KK_DOWN))  GetControlledCamera().Translate({   0,  0,-dt });
		if (input.Get().KeyPressed(KK_LEFT))  GetControlledCamera().Translate({ -dt,  0,  0 });
		if (input.Get().KeyPressed(KK_RIGHT)) GetControlledCamera().Translate({  dt,  0,  0 });
	}

	while (const auto delta = input.Get().ReadRawDelta())
	{
		if (!wnd->CursorEnabled() && controlled != 0)
		{
			GetControlledCamera().Rotate(delta->x, delta->y);
		}
	}
#endif
}

void CameraContainer::Submit(size_t channels) const
{
	for (size_t i = 0; i < cameras.size(); i++)
	{
		if (i != active)
		{
			cameras[i]->Submit(channels);
		}
	}
}

void CameraContainer::Reset()
{
	FollowTarget = { 0.0f, 40.0f, 0.0f };
	cameras[0]->SetPos(FollowTarget);
}


void CameraContainer::SpawnWindow(Graphics& gfx)
{
	if (ImGui::Begin("Cameras"))
	{
		if (ImGui::BeginCombo("Active Camera", (*this)->GetName().c_str()))
		{
			for (int i = 0; i < std::size(cameras); i++)
			{
				const bool isSelected = i == active;
				if (ImGui::Selectable(cameras[i]->GetName().c_str(), isSelected))
				{
					active = i;
				}
			}
			ImGui::EndCombo();
		}

		if (ImGui::BeginCombo("Controlled Camera", GetControlledCamera().GetName().c_str()))
		{
			for (int i = 0; i < std::size(cameras); i++)
			{
				const bool isSelected = i == controlled;
				if (ImGui::Selectable(cameras[i]->GetName().c_str(), isSelected))
				{
					controlled = i;
				}
			}
			ImGui::EndCombo();
		}

		GetControlledCamera().SpawnControlWidgets(gfx);
	}
	ImGui::End();
}

void CameraContainer::LinkTechniques(Rgph::RenderGraph& rg)
{
	for (auto& pcam : cameras)
	{
		pcam->LinkTechniques(rg);
	}
}

Camera& CameraContainer::GetActiveCamera()
{
	return *cameras[active];
}

Camera& CameraContainer::GetControlledCamera()
{
	return *cameras[controlled];
}





// Camera Shake Setter
void CameraContainer::SetScreenShake(int frames, float minRange, float maxRange)
{
	ShakeFrames = frames;
	ShakeMinRange = minRange;
	ShakeMaxRange = maxRange;
}

// Frame Freeze Setter
void CameraContainer::SetScreenFroze(int frames)
{
	FrozeFrames = frames;
	FrozeScreen = true;
}

// Camera Shake
void CameraContainer::ScreenShake(void)
{
	std::mt19937 rng(std::random_device{}());
	std::uniform_real_distribution<float> rad(1, 360);
	std::uniform_real_distribution<float> range(ShakeMinRange, ShakeMaxRange);

	float shake_rad = rad(rng);
	float shake_range = range(rng);

	float shake_x = shake_range * sinf(shake_rad * PI / 180);
	float shake_y = shake_range * cosf(shake_rad * PI / 180);

	FollowTarget = (V(FollowTarget) + Vec3{ shake_x, shake_y, 0.0f }).ToFloat3();

	ShakeFrames--;
}

// Frame Freeze
void CameraContainer::ScreenFroze(void)
{
	if (--FrozeFrames <= 0)
		FrozeScreen = false;
}
