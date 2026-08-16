#pragma once
#include "ModuleNodeLabel.h"
#include "IModuleNode.h"

#include <DirectXMath.h>
#include <memory>

namespace ModuleNodeFactory
{
	[[nodiscard]] std::unique_ptr<IModuleNode> MakeModuleNode(ModuleNodeLabel label, DirectX::XMFLOAT2 localPos);
}
