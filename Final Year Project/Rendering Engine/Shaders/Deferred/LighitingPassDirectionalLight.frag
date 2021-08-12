#version 450
#extension GL_ARB_separate_shader_objects : enable

#include "LightingPassGeneric.glsl"

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

layout(set = 1, binding = 5) uniform DirectionalLight
{
	vec3 lightDirection;
    float pad1;
    vec3 lightColour;
	float intensity;
} directionalLight;


layout(set = 0, location = 0) out vec4 outColor;

void main()
{
	float depth = texture(depth, fragTexCoord).r;

    vec2 metalicRoughness = texture(metalicRoughness, fragTexCoord).rg;

    GBufferInfo gBufferInfo;
    gBufferInfo.position = ScreenToWorld(fragTexCoord, depth, perLightingPassInfo.InverseProjection, perLightingPassInfo.InverseView).xyz;
    gBufferInfo.normal=  texture(normals, fragTexCoord).rgb;
    gBufferInfo.albedo = texture(albedo, fragTexCoord);
    gBufferInfo.metalness = metalicRoughness.x;
    gBufferInfo.roughness = metalicRoughness.y;

    LightInfo lightInfo;
    lightInfo.direction = -directionalLight.lightDirection;
    lightInfo.colour = directionalLight.lightColour;
    lightInfo.falloff = 1.0f;


    outColor = GenericLightingPass(gBufferInfo, lightInfo, perLightingPassInfo.camPosition);
}