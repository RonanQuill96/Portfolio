#pragma once

#include "IndexBuffer.h"
#include "VertexBuffer.h"
#include "Vertex.h"
#include "UniformBuffer.h"

#include "MeshMaterial.h"

#include "AABB.h"

#include <glm/glm.hpp>

#include <string>
#include <vector>

struct PerObjectBufferInfo
{
	glm::mat4 wvm;
	glm::mat4 worldMatrix;
	glm::mat4 worldMatrixInvTrans;
	glm::mat4 view;
};


struct Mesh
{
	std::string meshName = "";

	unsigned int polyCount = 0;
	std::vector<Vertex> vertices = {};
	std::vector<unsigned int> indices = {};

	std::vector<MeshMaterial> materials;

	VertexBuffer vertexBuffer;
	IndexBuffer indexBuffer;

	AABB aabb;
};