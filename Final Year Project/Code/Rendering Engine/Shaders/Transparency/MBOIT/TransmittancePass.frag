#version 450
#extension GL_ARB_separate_shader_objects : enable

#include "MBOITCommon.glsl"

#include "../../pbr.glsl"

#include "../../Common/LightingCommon.glsl"

// Material information
layout(set = 0, location = 0) in vec3 inPosition;
layout(set = 0, location = 1) in vec2 inTexCoord;
layout(set = 0, location = 2) in vec3 inNormal;
layout(set = 0, location = 3) in vec3 inTangent;
layout(set = 0, location = 4) in vec3 inBiTangent;
layout(set = 0, location = 5) in vec3 inScreenPosition;

layout(set = 1, binding = 1) uniform  PerFrameInfo
{
	vec3 camPosition;
    float pad;
	
	float lnDepthMin;
    float lnDepthMax;
    float pad1;
    float pad2;

} perFrameInfo;

#define MAX_POINT_LIGHT_PER_TILE 1024
struct LightVisiblity
{
	uint count;
	uint lightindices[MAX_POINT_LIGHT_PER_TILE];
};

#define MAX_LIGHT_PER_PASS 10000
layout(set = 1, binding = 2) uniform  PointLights
{
    int count;
	int pad1;
	int pad2;
	int pad3;
	PointLightData pointLights[MAX_LIGHT_PER_PASS];
} pointLights;

layout(set = 1, binding = 3) buffer readonly ClusterLights
{
    LightVisiblity lightVisiblity[];
} clusterLights;

layout(set = 1, binding = 4) uniform sampler2D b0Texture;
layout(set = 1, binding = 5) uniform sampler2D b1234Texture;
layout(set = 1, binding = 6) uniform sampler2D b56Texture;

layout(set = 1, binding = 7) uniform samplerCube radianceMap;
layout(set = 1, binding = 8) uniform samplerCube irradianceMap;
layout(set = 1, binding = 9) uniform sampler2D integrationMap;

layout(set = 2, binding = 10) uniform PBRMaterialParameters
{
    vec4 albedo;
	float metalness;
	float roughness;
} perMaterialInfo;

layout(push_constant) uniform PushConstantObject
{
	float moment_bias;
    float overestimation;
    float screenWidth;
    float screenHeight; 

    ivec3 clusterCounts;
    int clusterSizeX;

    float zNear;
    float zFar;
    float scale;
    float bias;
} pushConstants;

layout (location = 0) out vec4 outColor;

float GetTransmittanceAtDepth()
{
    float depth =  WarpDepth(-inScreenPosition.z, perFrameInfo.lnDepthMin, perFrameInfo.lnDepthMax);

    vec2 addr2D = vec2(gl_FragCoord.xy) / vec2(pushConstants.screenWidth, pushConstants.screenHeight);
    float transmittance_at_depth = 1.0;
    float total_transmittance = 1.0; 

    float b0 = texture(b0Texture, addr2D).r;
    vec4 b_1234 = texture(b1234Texture, addr2D);
    vec4 b_56 = texture(b56Texture, addr2D);

    ResolveMoments(transmittance_at_depth, total_transmittance, depth, pushConstants.moment_bias, pushConstants.overestimation, b0, b_1234, b_56);

    return transmittance_at_depth;
}

float linearDepth(float depthSample)
{
    float depthRange = 2.0 * depthSample - 1.0;
    // Near... Far... wherever you are...
    float linear = 2.0 * pushConstants.zNear * pushConstants.zFar / (pushConstants.zFar + pushConstants.zNear - depthRange * (pushConstants.zFar - pushConstants.zNear));
    return linear;
}

vec3 ShadeMaterial(PBRMaterial material)
{
    vec3 N = normalize(inNormal);
    vec3 V = normalize(perFrameInfo.camPosition  - inPosition);

    float fragDepth = gl_FragCoord.z;
    
    vec3 directLight = vec3(0.0f);

    uint zTile     = uint(max(log2(LinearDepth(fragDepth, pushConstants.zNear, pushConstants.zFar)) * pushConstants.scale + pushConstants.bias, 0.0));
    uvec3 tiles    = uvec3( uvec2( gl_FragCoord.xy / pushConstants.clusterSizeX ), zTile);
    uint tileIndex = tiles.x +
                     pushConstants.clusterCounts.x * tiles.y +
                     (pushConstants.clusterCounts.x * pushConstants.clusterCounts.y) * tiles.z;  

    uint tile_light_num = clusterLights.lightVisiblity[tileIndex].count;
    for(int index = 0; index < tile_light_num; index++)
    {
        PointLightData pointLight = pointLights.pointLights[clusterLights.lightVisiblity[tileIndex].lightindices[index]];
        vec3 lightVector = pointLight.position - inPosition;
        float lightDistance = max(length(lightVector), 0.00001);

        LightInfo lightInfo;
        lightInfo.direction = lightVector / lightDistance;
        lightInfo.colour = pointLight.lightColour;

        lightInfo.falloff = pointLight.lightIntensity / (lightDistance * lightDistance);

        vec3 L = normalize(lightInfo.direction);
        
        float NdotL = max(dot(N, L), 0.000001f);     

        directLight += CalculateDirectLighting(material, V, L, N) * lightInfo.colour * lightInfo.falloff * NdotL;	
    }

    vec3 indirectLight = CalculateIndirectLighting(material, V, N, radianceMap, irradianceMap, integrationMap);

    return directLight + indirectLight;
}

void main()
{
    float transmittance_at_depth = GetTransmittanceAtDepth();

    PBRMaterial material;
    material.albedo = vec3(perMaterialInfo.albedo);
    material.metalness = perMaterialInfo.metalness;
    material.roughness = perMaterialInfo.roughness;

    vec3 finalLight = ShadeMaterial(material);
    
    outColor = vec4(finalLight * perMaterialInfo.albedo.a * transmittance_at_depth, perMaterialInfo.albedo.a * transmittance_at_depth);
}