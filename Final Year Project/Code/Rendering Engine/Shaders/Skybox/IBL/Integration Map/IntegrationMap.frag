
#version 450
#extension GL_ARB_separate_shader_objects : enable

#include "../../../pbr.glsl"

layout(set = 0, location = 1) in vec2 fragTexCoord;
layout(set = 0, location = 0) in vec3 position;

layout(set = 0, location = 0) out vec4 outColor;

//=================================================================================================
// http://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf 
// ================================================================================================
vec2 IntegrateBRDF( float Roughness, float NoV )
{
    vec3 V;
    V.x = sqrt( 1.0f - NoV * NoV ); // sin
    V.y = 0;
    V.z = NoV; // cos

    float A = 0;
    float B = 0;

	vec3 N = vec3(0.0f, 0.0f, 1.0f);

    const uint NumSamples = 1024;
    for( uint i = 0; i < NumSamples; i++ )
    {
        vec2 Xi = Hammersley( i, NumSamples );
        vec3 H = ImportanceSampleGGX( Xi, Roughness, N );
        vec3 L = 2 * dot( V, H ) * H - V;

        float NoL = max( L.z, 0.0 );
        float NoH = max( H.z, 0.0 );
        float VoH = max( dot( V, H ), 0.0 );

        if( NoL > 0 )
        {
            float G = GeometrySmithIBL(N, V, L, Roughness);
            float G_Vis = G * VoH / (NoH * NoV);
            float Fc = pow( 1 - VoH, 5 );
            A += (1 - Fc) * G_Vis;
            B += Fc * G_Vis;
        }
    }

    return vec2( A, B ) / NumSamples;
}

void main() 
{
	outColor = vec4(IntegrateBRDF(fragTexCoord.x, fragTexCoord.y), 0.0, 1.0);
}
