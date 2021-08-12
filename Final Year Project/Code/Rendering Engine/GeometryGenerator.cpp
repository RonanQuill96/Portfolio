#include "GeometryGenerator.h"

#include "RenderingEngine.h"

#include <unordered_map>

bool GeometryGenerator::GenerateSphere(RenderingEngine& renderingEngine, const float radius, const unsigned int sliceCount, const unsigned int stackCount, GameObject& gameObject, glm::vec4 albido, float metalness, float roughness, MaterialRenderMode materialRenderMode)
{
	const GraphicsAPI& graphicsAPI = renderingEngine.GetGraphicsAPI();
	CommandPool& commandPool = renderingEngine.GetCommandPool();

	return GeometryGenerator::GenerateSphere(graphicsAPI, commandPool, radius, sliceCount, stackCount, gameObject, albido, metalness, roughness, materialRenderMode);
}

bool GeometryGenerator::GenerateSphere(const GraphicsAPI& graphicsAPI, CommandPool& commandPool, const float radius, const unsigned int sliceCount, const unsigned int stackCount, GameObject& gameObject, glm::vec4 albido, float metalness, float roughness, MaterialRenderMode materialRenderMode)
{
	std::shared_ptr<Mesh> mesh = std::make_shared<Mesh>();

	//
	// Compute the vertices stating at the top pole and moving down the stacks.
	//

	// Poles: note that there will be texture coordinate distortion as there is
	// not a unique point on the texture map to assign to the pole when mapping
	// a rectangular texture onto a sphere.
	Vertex topVertex({ 0.0f, +radius, 0.0f }, { 0.0f, +1.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 0.0f });
	Vertex bottomVertex({ 0.0f, -radius, 0.0f }, { 0.0f, -1.0f }, { 0.0f, -1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f });

	mesh->vertices.push_back(topVertex);

	const float phiStep = glm::pi<float>() / stackCount;
	const float thetaStep = 2.0f * glm::pi<float>() / sliceCount;

	// Compute vertices for each stack ring (do not count the poles as rings).
	for (unsigned int i = 1; i <= stackCount - 1; ++i)
	{
		const float phi = i * phiStep;

		// vertices of ring.
		for (unsigned int j = 0; j <= sliceCount; ++j)
		{
			float theta = j * thetaStep;

			Vertex v;

			// spherical to cartesian
			v.pos.x = radius * sinf(phi) * cosf(theta);
			v.pos.y = radius * cosf(phi);
			v.pos.z = radius * sinf(phi) * sinf(theta);

			// Partial derivative of P with respect to theta
			v.tangent.x = -radius * sinf(phi) * sinf(theta);
			v.tangent.y = 0.0f;
			v.tangent.z = +radius * sinf(phi) * cosf(theta);
			glm::normalize(v.tangent);

			v.normal = v.pos;
			glm::normalize(v.normal);

			v.texCoord.x = theta / glm::pi<float>();
			v.texCoord.y = phi / glm::pi<float>();

			mesh->vertices.push_back(v);
		}
	}

	mesh->vertices.push_back(bottomVertex);

	//
	// Compute indices for top stack.  The top stack was written first to the vertex buffer
	// and connects the top pole to the first ring.
	//

	for (unsigned int i = 1; i <= sliceCount; ++i)
	{
		mesh->indices.push_back(0);
		mesh->indices.push_back(i + 1);
		mesh->indices.push_back(i);
	}

	//
	// Compute indices for inner stacks (not connected to poles).
	//

	// Offset the indices to the index of the first vertex in the first ring.
	// This is just skipping the top pole vertex.
	unsigned int baseIndex = 1;
	const unsigned int ringVertexCount = sliceCount + 1;
	for (unsigned int i = 0; i < stackCount - 2; ++i)
	{
		for (unsigned int j = 0; j < sliceCount; ++j)
		{
			mesh->indices.push_back(baseIndex + i * ringVertexCount + j);
			mesh->indices.push_back(baseIndex + i * ringVertexCount + j + 1);
			mesh->indices.push_back(baseIndex + (i + 1) * ringVertexCount + j);

			mesh->indices.push_back(baseIndex + (i + 1) * ringVertexCount + j);
			mesh->indices.push_back(baseIndex + i * ringVertexCount + j + 1);
			mesh->indices.push_back(baseIndex + (i + 1) * ringVertexCount + j + 1);
		}
	}

	//
	// Compute indices for bottom stack.  The bottom stack was written last to the vertex buffer
	// and connects the bottom pole to the bottom ring.
	//

	// South pole vertex was added last.
	const size_t southPoleIndex = mesh->vertices.size() - 1;

	// Offset the indices to the index of the first vertex in the last ring.
	baseIndex = southPoleIndex - ringVertexCount;

	for (size_t i = 0; i < sliceCount; ++i)
	{
		mesh->indices.push_back(southPoleIndex);
		mesh->indices.push_back(baseIndex + i);
		mesh->indices.push_back(baseIndex + i + 1);
	}

	mesh->vertexBuffer.Create(graphicsAPI, commandPool, mesh->vertices);
	mesh->indexBuffer.Create(graphicsAPI, commandPool, mesh->indices);

	mesh->polyCount = mesh->indices.size() / 3;

	mesh->aabb = AABB::CreateAABB(mesh->vertices);

	MeshComponent* meshComponent = gameObject.CreateComponent<MeshComponent>();
	meshComponent->perObjectBuffer.Create(graphicsAPI);

	meshComponent->mesh = mesh;

	std::shared_ptr<PBRParameterMaterial> material = std::make_shared<PBRParameterMaterial>();
	material->parameters.albido = albido;
	material->parameters.metalness = metalness;
	material->parameters.roughness = roughness;
	material->materialRenderMode = materialRenderMode;
	material->uniformBuffer.Create(graphicsAPI);
	material->uniformBuffer.Update(material->parameters);

	MeshMaterial meshMaterial;
	meshMaterial.material = material;
	meshMaterial.startIndex = 0;
	meshMaterial.indexCount = mesh->indices.size();
	
	meshComponent->mesh->materials.push_back(meshMaterial);

	return true;
}

