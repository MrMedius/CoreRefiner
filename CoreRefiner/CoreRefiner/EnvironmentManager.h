#pragma once
#include "Graphics.h"
#include "RenderGraph.h"

#include "Environment.h"

class EnvironmentManager
{
public:
	EnvironmentManager(Graphics& gfx, Rgph::RenderGraph& rg);
	~EnvironmentManager() = default;
	void Update(float dt);
	void Submit(void);
	void Reset(void);
private:
	Graphics& gfx;
	Rgph::RenderGraph& rg;

	std::vector<Environment*> eG;

	bool isLearnt{ false };
	bool isTutorial{ true };
};