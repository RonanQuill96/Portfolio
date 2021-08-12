#version 450
#extension GL_ARB_separate_shader_objects : enable

#include "../../../common.glsl"

layout(set = 0, location = 1) in vec2 fragTexCoord;
layout(set = 0, location = 0) in vec3 position;

layout(set = 0, location = 0) out vec4 outColor;

layout(push_constant) uniform PushConstants
{
    float face;
} constants;

layout(set = 0, binding = 0) uniform samplerCube environmentMapTexture;


void main() 
{

    // The world vector acts as the normal of a tangent surface
    // from the origin, aligned to WorldPos. Given this normal, calculate all
    // incoming radiance of the environment. The result of this radiance
    // is the radiance of light coming from -Normal direction, which is what
    // we use in the PBR shader to sample irradiance.
    //vec3 N = normalize(position);
    //N.y = N.y * -1.0;

    vec3 N = GetNormalOfFace(constants.face, fragTexCoord);

    vec3 irradiance = vec3(0.0);  

    vec3 up = vec3(0.0, 1.0, 0.0);
    vec3 right = cross(up, N);
    up = cross(N, right);

    float sampleDelta = 0.025;
	float innerSampleDelta = 0.025; //0.01;
	float nrSamples = 0.0;

	for (float phi = 0.0; phi < 2.0 * PI; phi += sampleDelta)
	{
		for (float theta = 0.0; theta < 0.5 * PI; theta += innerSampleDelta)
		{
			// spherical to cartesian (in tangent space)
			vec3 tangentSample = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
			// tangent space to world
			vec3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * N;

			irradiance += texture(environmentMapTexture, sampleVec).rgb * cos(theta) * sin(theta);
			nrSamples = nrSamples + 1.0;
		}
	}

	irradiance = PI *irradiance * (1.0 / float(nrSamples));

    outColor = vec4(irradiance, 1.0);
}