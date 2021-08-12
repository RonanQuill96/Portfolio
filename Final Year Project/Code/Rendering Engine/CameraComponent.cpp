#include "CameraComponent.h"

#include "VulkanIncludes.h"
#include "GameObject.h"
#include "Scene.h"

#include <glm/gtx/quaternion.hpp>

CameraComponent::CameraComponent(GameObject& pOwner) : BaseComponent(pOwner)
{
}

void CameraComponent::RegisterComponent()
{
    GetOwner().GetScene()->SetActiveCamera(this);
}

void CameraComponent::UnregisterComponent()
{
    GetOwner().GetScene()->SetActiveCamera(nullptr);
}

void CameraComponent::Update()
{
    CaluclateViewMatrix();
    CalculateViewFrustum();
}

void CameraComponent::CreateCamera(const float FOV, const float desiredAspectRatio, const float desiredNearPlane, const float desiredFarPlane)
{
    fieldOfView = FOV;
    nearPlane = desiredNearPlane;
    farPlane = desiredFarPlane;
    aspectRatio = desiredAspectRatio;

    CaluclateViewMatrix();

    projection = glm::perspective(fieldOfView, aspectRatio, nearPlane, farPlane);
    projection[1][1] *= -1;
}

void CameraComponent::LookAt(glm::vec3 position)
{
    glm::mat4 RotationMatrix = glm::transpose(glm::lookAt(GetOwner().GetWorldPosition(), position, glm::vec3(0.0f, 1.0f, 0.0f)));
    glm::quat Rotation = glm::toQuat(RotationMatrix);

    GetOwner().SetWorldRotation(Rotation);
}

void CameraComponent::SetFOV(const float desiredFieldOfView)
{
    fieldOfView = desiredFieldOfView;
    projection = glm::perspective(fieldOfView, aspectRatio, nearPlane, farPlane);
    projection[1][1] *= -1;
}

float CameraComponent::GetFOV() const
{
    return fieldOfView;
}

void CameraComponent::SetCameraAspectRatio(const float desiredAspectRatio)
{
    aspectRatio = desiredAspectRatio;
    projection = glm::perspective(fieldOfView, aspectRatio, nearPlane, farPlane);
    projection[1][1] *= -1;
}

float CameraComponent::GetCameraAspectRatio() const
{
    return aspectRatio;
}

void CameraComponent::SetNearPlane(const float desiredNearPlane)
{
    nearPlane = desiredNearPlane;
    projection = glm::perspective(fieldOfView, aspectRatio, nearPlane, farPlane);
    projection[1][1] *= -1;
}

float CameraComponent::GetNearPlane() const
{
    return nearPlane;
}

void CameraComponent::SetFarPlane(const float desiredFarPlane)
{
    farPlane = desiredFarPlane;
    projection = glm::perspective(fieldOfView, aspectRatio, nearPlane, farPlane);
    projection[1][1] *= -1;
}

float CameraComponent::GetFarPlane() const
{
    return farPlane;
}

const glm::mat4x4& CameraComponent::GetView() const
{
    return view;
}

const glm::mat4x4& CameraComponent::GetProjection() const
{
    return projection;
}

const Frustum& CameraComponent::GetViewFrustum() const
{
    return viewFrustum;
}

glm::vec3 IntersectPlanes(glm::vec4 P0, glm::vec4 P1, glm::vec4 P2)
{
    glm::vec3 bxc = glm::cross(glm::vec3(P1), glm::vec3(P2));
    glm::vec3 cxa = glm::cross(glm::vec3(P2), glm::vec3(P0));
    glm::vec3 axb = glm::cross(glm::vec3(P0), glm::vec3(P1));
    glm::vec3 r = -P0.w * bxc - P1.w * cxa - P2.w * axb;

    return r / glm::dot(glm::vec3(P0), bxc);
}

