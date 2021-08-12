#pragma once

#include <vector>

#include <glm/glm.hpp>

struct AABB
{
	AABB() = default;

	AABB(glm::vec3 min, glm::vec3 max) : minVertex(min), maxVertex(max)
	{}

	template<class DataType>
	static AABB CreateAABB(const std::vector<DataType>& vertices)
	{
		AABB aabb;

		for (const auto& vertex : vertices)
		{
			aabb.minVertex.x = std::min(aabb.minVertex.x, vertex.pos.x);	// Find smallest x value in model
			aabb.minVertex.y = std::min(aabb.minVertex.y, vertex.pos.y);	// Find smallest y value in model
			aabb.minVertex.z = std::min(aabb.minVertex.z, vertex.pos.z);	// Find smallest z value in model

			aabb.maxVertex.x = std::max(aabb.maxVertex.x, vertex.pos.x);	// Find largest x value in model
			aabb.maxVertex.y = std::max(aabb.maxVertex.y, vertex.pos.y);	// Find largest y value in model
			aabb.maxVertex.z = std::max(aabb.maxVertex.z, vertex.pos.z);	// Find largest z value in model
		}

		return aabb;
	}

	static AABB GetMaxAABB(const AABB& first, const AABB& second) noexcept
	{
		AABB max;

		max.minVertex.x = std::min(first.minVertex.x, second.minVertex.x);	// Find smallest x value in model
		max.minVertex.y = std::min(first.minVertex.y, second.minVertex.y);	// Find smallest y value in model
		max.minVertex.z = std::min(first.minVertex.z, second.minVertex.z);	// Find smallest z value in model

		max.maxVertex.x = std::max(first.maxVertex.x, second.maxVertex.x);	// Find largest x value in model
		max.maxVertex.y = std::max(first.maxVertex.y, second.maxVertex.y);	// Find largest y value in model
		max.maxVertex.z = std::max(first.maxVertex.z, second.maxVertex.z);	// Find largest z value in model

		return max;
	}

	glm::vec3 GetCenter() const noexcept
	{
		return minVertex + (GetDimensions() * 0.5f);
	}

	float GetRadius() const noexcept
	{
		return glm::length(GetDimensions() * 0.5f);
	}

	glm::vec3 GetDimensions() const noexcept
	{
		return maxVertex - minVertex;
	}

	AABB Transform(const glm::mat4& matrix) const
	{
		auto transformPoint = [](const glm::mat4& mat, const glm::vec3& vec) -> glm::vec3
		{
			glm::vec4 transVec = mat * glm::vec4(vec.x, vec.y, vec.z, 1.0f);
			if (transVec.w != 1.0f)
				transVec /= transVec.w;
			return glm::vec3(transVec.x, transVec.y, transVec.z);
		};

		glm::vec3 transfromedMinVertex = transformPoint(matrix, minVertex);
		glm::vec3 transfromedMaxVertex = transformPoint(matrix, maxVertex);

		AABB transfromed;

		transfromed.minVertex.x = std::min(transfromedMinVertex.x, transfromedMaxVertex.x);
		transfromed.minVertex.y = std::min(transfromedMinVertex.y, transfromedMaxVertex.y);
		transfromed.minVertex.z = std::min(transfromedMinVertex.z, transfromedMaxVertex.z);

		transfromed.maxVertex.x = std::max(transfromedMinVertex.x, transfromedMaxVertex.x);
		transfromed.maxVertex.y = std::max(transfromedMinVertex.y, transfromedMaxVertex.y);
		transfromed.maxVertex.z = std::max(transfromedMinVertex.z, transfromedMaxVertex.z);

		return transfromed;
	}

	glm::vec3 minVertex = { std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max() };
	glm::vec3 maxVertex = { std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest() };
};
