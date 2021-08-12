#version 450
#extension GL_ARB_separate_shader_objects : enable

#include "../../../pbr.glsl"

layout(set = 0, location = 1) in vec2 fragTexCoord;
layout(set = 0, location = 0) in vec3 position;

layout(set = 0, location = 0) out vec4 outColor;

layout(push_constant) uniform PushConstants
{
    float face;
    float roughness;
} constants;

layout(set = 0, binding = 0) uniform samplerCube environmentMapTexture;

// ================================================================================================
// http://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf 
// ================================================================================================
vec3 PrefilterEnvMap( float Roughness, vec3 R )
{
    vec3 N = R;
    vec3 V = R;

    vec3 PrefilteredColor = vec3(0);

    const uint NumSamples = 1024;
    float TotalWeight = 0.0;   
    for( uint i = 0; i < NumSamples; i++ )
    {
        vec2 Xi = Hammersley( i, NumSamples );
        vec3 H = ImportanceSampleGGX( Xi, Roughness, N );
        vec3 L = 2 * dot( V, H ) * H - V;
        float NdotL = max(dot(N, L), 0.0);

        if( NdotL > 0 )
        {
            PrefilteredColor += texture(environmentMapTexture, L).rgb * NdotL;
            TotalWeight += NdotL;
        }
    }

    return PrefilteredColor / TotalWeight;
}


void main() 
{
    //vec3 R = normalize(position);
    vec3 R = GetNormalOfFace(constants.face, fragTexCoord);

    outColor = vec4(PrefilterEnvMap(constants.roughness, R), 1.0);
}