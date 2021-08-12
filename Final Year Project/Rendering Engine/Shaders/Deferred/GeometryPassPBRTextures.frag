#version 450
#extension GL_ARB_separate_shader_objects : enable

#include "../common.glsl"

layout(set = 0, location = 0) in vec3 inPosition;
layout(set = 0, location = 1) in vec2 inTexCoord;
layout(set = 0, location = 2) in vec3 inNormal;
layout(set = 0, location = 3) in vec3 inTangent;
layout(set = 0, location = 4) in vec3 inBiTangent;

layout (location = 0) out vec4 outNormal;
layout (location = 1) out vec4 outAlbedo;
layout (location = 2) out vec4 outMetalicRoughness;

layout(set = 1, binding = 1) uniform sampler2D albedo;
layout(set = 1, binding = 2) uniform sampler2D normals;
layout(set = 1, binding = 3) uniform sampler2D metalicRoughness;

layout(early_fragment_tests) in; // for early depth test

void main()
{
	vec3 normalWS = normalize(NormalSampleToWorldSpace(texture(normals, inTexCoord).rgb, inNormal, inTangent, inBiTangent));

	vec4 albedo = texture(albedo, inTexCoord);

    vec2 metalicRoughness = texture(metalicRoughness, inTexCoord).rg;
    outNormal = vec4(normalWS, 1.0);
	outAlbedo = albedo;
	outMetalicRoughness = vec4(metalicRoughness, 0.0, 0.0);
}