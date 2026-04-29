#pragma once
#include "Graphics.h"
#include "Sprite2D.h"
#include "Channels.h"
#include "GameStatsCodex.h"

class UI_Score
{
public:
	UI_Score(Graphics& gfx, Rgph::RenderGraph& rg)
	{
		for (int i = 0; i < digitsNum; i++)
		{
			num.push_back(std::make_unique<Sprite2D>(gfx, std::vector<std::string>{ "asset\\Images\\UI_Game\\Number_No_2.png" }));
			num[i]->SetPosition(250.0f - 30.0f * i, 50.0f);
			num[i]->SetScale(50.0f, 50.0f);
			num[i]->LinkTechniques(rg);
		}
	}

	~UI_Score() = default;

	void Update(float dt)
	{
		float scoreCumulated = (float)GameStatsCodex::Get().score;
		auto SmoothTo = [](float& cur, float target, float k, float dt)
			{
				if (dt <= 0.0f) { cur = target; return; }
				const float a = 1.0f - std::exp(-k * dt);   // 0..1 the bigger k is, the faster number changes
				cur += (target - cur) * a;
			};

		SmoothTo(scoreDraw, scoreCumulated, 20.0f, dt);
		const int sd = (int)(scoreDraw + 0.5f);
		if (sd != prevScoreDraw) 
		{
			prevScoreDraw = sd;
		}

		int v = sd;
		for (int i = 0; i < digitsNum; i++)
		{
			const int d = v % 10; v /= 10;
			num[i]->SetFrame(5, 2, d, 1, 1, 0, false);
		}
	}

	void Submit(void)
	{
		for(auto& n : num)
		{
			n->Submit(Chan::ui);
		}
	}

	void Reset(void)
	{
		scoreDraw = 0;
	}
private:
	static constexpr int digitsNum = 8;
	std::vector<std::unique_ptr<Sprite2D>> num;
	float scoreDraw{ 0 }; // 描く用のスコア
	int prevScoreDraw = 0;
};