#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, location = 0) in vec3 inPosition;
layout(set = 0, location = 1) in vec2 inTexCoord;
layout(set = 0, location = 2) in vec3 inNormal;
layout(set = 0, location = 3) in vec3 inTangent;
layout(set = 0, location = 4) in vec3 inBiTangent;

layout (location = 0) out vec4 outNormal;
layout (location = 1) out vec4 outAlbedo;
layout (location = 2) out vec4 outMetalicRoughness;

layout(set = 1, binding = 1) uniform PBRMaterialParameters{
    vec4 albedo;
	float metalness;
	float roughness;
} material;

void main()
{
    outNormal = vec4(inNormal, 1.0);
	outAlbedo = material.albedo;
	outMetalicRoughness = vec4(material.metalness, material.roughness, 0.0, 0.0);
}