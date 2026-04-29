#pragma once
#include "Graphics.h"
#include "ObjectCodex.h"
#include "Sprite2D.h"
#include "Channels.h"
#include <random>

class UI_Loading
{
public:
	UI_Loading(Graphics& gfx, Rgph::RenderGraph& rg)
	{
		// Loading_Bg
		{
			pBg = std::make_unique<Sprite2D>(gfx, std::vector<std::string>{ "asset\\Images\\UI_Loading\\Loading_Bg.png" });
			pBg->SetScale(SCREEN_WIDTH * 2.0f, SCREEN_HEIGHT);
			pBg->SetFrame(1, 1, 1, 1, 1, 0, false);
			pBg->LinkTechniques(rg);
		}
		// Loading_Cut
		{
			for (int i = 0;i < TotalCutCount;i++)
			{
				float posY = SCREEN_HEIGHT - 100.0f;
				pCut.push_back(std::make_unique<Sprite2D>(gfx, std::vector<std::string>{ "asset\\Images\\UI_Loading\\Loading_Cut.png" }));
				pCut[i]->SetScale(150.0f, 150.0f);
				pCut[i]->SetFrameAuto(12, 14, 1 + i * 24, 24, 0, 20.0f);
				pCut[i]->LinkTechniques(rg);
			}
			speedCut.push_back(3.5f);
			speedCut.push_back(2.0f); speedCut.push_back(1.0f); speedCut.push_back(3.0f);
			speedCut.push_back(1.5f); speedCut.push_back(0.5f); speedCut.push_back(2.5f);
		}
		// Loading_Octo
		{
			for (int i = 0;i < TotalOctoCount;i++)
			{
				pOcto.push_back(std::make_unique<Sprite2D>(gfx, std::vector<std::string>{ "asset\\Images\\UI_Loading\\Loading_Octo.png" }));
				pOcto[i]->SetScale(500.0f, 500.0f);
				pOcto[i]->SetFrame(2, 2, 1 + i, 1, 1, 0);
				pOcto[i]->LinkTechniques(rg);
			}
		}
		// Loading_Info
		{
			float scale = 80.0f;
			pInfo = std::make_unique<Sprite2D>(gfx, std::vector<std::string>{ "asset\\Images\\UI_Loading\\Loading_Info.png" });
			pInfo->SetPosition(SCREEN_WIDTH / 2, 350.0f);
			pInfo->SetScale(16.0f * scale, 9.0f * scale);
			pInfo->SetFrame(1, 1, 1, 1, 1, 0, false);
			pInfo->LinkTechniques(rg);
		}
		// Loading
		{
			float posX = SCREEN_WIDTH / 2 + 340.0f;
			float posY = SCREEN_HEIGHT - 130.0f;
			pLoading.push_back(std::make_unique<Sprite2D>(gfx, std::vector<std::string>{ "asset\\Images\\UI_Loading\\Loading.png" }));
			pLoading[0]->SetPosition(posX, posY);
			pLoading[0]->SetScale(300.0f, 300.0f);
			pLoading[0]->SetFrame(1, 1, 1, 1, 1, 0, false);
			pLoading[0]->LinkTechniques(rg);
			for (int i = 1;i < LoadingCount;i++)
			{
				pLoading.push_back(std::make_unique<Sprite2D>(gfx, std::vector<std::string>{ "asset\\Images\\UI_Loading\\Loading_Dot.png" }));
				pLoading[i]->SetPosition(posX + 120.0f + 30.0f * (float)i, posY);
				pLoading[i]->SetScale(300.0f, 300.0f);
				pLoading[i]->SetFrame(1, 1, 1, 1, 1, 0, false);
				pLoading[i]->LinkTechniques(rg);
			}
		}
	}
	~UI_Loading() = default;

	void Update(float dt)
	{
		// Countdown
		counter += dt;
		// Loading_Bg
		{
			auto pos = pBg->GetPosition();
			pBg->SetPosition(pos.x + speedBg, pos.y);
		}
		// Loading_Cut
		for (int i = 0;i < cutCount;i++)
		{
			int idx = numCut[i];
			pCut[idx]->Update(dt);
			auto pos = pCut[idx]->GetPosition();
			pCut[idx]->SetPosition(pos.x + speedCut[idx], pos.y);
		}
		// Loading
		drawLoading = static_cast<int>(counter * 5.0f) % LoadingCount + 1;
		// Loading_Octo
		for (int i = 0;i < octoCount;i++)
		{
			int idx = numOcto[i];
			auto pos = pOcto[idx]->GetPosition();
			pOcto[idx]->SetPosition(pos.x + speedOcto[i], pos.y);

			auto rot = XMConvertToDegrees(pOcto[idx]->GetRotation().z) + rotOcto[i];
			if (std::abs(rot) > OctoWave) rotOcto[i] = -rotOcto[i];
			pOcto[idx]->SetRotation(rot);
		}
	}

