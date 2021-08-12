#include "BaseComponent.h"

BaseComponent::BaseComponent(GameObject& pOwner) : owner(pOwner)
{
}

GameObject& BaseComponent::GetOwner()
{
	return owner;
}

const GameObject& BaseComponent::GetOwner() const
{
	return owner;
}
