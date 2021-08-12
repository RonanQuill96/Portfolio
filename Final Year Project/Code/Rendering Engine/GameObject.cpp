#include "GameObject.h"

#include "BaseComponent.h"
#include "Scene.h"

#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>

GameObject::GameObject() : localRotation(1.0, 0.0, 0.0, 0.0), worldRotation(1.0, 0.0, 0.0, 0.0)
{
}

void GameObject::SetParent(GameObject* newParent)
{
	if (parent != nullptr)
	{
		//remove as child from old parent 
		parent->RemoveChild(this);
	}

	parent = newParent;

	if (newParent != nullptr)
	{
		parent->AddChild(this);
	}

	SetWorldMatrixUpdateNeeded(true);
}

GameObject* const GameObject::GetParent() const
{
	return parent;
}

const std::vector<GameObject*>& GameObject::GetChildren() const
{
	return children;
}

void GameObject::SetLocalPosition(const glm::vec3& desiredPosition)
{
	localPosition = desiredPosition;
	SetWorldMatrixUpdateNeeded(true);
}

glm::vec3 GameObject::GetWorldPosition() const
{
	return worldPosition;
}

void GameObject::Translate(const glm::vec3& desiredTranslation, const TransformationSpace transformationSpace)
{
	if (transformationSpace == TransformationSpace::World)
	{
		localPosition += desiredTranslation;
	}
	else
	{
		localPosition += worldRotation * desiredTranslation;
	}

	SetWorldMatrixUpdateNeeded(true);
}


glm::vec3 GameObject::GetLocalPosition() const
{
	return localPosition;
}

void GameObject::SetWorldPosition(const glm::vec3& desiredPosition)
{
	if (parent != nullptr)
	{
		localPosition = desiredPosition - parent->worldPosition;
	}
	else
	{
		localPosition = desiredPosition;
	}
	SetWorldMatrixUpdateNeeded(true);
}

void GameObject::SetLocalRotation(const glm::vec3& desiredRotation)
{
	SetLocalRotation(glm::quat(desiredRotation));
}

void GameObject::SetLocalRotation(const glm::quat& desiredRotation)
{
	localRotation = desiredRotation;
	SetWorldMatrixUpdateNeeded(true);
}

glm::vec3 GameObject::GetWorldRotation() const
{
	return glm::eulerAngles(worldRotation);
}

glm::quat GameObject::GetWorldRotationQuaternion() const
{
	return worldRotation;
}

void GameObject::Rotate(const glm::vec3& desiredRotation, const TransformationSpace transformationSpace)
{
	Rotate(glm::quat(desiredRotation), transformationSpace);
}

void GameObject::Rotate(const glm::quat& desiredRotation, const TransformationSpace transformationSpace)
{
	if (transformationSpace == TransformationSpace::World)
	{
		localRotation = desiredRotation * localRotation;
	}
	else
	{
		localRotation = localRotation * desiredRotation;
	}

	SetWorldMatrixUpdateNeeded(true);
}

glm::vec3 GameObject::GetForwardDirection() const
{
	return forwardDirection;
}

glm::vec3 GameObject::GetUpDirection() const
{
	return upDirection;
}

glm::vec3 GameObject::GetRightDirection() const
{
	return rightDirection;
}

glm::vec3 GameObject::GetLocalRotation() const
{
	return glm::eulerAngles(localRotation);
}

glm::quat GameObject::GetLocalRotationQuaternion() const
{
	return localRotation;
}

void GameObject::SetWorldRotation(const glm::vec3& desiredRotation)
{
	SetWorldRotation(glm::quat(desiredRotation));
}

void GameObject::SetWorldRotation(const glm::quat& desiredRotation)
{
	if (parent != nullptr)
	{
		localRotation = glm::inverse(parent->worldRotation) * desiredRotation;
	}
	else
	{
		localRotation = desiredRotation;
	}
	SetWorldMatrixUpdateNeeded(true);
}

void GameObject::SetScale(const glm::vec3& desiredScale)
{
	scale = desiredScale;
	SetWorldMatrixUpdateNeeded(true);
}

void GameObject::Scale(const glm::vec3& desiredScale)
{
	scale *= desiredScale;
	SetWorldMatrixUpdateNeeded(true);
}

const glm::mat4 GameObject::GetWorldMatrix() const
{
	return worldMatrix;
}

void GameObject::Update()
{
	if (worldMatrixUpdateNeeded)
	{
		UpdateWorldMatrix();

		for (const std::unique_ptr<BaseComponent>& component : components)
		{
			component->Update();
		}

		for (GameObject* child : children)
		{
			child->SetWorldMatrixUpdateNeeded(true);
			child->Update();
		}
	}
}

void GameObject::UpdateWorldMatrix()
{
	const glm::vec4 UnitForward = { 0.0f, 0.0f, 1.0f, 0.0f };
	const glm::vec4 UnitRight = { 1.0f, 0.0f, 0.0f, 0.0f };
	const glm::vec4 UnitUp = { 0.0f, 1.0f, 0.0f, 0.0f };

	worldMatrix = glm::translate(localPosition) * glm::toMat4(localRotation) * glm::scale(scale);

	if (parent != nullptr)
	{
		worldMatrix = worldMatrix * parent->worldMatrix;
	}

	glm::vec3 worldScale;
	glm::vec3 skew;
	glm::vec4 perspective;
	glm::decompose(worldMatrix, worldScale, worldRotation, worldPosition, skew, perspective);

	const glm::mat4 worldRoationMatrix = glm::toMat4(worldRotation);

	forwardDirection =  worldRoationMatrix * UnitForward;
	forwardDirection = glm::normalize(forwardDirection);

	rightDirection = worldRoationMatrix * UnitRight;
	rightDirection = glm::normalize(rightDirection);

	upDirection = glm::cross(forwardDirection, rightDirection);
	//upDirection.Normalize();

	SetWorldMatrixUpdateNeeded(false);
}

Scene* const GameObject::GetScene() const
{
	return scene;
}

const std::vector<std::unique_ptr<BaseComponent>>& GameObject::GetComponents() const
{
	return components;
}

void GameObject::AddChild(GameObject* newChild)
{
	if (newChild != nullptr)
	{
		children.push_back(newChild);
	}
}

void GameObject::RemoveChild(GameObject* child)
{
	if (child != nullptr)
	{
		auto location = std::find(children.begin(), children.end(), child);
		if (location != children.end())
		{
			(*location)->SetWorldMatrixUpdateNeeded(true);
			children.erase(location);
		}
	}
}

void GameObject::SetWorldMatrixUpdateNeeded(bool option)
{
	worldMatrixUpdateNeeded = option;
}

glm::vec3 GameObject::GetScale() const
{
	return scale;
}