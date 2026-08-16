#include "ModuleNodeFactory.h"
#include "ModuleNodes.h"

namespace ModuleNodeFactory
{
	namespace
	{
		constexpr float kLifetimeSeconds = 2.0f;
		constexpr float kSpeedRate = 0.5f;
	}

	std::unique_ptr<IModuleNode> MakeModuleNode(
		ModuleNodeLabel label,
		DirectX::XMFLOAT2 localPos)
	{
		switch (label)
		{
		case ModuleNodeLabel::Spawn_Ball:
			return std::make_unique<ModuleNode_Spawn_Ball>(localPos);
		case ModuleNodeLabel::Attribute_Lifetime:
			return std::make_unique<ModuleNode_Attribute_Lifetime>(localPos, kLifetimeSeconds);
		case ModuleNodeLabel::Attribute_SpeedRate:
			return std::make_unique<ModuleNode_Attribute_SpeedRate>(localPos, kSpeedRate);
		case ModuleNodeLabel::Rule_Orbit:
			return std::make_unique<ModuleNode_Rule_Orbit>(localPos);
		case ModuleNodeLabel::Other_Child:
			return std::make_unique<ModuleNode_Other_Child>(localPos);
		case ModuleNodeLabel::Count:
		default:
			return nullptr;
		}
	}
}
