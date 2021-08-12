#pragma once

class GameObject;

class BaseComponent
{
public:
	BaseComponent(GameObject& pOwner);
	virtual ~BaseComponent() = default;

	virtual void RegisterComponent() = 0;
	virtual void UnregisterComponent() = 0;

	virtual void Update() {}

	GameObject& GetOwner();
	const GameObject& GetOwner() const;

private:
	GameObject& owner;

};

