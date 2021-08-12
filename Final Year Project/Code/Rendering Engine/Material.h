#pragma once

#include "VulkanIncludes.h"

#include "UniformBuffer.h"

#include "ImageData.h"

#include "ImageUtils.h"

enum class MaterialType : uint32_t
{
	PBRTextures,
	PBRParameters
};

enum class MaterialRenderMode : uint32_t
{
	Opaque,
	Transparent,
	AlphaCutout
};

struct Material
{
	std::string materialName = "";
	MaterialType materialType;
	MaterialRenderMode materialRenderMode = MaterialRenderMode::Opaque;
protected:
	Material(MaterialType pMaterialType) : materialType(pMaterialType) {}
};

struct PBRParameters
{
	glm::vec4 albido;
	float metalness = 0.0f;
	float roughness = 0.0f;
	float pad;
	float pad2;
};

struct PBRParameterMaterial : public Material
{
	PBRParameterMaterial() : Material(MaterialType::PBRParameters) {}

	PBRParameters parameters;

	UniformBuffer<PBRParameters> uniformBuffer;
};

struct PBRTextureMaterial : public Material
{
	PBRTextureMaterial() : Material(MaterialType::PBRTextures) {}

	Texture albedo;
	Texture metalicRougness;
	Texture normals;
};

