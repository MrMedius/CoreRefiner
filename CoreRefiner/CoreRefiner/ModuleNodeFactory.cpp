#include "ModuleNodeFactory.h"
#include "ModuleNodes.h"

namespace ModuleNodeFactory
{
	namespace
	{
		constexpr float kLifetimeSeconds = 2.0f;
		constexpr float kSpeedRate = 0.5f;
		constexpr float kSizeRate = 0.5f;
		constexpr float kDamageRate = 0.5f;
	}

	std::unique_ptr<IModuleNode> MakeModuleNode(
		ModuleNodeLabel label,
		DirectX::XMFLOAT2 localPos)
	{
		switch (label)
		{
		case ModuleNodeLabel::Core_Ball:				return std::make_unique<ModuleNode_Spawn_Ball_Core>(localPos);
		case ModuleNodeLabel::Spawn_Ball:				return std::make_unique<ModuleNode_Spawn_Ball>(localPos);
		case ModuleNodeLabel::Attribute_Lifetime:		return std::make_unique<ModuleNode_Attribute_Lifetime>(localPos, kLifetimeSeconds);
		case ModuleNodeLabel::Attribute_SpeedRate:		return std::make_unique<ModuleNode_Attribute_SpeedRate>(localPos, kSpeedRate);
		case ModuleNodeLabel::Attribute_SizeRate:		return std::make_unique<ModuleNode_Attribute_SizeRate>(localPos, kSizeRate);
		case ModuleNodeLabel::Attribute_DamageRate:		return std::make_unique<ModuleNode_Attribute_DamageRate>(localPos, kDamageRate);
		case ModuleNodeLabel::Rule_Orbit:				return std::make_unique<ModuleNode_Rule_Orbit>(localPos);
		case ModuleNodeLabel::Rule_Return:				return std::make_unique<ModuleNode_Rule_Return>(localPos);
		case ModuleNodeLabel::Passive_DamageFix:		return std::make_unique<ModuleNode_Passive_DamageFix>(localPos);
		case ModuleNodeLabel::Other_Child:				return std::make_unique<ModuleNode_Other_Child>(localPos);
		case ModuleNodeLabel::Other_Revive:				return std::make_unique<ModuleNode_Other_Revive>(localPos);
		case ModuleNodeLabel::Other_Repeat:				return std::make_unique<ModuleNode_Other_Repeat>(localPos);
		case ModuleNodeLabel::Count:
		default:
			return nullptr;
		}
	}
}
