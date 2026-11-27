#pragma once
#include "ModuleNodeLabel.h"
#include "IModuleNode.h"

#include <DirectXMath.h>
#include <memory>

namespace ModuleNodeFactory
{
	[[nodiscard]] std::unique_ptr<IModuleNode> MakeModuleNode(ModuleNodeLabel label, DirectX::XMFLOAT2 localPos);

	[[nodiscard]] std::unique_ptr<IModuleNode> MakeFusion(std::unique_ptr<IModuleNode> primary, std::unique_ptr<IModuleNode> material, DirectX::XMFLOAT2 localPos);

	// 名字取当前语言的默认标题，图案用圆框，3 个空栏位。
	[[nodiscard]] std::unique_ptr<IModuleNode> MakeUltra(DirectX::XMFLOAT2 localPos);
}
