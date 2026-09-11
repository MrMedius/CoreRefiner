#include "ModuleNodeFactory.h"
#include "ModuleNodes.h"

namespace ModuleNodeFactory
{
	std::unique_ptr<IModuleNode> MakeModuleNode(
		ModuleNodeLabel label,
		DirectX::XMFLOAT2 localPos)
	{
		switch (label)
		{
		case ModuleNodeLabel::Core_Ball:				return std::make_unique<ModuleNode_Spawn_Ball_Core>(localPos);
		case ModuleNodeLabel::Spawn_Ball:				return std::make_unique<ModuleNode_Spawn_Ball>(localPos);
		case ModuleNodeLabel::Attribute_LifetimeRate:	return std::make_unique<ModuleNode_Attribute_LifetimeRate>(localPos);
		case ModuleNodeLabel::Attribute_SpeedRate:		return std::make_unique<ModuleNode_Attribute_SpeedRate>(localPos);
		case ModuleNodeLabel::Attribute_SizeRate:		return std::make_unique<ModuleNode_Attribute_SizeRate>(localPos);
		case ModuleNodeLabel::Attribute_DamageRate:		return std::make_unique<ModuleNode_Attribute_DamageRate>(localPos);
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