bool GeometryGenerator::GenerateCube(RenderingEngine& renderingEngine, const float width, const float height, const float depth, GameObject& gameObject, glm::vec4 albido, float metalness, float roughness, MaterialRenderMode materialRenderMode)
{
	std::shared_ptr<Mesh> mesh = std::make_shared<Mesh>();

	mesh->vertices =
	{
		Vertex({0.5f * width, -0.5f * height, 0.5f * depth }, { 0.0f, 1.0f }, { 0.0f, -1.0f, 0.0f }),
		Vertex({-0.5f * width, -0.5f * height, -0.5f * depth }, { 1.0f, 0.0f }, { 0.0f, -1.0f, 0.0f}),
		Vertex({-0.5f * width, -0.5f * height, 0.5f * depth }, { 0.0f, 0.0f }, { 0.0f, -1.0f, 0.0f}),
		Vertex({-0.5f * width, 0.5f * height, 0.5f * depth }, { 1.0f, 1.0f }, { 0.0f, 1.0f, 0.0f}),
		Vertex({0.5f * width, 0.5f * height, -0.5f * depth }, { 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f}),
		Vertex({0.5f * width, 0.5f * height, 0.5f * depth }, { 0.0f, 1.0f }, { 0.0f, 1.0f, 0.0f}),
		Vertex({0.5f * width, 0.5f * height, 0.5f * depth }, { 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f}),
		Vertex({0.5f * width, -0.5f * height, -0.5f * depth }, { 1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f}),
		Vertex({0.5f * width, -0.5f * height, 0.5f * depth }, { 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f}),
		Vertex({0.5f * width, 0.5f * height, -0.5f * depth }, { 0.0f, 1.0f }, { 0.0f, 0.0f, -1.0f}),
		Vertex({-0.5f * width, -0.5f * height, -0.5f * depth }, { 1.0f, 0.0f }, { 0.0f, 0.0f, -1.0f}),
		Vertex({0.5f * width, -0.5f * height, -0.5f * depth }, { 0.0f, 0.0f }, { 0.0f, 0.0f, -1.0f}),
		Vertex({-0.5f * width, -0.5f * height, -0.5f * depth }, { 1.0f, 0.0f }, { -1.0f, 0.0f, 0.0f}),
		Vertex({-0.5f * width, 0.5f * height, 0.5f * depth }, { 0.0f, 1.0f }, { -1.0f, 0.0f, 0.0f}),
		Vertex({-0.5f * width, -0.5f * height, 0.5f * depth }, { 1.0f, 1.0f }, { -1.0f, 0.0f, 0.0f}),
		Vertex({0.5f * width, -0.5f * height, 0.5f * depth }, { 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f}),
		Vertex({-0.5f * width, 0.5f * height, 0.5f * depth }, { 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f}),
		Vertex({0.5f * width, 0.5f * height, 0.5f * depth }, { 0.0f, 1.0f }, { 0.0f, 0.0f, 1.0f}),
		Vertex({0.5f * width, -0.5f * height, -0.5f * depth }, { 1.0f, 1.0f }, { 0.0f, -1.0f, 0.0f}),
		Vertex({-0.5f * width, 0.5f * height, -0.5f * depth }, { 1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f}),
		Vertex({0.5f * width, 0.5f * height, -0.5f * depth }, { 1.0f, 1.0f }, { 1.0f, 0.0f, -0.0f}),
		Vertex({-0.5f * width, 0.5f * height, -0.5f * depth }, { 1.0f, 1.0f }, { 0.0f, 0.0f, -1.0f}),
		Vertex({-0.5f * width, 0.5f * height, -0.5f * depth }, { 0.0f, 0.0f }, { -1.0f, 0.0f, 0.0f}),
		Vertex({-0.5f * width, -0.5f * height, 0.5f * depth }, { 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f})
	};

	mesh->indices =
	{
		0, 2, 1,
		3, 5, 4,
		6, 8, 7,
		9, 11, 10,
		12, 14, 13,
		15, 17, 16,
		0, 1, 18,
		3, 4, 19,
		6, 7, 20,
		9, 10, 21,
		12, 13, 22,
		15, 16, 23,
	};

	GenerateTangents(mesh->vertices, mesh->indices);

	const GraphicsAPI& graphicsAPI = renderingEngine.GetGraphicsAPI();
	CommandPool& commandPool = renderingEngine.GetCommandPool();

	mesh->vertexBuffer.Create(graphicsAPI, commandPool, mesh->vertices);
	mesh->indexBuffer.Create(graphicsAPI, commandPool, mesh->indices);

	mesh->polyCount = mesh->indices.size() / 3;

	mesh->aabb = AABB::CreateAABB(mesh->vertices);

	MeshComponent* meshComponent = gameObject.CreateComponent<MeshComponent>();
	meshComponent->perObjectBuffer.Create(graphicsAPI);

	meshComponent->mesh = mesh;

	std::shared_ptr<PBRParameterMaterial> material = std::make_shared<PBRParameterMaterial>();
	material->parameters.albido = albido;
	material->parameters.metalness = metalness;
	material->parameters.roughness = roughness;
	material->materialRenderMode = materialRenderMode;
	material->uniformBuffer.Create(graphicsAPI);
	material->uniformBuffer.Update(material->parameters);

	MeshMaterial meshMaterial;
	meshMaterial.material = material;
	meshMaterial.startIndex = 0;
	meshMaterial.indexCount = mesh->indices.size();
	//meshMaterial.descriptorSet = renderingEngine.GetBasicRenderPass().GetPBRParametersPipeline().CreateDescriptorSet(graphicsAPI.GetLogicalDevice(), descriptorPool, mesh->perObjectBuffer, material->uniformBuffer);

	meshComponent->mesh->materials.push_back(meshMaterial);

	return true;
}

