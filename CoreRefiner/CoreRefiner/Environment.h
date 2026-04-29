#pragma once
#include "ObjectBase.h"
#include "Character.h"

class Environment : public ObjectBase
{
public:
	Environment(Object_Type_Tag tag)
		:
		ObjectBase(tag)
	{}
	void OnEnable(void) override = 0;
	void Update(float dt) override = 0;
	void Submit(void) override = 0;
	virtual void OnCollide(Character* other) = 0;
};