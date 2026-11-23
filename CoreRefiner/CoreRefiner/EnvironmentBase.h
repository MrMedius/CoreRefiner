#pragma once
#include "ObjectBase.h"
#include "CharacterBase.h"

class EnvironmentBase : public ObjectBase
{
public:
	EnvironmentBase(Object_Type_Tag tag)
		:
		ObjectBase(tag)
	{}
	void OnEnable(void) override = 0;
	void Update(float dt) override = 0;
	void Submit(void) override = 0;
	virtual void OnCollide(CharacterBase* other) = 0;
};