#version 450
#extension GL_ARB_separate_shader_objects : enable

#include "../pbr.glsl"

layout(set = 0, location = 0) in vec3 position;
layout(set = 0, location = 1) in vec2 fragTexCoord;

layout(set = 0, binding = 0) uniform PerLightingPassInfo
{
	vec3 camPosition;
    float pad;

	mat4 InverseProjection;
	mat4 InverseView;
} perLightingPassInfo;

layout(set = 0, binding = 1) uniform sampler2D normals;
layout(set = 0, binding = 2) uniform sampler2D albedo;
layout(set = 0, binding = 3) uniform sampler2D metalicRoughness;
layout(set = 0, binding = 4) uniform sampler2D depth;

layout(set = 1, binding = 5) uniform samplerCube radianceMap;
layout(set = 1, binding = 6) uniform samplerCube irradianceMap;
layout(set = 1, binding = 7) uniform sampler2D integrationMap;

layout(set = 2, binding = 8) uniform sampler2D ambientOcclusionMap;

layout(set = 0, location = 0) out vec4 outColor;

void main()
{
	float depth = texture(depth, fragTexCoord).r;
	vec3 position = ScreenToWorld(fragTexCoord, depth, perLightingPassInfo.InverseProjection, perLightingPassInfo.InverseView).xyz;

    float abmientOcclusion = texture(ambientOcclusionMap, fragTexCoord).r;

    vec3 normal = texture(normals, fragTexCoord).rgb;
    vec4 albedo = texture(albedo, fragTexCoord);
    vec2 metalicRoughness = texture(metalicRoughness, fragTexCoord).rg;
    	
    PBRMaterial material;
    material.albedo = albedo.xyz;
    material.metalness = metalicRoughness.x;
    material.roughness = metalicRoughness.y;

    vec3 N = normalize(normal);
	vec3 V = normalize(perLightingPassInfo.camPosition - position);

    vec3 indirectLight = CalculateIndirectLighting(material, V, N, radianceMap, irradianceMap, integrationMap) * abmientOcclusion;

    vec3 finalLighit = indirectLight;

    // Tone Mapping
    //finalLighit = finalLighit / (finalLighit + vec3(1.0));
    // Gamma correction
    //finalLighit = GammaCorrection(finalLighit);

    outColor = vec4(finalLighit, 0.0);

}