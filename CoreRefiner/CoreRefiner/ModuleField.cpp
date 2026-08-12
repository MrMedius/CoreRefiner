#include "ModuleField.h"
#include "Channels.h"
#include "RenderGraph.h"

void ModuleField::InitAllVisuals(Graphics& gfx, Rgph::RenderGraph& rg, DirectX::XMFLOAT3 fieldOrigin)
{
	origin_ = fieldOrigin;
	if (canvas_ == nullptr)
	{
		constexpr float side = ModuleFieldCanvas::kDefaultFieldSide;
		canvas_ = std::make_unique<ModuleFieldCanvas>(gfx, 300u, 300u);
		canvas_->SetScale(DirectX::XMFLOAT3{ side, side, 1.0f });
		canvas_->LinkTechniques(rg);
	}
	canvas_->SetPosition(origin_);

	for (auto& n : nodes_)
	{
		if (n != nullptr)
		{
			n->InitVisual(gfx, rg, origin_);
		}
	}
}

void ModuleField::SetFieldOrigin(DirectX::XMFLOAT3 fieldOrigin) noexcept
{
	origin_ = fieldOrigin;
	if (canvas_ != nullptr)
	{
		canvas_->SetPosition(origin_);
	}
	for (auto& n : nodes_)
	{
		if (n != nullptr)
		{
			n->SetFieldOrigin(origin_);
		}
	}
}

void ModuleField::SyncAllVisuals()
{
	for (auto& n : nodes_)
	{
		if (n != nullptr)
		{
			n->SyncVisual();
		}
	}
}

void ModuleField::SubmitBackground()
{
	if (canvas_ != nullptr)
	{
		canvas_->Submit(Chan::ui);
	}
}

void ModuleField::SubmitNodes()
{
	for (auto& n : nodes_)
	{
		if (n != nullptr)
		{
			n->SubmitVisual();
		}
	}
}

void ModuleField::SubmitAllVisuals()
{
	SubmitBackground();
	SubmitNodes();
}
