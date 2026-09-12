#pragma once
#include "ModuleNodeLabel.h"
#include "IModuleNode.h"

#include <DirectXMath.h>
#include <memory>

namespace ModuleNodeFactory
{
	[[nodiscard]] std::unique_ptr<IModuleNode> MakeModuleNode(ModuleNodeLabel label, DirectX::XMFLOAT2 localPos);

	[[nodiscard]] std::unique_ptr<IModuleNode> MakeFusion(std::unique_ptr<IModuleNode> primary, std::unique_ptr<IModuleNode> material, DirectX::XMFLOAT2 localPos);
}