void GeometryGenerator::GenerateTangents(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices)
{
	std::unordered_map<size_t, size_t> facesUsedPerVertex;
	std::unordered_map<size_t, glm::vec3> tangentSums;

	std::vector<glm::vec3> tempTangent;

	glm::vec3 tangent;

	glm::vec2 texCoord1;
	glm::vec2 texCoord2;

	glm::vec3 edge1;
	glm::vec3 edge2;

	size_t polyCount = indices.size() / 3;

	for (unsigned int i = 0; i < polyCount; ++i)
	{
		//Get the vector describing one edge of our triangle (edge 0,2)
		edge1 = vertices[indices[(i * 3)]].pos - vertices[indices[(i * 3) + 2]].pos;

		//Get the vector describing another edge of our triangle (edge 2,1)
		edge2 = vertices[indices[(i * 3) + 2]].pos - vertices[indices[(i * 3) + 1]].pos;

		//Find first texture coordinate edge 2d vector
		texCoord1 = vertices[indices[(i * 3)]].texCoord - vertices[indices[(i * 3) + 2]].texCoord;

		//Find second texture coordinate edge 2d vector
		texCoord2 = vertices[indices[(i * 3) + 2]].texCoord - vertices[indices[(i * 3) + 1]].texCoord;

		float texCooardsCrossed = texCoord1.x * texCoord2.y - texCoord2.x * texCoord1.y;

		//Find tangent using both tex coord edges and position edges
		tangent = (edge1 * texCoord1.y - edge2 * texCoord2.y) * (1.0f / texCooardsCrossed);

		tempTangent.push_back(tangent);
	}

	for (size_t i = 0; i < indices.size(); i += 3)
	{
		unsigned int vertex = indices[i];

		if (facesUsedPerVertex.find(vertex) == facesUsedPerVertex.end())
		{
			facesUsedPerVertex[vertex] = 1;
			tangentSums[vertex] = tempTangent[i / 3];
		}
		else
		{
			facesUsedPerVertex[vertex]++;
			tangentSums[vertex] += tempTangent[i / 3];
		}
	}

	for (auto& vertex : facesUsedPerVertex)
	{
		glm::vec3 tangentSum = tangentSums[vertex.first] / static_cast<float>(vertex.second);

		//Normalize the normalSum vector and tangent
		glm::normalize(tangentSum);

		//Store the normal and tangent in our current vertex
		vertices[vertex.first].tangent = tangentSum;
	}
}
