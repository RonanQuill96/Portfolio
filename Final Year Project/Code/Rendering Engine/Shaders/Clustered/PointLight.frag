#version 450
#extension GL_ARB_separate_shader_objects : enable

#include "../Deferred/LightingPassGeneric.glsl"

#include "../Common/LightingCommon.glsl"

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

#define MAX_LIGHT_PER_PASS 10000
layout(set = 1, binding = 5) uniform  PointLights
{
    int count;
	int count4;
	int count3;
	int count2;
	PointLightData pointLights[MAX_LIGHT_PER_PASS];
} pointLights;

#define MAX_POINT_LIGHT_PER_CLUSTER 1024
struct LightVisiblity
{
	uint count;
	uint lightindices[MAX_POINT_LIGHT_PER_CLUSTER];
};

layout(set = 1, binding = 6) buffer readonly ClusterLights
{
    LightVisiblity lightVisiblity[];
} clusterLights;

layout(set = 0, location = 0) out vec4 outColor;

layout(push_constant) uniform PushConstantObject
{
	ivec3 clusterCounts;
    int clusterSizeX;

    mat4 InverseViewProjection;

    vec3 cameraPosition;
	float pad;

    float zNear;
    float zFar;
    float scale;
    float bias;
} push_constants;

layout(early_fragment_tests) in; // for early depth test

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

    vec4 colour = vec4(0);

    uint zTile     = uint(max(log2(LinearDepth(depth, push_constants.zNear, push_constants.zFar)) * push_constants.scale + push_constants.bias, 0.0));
    uvec3 tiles    = uvec3( uvec2( gl_FragCoord.xy / push_constants.clusterSizeX ), zTile);
    uint tileIndex = tiles.x +
                     push_constants.clusterCounts.x * tiles.y +
                     (push_constants.clusterCounts.x * push_constants.clusterCounts.y) * tiles.z;  

    uint tile_light_num = clusterLights.lightVisiblity[tileIndex].count;
    for(int index = 0; index < tile_light_num; index++)
    {
        PointLightData pointLight = pointLights.pointLights[clusterLights.lightVisiblity[tileIndex].lightindices[index]];
       
        vec3 lightVector = pointLight.position - gBufferInfo.position;
        float lightDistance = max(length(lightVector), 0.00001);

        LightInfo lightInfo;
        lightInfo.direction = lightVector / lightDistance;
        lightInfo.colour = pointLight.lightColour;

        lightInfo.falloff = pointLight.lightIntensity / (lightDistance * lightDistance);

        colour = colour + GenericLightingPass(gBufferInfo, lightInfo, perLightingPassInfo.camPosition);
    }

    outColor = vec4(vec3(colour), 1.0);
}