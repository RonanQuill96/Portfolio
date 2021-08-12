#ifndef PBR_GLSL
#define PBR_GLSL

#include "common.glsl"

struct LightInfo
{
    vec3 direction;
    vec3 colour;
    float falloff;
};

struct PBRMaterial
{
    vec3 albedo;
	float metalness;
	float roughness;
};

//Common methods for IBL

vec3 ImportanceSampleGGX(vec2 Xi, float Roughness, vec3 N)
{
	float a = Roughness * Roughness; // DISNEY'S ROUGHNESS [see Burley'12 siggraph]

	float Phi = 2 * PI * Xi.x;
	float CosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a * a - 1.0) * Xi.y));
	float SinTheta = sqrt(1.0 - CosTheta * CosTheta);

	vec3 H;
	H.x = SinTheta * cos(Phi);
	H.y = SinTheta * sin(Phi);
	H.z = CosTheta;

	vec3 UpVector = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
	vec3 TangentX = normalize(cross(UpVector, N));
	vec3 TangentY = cross(N, TangentX);

	// Tangent to world space
	vec3 sampleVec = TangentX * H.x + TangentY * H.y + N * H.z;

    return normalize(sampleVec);
}

float RadicalInverse_VdC(uint bits) 
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}
// ----------------------------------------------------------------------------
vec2 Hammersley(uint i, uint N)
{
    return vec2(float(i)/float(N), RadicalInverse_VdC(i));
}  

//Lambertian
vec3 DiffuseBRDF(vec3 albedo)
{
	return albedo / PI;
}

//GGX (Trowbridge-Reitz)
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness*roughness;
    float a2 = a * a;
    float NdotH = max(abs(dot(N, H)), 0.0);
    float NdotH2 = NdotH * NdotH;

    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    float denom2 = denom * denom;

    return a2 / (PI * denom2);
}

//Schlick 
vec3 FresnelSchlick(float cosTheta, vec3 F0) // costTheta is the LDotH or the anlge between the half vector (or HdotV)
{
    return F0 + (1.0 - F0) * pow(max(1.0 - cosTheta, 0.0), 5.0);
}

//https://de45xmedrsdbp.cloudfront.net/Resources/files/2013SiggraphPresentationsNotes-26915738.pdf PG. 3
float GeometrySchlickGGX(vec3 N, vec3 V, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float NdotV = max(abs(dot(N, V)), 0.0);

    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return num / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    return GeometrySchlickGGX(N, V, roughness) *  GeometrySchlickGGX(N, L, roughness);
}

//Different version for IBL, k = (r * r) / 2, 
float GeometrySchlickGGXIBL(vec3 N, vec3 V, float roughness)
{
    float a = roughness;
    float k = (a * a) / 2.0;

    float NdotV = max(abs(dot(N, V)), 0.0);

    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return num / denom;
}

float GeometrySmithIBL(vec3 N, vec3 V, vec3 L, float roughness)
{
    return GeometrySchlickGGXIBL(N, V, roughness) *  GeometrySchlickGGXIBL(N, L, roughness);
}

vec3 SpecularBRDF(PBRMaterial material, vec3 N, vec3 H, vec3 V, vec3 L, out vec3 FOut)
{
    float NDF = DistributionGGX(N, H, material.roughness);

    vec3 F0 = vec3(0.04); //for non-metallic surfaces F0 is always 0.04. this is a compromise and could be configurable.
    F0 = mix(F0, material.albedo, material.metalness);
    vec3 F = FresnelSchlick(max(abs(dot(H, V)), 0.0), F0);

    FOut = F;

    float G = GeometrySmith(N, V, L, material.roughness);

    //Calculate Cook-Torrance BRDF
    vec3 numerator = NDF * F * G;
    float denominator = 4 * max(abs(dot(N, V)), 0.0) * max(abs(dot(N, L)), 0.0); 

    return numerator / max(denominator, 0.001);   
}

vec3 CalculateDirectLighting(PBRMaterial material, vec3 V, vec3 L, vec3 N)
{
	//vec3 H = normalize(V + L);
	vec3 H = (V + L) / 2.0;

    vec3 diffuse = DiffuseBRDF(material.albedo);

    vec3 kS;
    vec3 specular = SpecularBRDF(material, N, V, H, L, kS);
 
    //Ensure energy conservation
    vec3 kD = vec3(1.0) - kS;
    
    //Remove diffuse component for metalics
    kD *= 1.0 - material.metalness;	

    return kD * diffuse + specular;
}


vec3 FresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(max(1.0 - cosTheta, 0.0), 5.0);
}   

vec3 CalculateIndirectLighting(PBRMaterial material, vec3 V, vec3 N, samplerCube radianceMap, samplerCube irradianceMap, sampler2D integrationMap)
{
    vec3 R = reflect(-V, N);   

    const float MAX_MIP_INDEX = 6.0; //TODO MAKE THIS SOME SORT OF INJECTED VARIABLE;

    vec3 F0 = vec3(0.04); //for non-metallic surfaces F0 is always 0.04. this is a compromise and could be configurable.
    F0 = mix(F0, material.albedo, material.metalness);
    vec3 F = FresnelSchlickRoughness(max(abs(dot(N, V)), 0.0), F0,  material.roughness);

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= vec3(1.0) - material.metalness;	  
    
    vec3 irradiance = texture(irradianceMap, N).rgb;
    vec3 diffuse = irradiance * material.albedo;
    
    vec3 prefilteredColor = textureLod(radianceMap, R,   material.roughness * MAX_MIP_INDEX).rgb;   
    
    vec2 envBRDF = texture(integrationMap, vec2(max(abs(dot(N, V)), 0.0),  material.roughness)).rg;
    vec3 specular = prefilteredColor * (F * envBRDF.x + envBRDF.y);
    
    return (kD * diffuse) + specular; 
}

#endif