#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <memory>
#include <string>
#include <vector>

class BaseComponent;
class Scene;

class GameObject
{
public:
	enum class TransformationSpace
	{
		Local,
		World
	};

	GameObject();

	void SetParent(GameObject* newParent);
	GameObject* const GetParent() const;

	const std::vector<GameObject*>& GetChildren() const;

	void SetLocalPosition(const glm::vec3& desiredPosition);
	glm::vec3 GetLocalPosition() const;
	void SetWorldPosition(const glm::vec3& desiredPosition);
	glm::vec3 GetWorldPosition() const;
	void Translate(const glm::vec3& desiredTranslation, const TransformationSpace transformationSpace/* = TransformationSpace::World*/);

	void SetLocalRotation(const glm::vec3& desiredRotation);
	glm::vec3 GetLocalRotation() const;
	void SetLocalRotation(const glm::quat& desiredRotation);
	glm::quat GetLocalRotationQuaternion() const;

	void SetWorldRotation(const glm::vec3& desiredRotation);
	glm::vec3 GetWorldRotation() const;
	void SetWorldRotation(const glm::quat& desiredRotation);
	glm::quat GetWorldRotationQuaternion() const;

	void Rotate(const glm::vec3& desiredRotation, const TransformationSpace transformationSpace);
	void Rotate(const glm::quat& desiredRotation, const TransformationSpace transformationSpace);

	glm::vec3 GetForwardDirection() const;
	glm::vec3 GetUpDirection() const;
	glm::vec3 GetRightDirection() const;

	void SetScale(const glm::vec3& desiredScale);
	glm::vec3 GetScale() const;
	void Scale(const glm::vec3& desiredScale);

	const glm::mat4 GetWorldMatrix() const;

	void Update();
	void UpdateWorldMatrix();

	Scene* const GetScene() const;

	const std::vector<std::unique_ptr<BaseComponent>>& GetComponents() const;

	template<class ComponentType>
	ComponentType* CreateComponent()
	{
		std::unique_ptr<ComponentType> component = std::make_unique<ComponentType>(*this);
		components.emplace_back(std::move(component));

		return dynamic_cast<ComponentType*>(components.back().get());
	}
private:
	friend class Scene;

	void AddChild(GameObject* newChild);
	void RemoveChild(GameObject* child);

	void SetWorldMatrixUpdateNeeded(bool option);

	glm::vec3 forwardDirection = { 0.0f, 0.0f, 0.0f };
	glm::vec3 rightDirection = { 0.0f, 0.0f, 0.0f };
	glm::vec3 upDirection = { 0.0f, 0.0f, 0.0f };

	glm::vec3 localPosition = { 0.0f, 0.0f, 0.0f };
	glm::quat localRotation = {1.0f, 0.0f, 0.0f, 0.0f };

	glm::vec3 worldPosition = { 0.0f, 0.0f, 0.0f };
	glm::quat worldRotation = {1.0f, 0.0f, 0.0f, 0.0f};

	glm::vec3 scale = { 1.0f, 1.0f, 1.0f };

	glm::mat4x4 worldMatrix = glm::mat4x4(1.0f);

	Scene* scene = nullptr;

	GameObject* parent = nullptr;
	std::vector<GameObject*> children = {};

	std::vector<std::unique_ptr<class BaseComponent>> components = {};

	bool worldMatrixUpdateNeeded = true;
};

