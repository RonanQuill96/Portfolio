#pragma once

#include "BaseComponent.h"

#include "Frustum.h"

#include <glm/glm.hpp>

#include <array>



class CameraComponent : public BaseComponent
{
public:
	CameraComponent(GameObject& pOwner);

	void RegisterComponent() override;
	void UnregisterComponent() override;

	void Update() override;

	void CreateCamera(const float FOV, const float desiredAspectRatio, const float desiredNearPlane, const float desiredFarPlane);

	void LookAt(glm::vec3 position);

	void SetFOV(const float desiredFieldOfView);
	float GetFOV() const;

	void SetCameraAspectRatio(const float desiredAspectRatio);
	float GetCameraAspectRatio() const;

	void SetNearPlane(const float desiredNearPlane);
	float GetNearPlane() const;

	void SetFarPlane(const float desiredFarPlane);
	float GetFarPlane() const;

	const glm::mat4x4& GetView() const;
	const glm::mat4x4& GetProjection() const;

	const Frustum& GetViewFrustum() const;

	std::array<glm::vec3, 8> GetFrustumPoints() const;

private:
	void CaluclateViewMatrix();
	void CalculateViewFrustum();

	glm::mat4x4 view;
	glm::mat4x4 projection;

	float fieldOfView;
	float aspectRatio;
	float nearPlane;
	float farPlane;

	Frustum viewFrustum;

	size_t Left;
	size_t Right;
	size_t Top;
	size_t Bottom;
	size_t Near;
	size_t Far;
};

