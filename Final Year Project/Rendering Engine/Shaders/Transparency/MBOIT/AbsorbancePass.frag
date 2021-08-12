#version 450
#extension GL_ARB_separate_shader_objects : enable

#include "MBOITCommon.glsl"
#include "../../Common/LightingCommon.glsl"

layout (location = 0) out float b0; //Total Absorbance
layout (location = 1) out vec4 b1234; //Absorbance
layout (location = 2) out vec4 b56; //Absorbance

// Material information
layout(set = 0, location = 0) in vec3 inPosition;
layout(set = 0, location = 1) in vec2 inTexCoord;
layout(set = 0, location = 2) in vec3 inNormal;
layout(set = 0, location = 3) in vec3 inTangent;
layout(set = 0, location = 4) in vec3 inBiTangent;
layout(set = 0, location = 5) in vec3 inScreenPosition;

layout(set = 1, binding = 1) uniform PBRMaterialParameters
{
    vec4 albedo;
	float metalness;
	float roughness;
} material;

layout(push_constant) uniform PushConstantObject
{
	float lnDepthMin;
    float lnDepthMax;
    float pad;
    float pad1;      

    ivec3 clusterCounts;
    int clusterSizeX;

    float zNear;
    float zFar;
    float scale;
    float bias;
} pushConstants;

struct Cluster
{
    vec4 minVertex; //W component is if the cluster is active or not
    vec4 maxVertex;
};

layout(set = 2, binding = 2) buffer writeonly Clusters
{
    Cluster cluster[];
} clusters;

void main()
{
    float depth = WarpDepth(-inScreenPosition.z, pushConstants.lnDepthMin, pushConstants.lnDepthMax);

    float transmittance = 1.0 - material.albedo.a;
    GenerateMoments(depth, transmittance, b0, b1234, b56);
    
    float fragDepth = gl_FragCoord.z;
    uint zTile      = uint(max(log2(LinearDepth(fragDepth, pushConstants.zNear, pushConstants.zFar)) * pushConstants.scale + pushConstants.bias, 0.0));
    uvec3 tiles     = uvec3( uvec2( gl_FragCoord.xy / pushConstants.clusterSizeX ), zTile);
    uint tileIndex  = tiles.x +
                        pushConstants.clusterCounts.x * tiles.y +
                        (pushConstants.clusterCounts.x * pushConstants.clusterCounts.y) * tiles.z;  

    clusters.cluster[tileIndex].minVertex.w = 1.0;
}