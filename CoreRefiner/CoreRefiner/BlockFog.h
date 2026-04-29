#pragma once
#include "Environment.h"
#include "RenderGraph.h"
#include "FogVolume.h"
#include "Channels.h"

class BlockFog : public Environment
{
public:
	BlockFog(Graphics& gfx, Rgph::RenderGraph& rg, XMFLOAT3 position, XMFLOAT3 size, Object_Type_Tag tag = environment_BlockFog)
		:
		Environment(tag)
	{
		// parameters init
		SetPosition(position);
		SetSize(size);

		// graphics init
		visualPre = std::make_unique<FogVolume>(gfx, size);
		visualPre->SetPosition(transInfo.position);
		visualPre->LinkTechniques(rg);
	}
	void OnEnable(void) override {}
	void Update(float dt) override {}
	void Submit(void) override
	{
		visualPre->Submit(Chan::main);
	}
	void OnCollide(Character* other) override {}
private:
	std::unique_ptr<FogVolume> visualPre;
};