	void Submit(void)
	{
		// Loading_Bg
		pBg->Submit(Chan::ui);
		// Loading_Info
		pInfo->Submit(Chan::ui);
		// Loading_Cut
		for (int i = 0;i < cutCount;i++)	pCut[numCut[i]]->Submit(Chan::ui);
		// Loading
		for (int i = 0;i < drawLoading;i++) pLoading[i]->Submit(Chan::ui);
		// Loading_Octo
		for (int i = 0;i < octoCount;i++)	pOcto[numOcto[i]]->Submit(Chan::ui);
	}

	void StartLoading(int scene)
	{
		toScene = scene;
		std::mt19937 rng(std::random_device{}());
		std::uniform_real_distribution<float> speed(-1.0f, 1.0f);
		// Loading
		{
			speedBg = speed(rng) > 0.0f ? 1.0f : -1.0f;
			pBg->SetPosition(speedBg > 0.0f ? 0.0f : SCREEN_WIDTH, SCREEN_HEIGHT / 2);
		}
		// Loading_Cut
		{
			std::vector<int> pool = { 0,1,2,3,4,5,6 };
			std::shuffle(pool.begin(), pool.end(), rng);
			std::uniform_int_distribution<int> num(3, 6);
			cutCount = num(rng);
			for (int i = 0;i < cutCount;i++)
			{
				int idx = pool[i];
				numCut.push_back(idx);

				speedCut[idx] = std::abs(speedCut[idx]) * (speed(rng) >= 0 ? -1.0f : 1.0f);

				pCut[idx]->SetFlip(speedCut[idx] > 0);

				float posX = speedCut[idx] > 0 ? -10.0f : SCREEN_WIDTH + 10.0f;
				float posY = SCREEN_HEIGHT - 165.0f;
				pCut[idx]->SetPosition(posX, posY);
			}
		}
		// Loading_Octo
		{
			std::vector<int> pool = { 0,1,2,3 };
			std::shuffle(pool.begin(), pool.end(), rng);
			std::uniform_int_distribution<int> num(2, 4);
			std::vector<float> x = { 100.0f,300.0f,500.0f,700.0f,900.0f,1100.0f };
			std::shuffle(x.begin(), x.end(), rng);
			octoCount = num(rng);
			for (int i = 0;i < octoCount;i++)
			{
				int idx = pool[i];
				numOcto.push_back(idx);

				float sp = speed(rng);
				sp = std::abs(sp) < 0.5f ? (sp + (sp > 0.0f) ? 1.0f : -1.0f) : sp;
				speedOcto.push_back(sp);
				pOcto[idx]->SetFrame(2, 2, 1 + idx, 1, 1, 0, speedOcto[i] > 0.0f);

				rotOcto.push_back(sp / 2);

				float posX = x[i];
				float posY = SCREEN_HEIGHT - 140.0f;
				pOcto[idx]->SetPosition(posX, posY);
			}
		}
	}
	bool FinishLoading(void)
	{
		if (counter >= Countdown)
		{
			// Countdown
			counter = 0.0f;
			// Loading_Bg
			pBg->SetPosition(0.0f, SCREEN_HEIGHT / 2);
			// Loading_Cut
			cutCount = 0;
			numCut.clear();
			// Loading_Octo
			octoCount = 0;
			numOcto.clear();
			speedOcto.clear();
			rotOcto.clear();
			// Loading
			drawLoading = 1;
			return true;
		}
		return false;
	}
	int SceneLoading(void)
	{
		return toScene;
	}

private:
	// Countdown
	static constexpr float Countdown = 5.0f;
	float counter{ 0.0f };
	int toScene{ 0 };
	// Loading_Bg
	float speedBg{ 1.0f };
	std::unique_ptr<Sprite2D> pBg;
	// Loading_Cut
	static constexpr int TotalCutCount = 7;
	int cutCount{ 0 };
	std::vector<int> numCut;
	std::vector<float> speedCut;
	std::vector<std::unique_ptr<Sprite2D>> pCut;
	// Loading_Octo
	static constexpr int TotalOctoCount = 4;
	static constexpr float OctoWave = 10.0f;
	int octoCount{ 0 };
	std::vector<int> numOcto;
	std::vector<float> speedOcto;
	std::vector<float> rotOcto;
	std::vector<std::unique_ptr<Sprite2D>> pOcto;
	// Loading_Info
	std::unique_ptr<Sprite2D> pInfo;
	// Loading
	static constexpr int LoadingCount = 4;
	int drawLoading{ 1 };
	std::vector<std::unique_ptr<Sprite2D>> pLoading;
};