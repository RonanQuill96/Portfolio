#include "../pbr.glsl"

struct GBufferInfo
{
    vec3 position;
    vec3 normal;
    vec4 albedo;
    float metalness;
    float roughness;
};

vec4 GenericLightingPass(GBufferInfo gBufferInfo, LightInfo lightInformation, vec3 cameraPosition)
{
    vec3 N = normalize(gBufferInfo.normal);
	vec3 V = normalize(cameraPosition - gBufferInfo.position);
    vec3 L = normalize(lightInformation.direction);
    
    float NdotL = max(dot(N, L), 0.000001f);  

    PBRMaterial material;
    material.albedo = gBufferInfo.albedo.xyz;
    material.metalness = gBufferInfo.metalness;
    material.roughness = gBufferInfo.roughness;

    vec3 directLight = CalculateDirectLighting(material, V, L, N) * lightInformation.colour * lightInformation.falloff * NdotL;

    // Tone Mapping
    //directLight = directLight / (directLight + vec3(1.0));

    // Gamma correction
    //directLight = GammaCorrection(directLight);

    return vec4(directLight, gBufferInfo.albedo.w);
}