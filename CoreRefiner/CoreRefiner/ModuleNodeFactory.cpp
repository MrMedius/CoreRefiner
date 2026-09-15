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
		// ————————————————————————————————————————————————————
		// Kind —— Core
		// ————————————————————————————————————————————————————
		case ModuleNodeLabel::Core_Ball:				return std::make_unique<ModuleNode_Core_Ball>(localPos);
		// ————————————————————————————————————————————————————
		// Kind —— Spawn
		// ————————————————————————————————————————————————————
		case ModuleNodeLabel::Spawn_Ball:				return std::make_unique<ModuleNode_Spawn_Ball>(localPos);
		// ————————————————————————————————————————————————————
		// Kind —— Attribute
		// ————————————————————————————————————————————————————
		case ModuleNodeLabel::Attribute_LifetimeRate:	return std::make_unique<ModuleNode_Attribute_LifetimeRate>(localPos);
		case ModuleNodeLabel::Attribute_SpeedRate:		return std::make_unique<ModuleNode_Attribute_SpeedRate>(localPos);
		case ModuleNodeLabel::Attribute_SizeRate:		return std::make_unique<ModuleNode_Attribute_SizeRate>(localPos);
		case ModuleNodeLabel::Attribute_DamageRate:		return std::make_unique<ModuleNode_Attribute_DamageRate>(localPos);
		// ————————————————————————————————————————————————————
		// Kind —— Rule
		// ————————————————————————————————————————————————————
		case ModuleNodeLabel::Rule_Orbit:				return std::make_unique<ModuleNode_Rule_Orbit>(localPos);
		case ModuleNodeLabel::Rule_Return:				return std::make_unique<ModuleNode_Rule_Return>(localPos);
		case ModuleNodeLabel::Rule_Child:				return std::make_unique<ModuleNode_Rule_Child>(localPos);
		case ModuleNodeLabel::Rule_Revive:				return std::make_unique<ModuleNode_Rule_Revive>(localPos);
		// ————————————————————————————————————————————————————
		// Kind —— Passive
		// ————————————————————————————————————————————————————
		case ModuleNodeLabel::Passive_DamageFix:		return std::make_unique<ModuleNode_Passive_DamageFix>(localPos);
		// ————————————————————————————————————————————————————
		// Kind —— Other
		// ————————————————————————————————————————————————————
		case ModuleNodeLabel::Other_Repeat:				return std::make_unique<ModuleNode_Other_Repeat>(localPos);
		// ————————————————————————————————————————————————————
		// Kind —— Fusion
		// ————————————————————————————————————————————————————
		case ModuleNodeLabel::Fusion:					return nullptr;
		// ————————————————————————————————————————————————————
		case ModuleNodeLabel::Count:					return nullptr;
		default:										return nullptr;
		}
	}

	std::unique_ptr<IModuleNode> MakeFusion(std::unique_ptr<IModuleNode> primary, std::unique_ptr<IModuleNode> material, DirectX::XMFLOAT2 localPos)
	{
		if (primary == nullptr || material == nullptr)
		{
			return nullptr;
		}
		// Core 只能当主体，不能当素材。
		if (material->IsCore())
		{
			return nullptr;
		}
		return std::make_unique<ModuleNode_Fusion>(std::move(primary), std::move(material), localPos);
	}
}