std::array<glm::vec3, 8> CameraComponent::GetFrustumPoints() const
{
    std::array<glm::vec3, 8> v;

    v[0] = IntersectPlanes(viewFrustum.plane[Near], viewFrustum.plane[Left], viewFrustum.plane[Bottom]); //Near bottom left
    v[1] = IntersectPlanes(viewFrustum.plane[Near], viewFrustum.plane[Left], viewFrustum.plane[Top]); //Near top left
    v[2] = IntersectPlanes(viewFrustum.plane[Near], viewFrustum.plane[Right], viewFrustum.plane[Top]); //Near top right
    v[3] = IntersectPlanes(viewFrustum.plane[Near], viewFrustum.plane[Right], viewFrustum.plane[Bottom]); //Near bottom right
    v[4] = IntersectPlanes(viewFrustum.plane[Far], viewFrustum.plane[Left], viewFrustum.plane[Bottom]); //Far bottom left
    v[5] = IntersectPlanes(viewFrustum.plane[Far], viewFrustum.plane[Left], viewFrustum.plane[Top]); //Far top left
    v[6] = IntersectPlanes(viewFrustum.plane[Far], viewFrustum.plane[Right], viewFrustum.plane[Top]); //Far top right
    v[7] = IntersectPlanes(viewFrustum.plane[Far], viewFrustum.plane[Right], viewFrustum.plane[Bottom]); //Far bottom right

    return v;
}

void CameraComponent::CaluclateViewMatrix()
{
    const glm::vec3 eyePosition = GetOwner().GetWorldPosition();
    const glm::vec3 rightDirection = GetOwner().GetRightDirection(); //s
    const glm::vec3 upDirection = GetOwner().GetUpDirection(); //u
    const glm::vec3 eyeDirection = GetOwner().GetForwardDirection(); //f
    
    //const glm::vec3 f = glm::normalize(glm::vec3(0.0f, 0.0f, 0.0f) - glm::vec3(2.0f, 2.0f, 2.0f)); //f

    const float x = -glm::dot(rightDirection, eyePosition);
    const float y = -glm::dot(upDirection, eyePosition);
    const float z = -glm::dot(eyeDirection, eyePosition);

    view = glm::mat4(1.0f);
    view[0][0] = rightDirection.x;
    view[1][0] = rightDirection.y;
    view[2][0] = rightDirection.z;
    view[3][0] = x;

    view[0][1] = upDirection.x;
    view[1][1] = upDirection.y;
    view[2][1] = upDirection.z;
    view[3][1] = y;

    view[0][2] = eyeDirection.x;
    view[1][2] = eyeDirection.y;
    view[2][2] = eyeDirection.z;
    view[3][2] = z;
}

glm::vec4 NormalizePlane(glm::vec4 p)
{
    float mag = glm::length(glm::vec3(p));

    return p / mag;
}

void CameraComponent::CalculateViewFrustum()
{
    // x, y, z, and w represent A, B, C and D in the plane equation
    // where ABC are the xyz of the planes normal, and D is the plane constant
    glm::mat4 p = projection;
    p[1][1] *= -1.0f;

    glm::mat4 viewProjection = p * view;

    viewProjection = glm::transpose(viewProjection);

    Left = 0;
    Right = 1;
    Top = 3;
    Bottom = 2;
    Near = 4;
    Far = 5;

    // Left Frustum Plane
    // Add first column of the matrix to the fourth column
    viewFrustum.plane[0] = viewProjection[3] + viewProjection[0];

    // Right Frustum Plane
    // Subtract first column of matrix from the fourth column
    viewFrustum.plane[1] = viewProjection[3] - viewProjection[0];

    // Bottom Frustum Plane
    // Add second column of the matrix to the fourth column
    viewFrustum.plane[3] = viewProjection[3] + viewProjection[1];

    // Top Frustum Plane
    // Subtract second column of matrix from the fourth column
    viewFrustum.plane[2] = viewProjection[3] - viewProjection[1];

    // Near Frustum Plane
    // We could add the third column to the fourth column to get the near plane,
    // but we don't have to do this because the third column IS the near plane
    viewFrustum.plane[4] = viewProjection[2];

    // Far Frustum Plane
    // Subtract third column of matrix from the fourth column
    viewFrustum.plane[5] = viewProjection[3] - viewProjection[2];

    for (size_t index = 0; index < viewFrustum.plane.size(); index++)
    {
        viewFrustum.plane[index] = NormalizePlane(viewFrustum.plane[index]);
    }
}
