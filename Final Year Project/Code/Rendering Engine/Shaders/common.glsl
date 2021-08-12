#ifndef COMMON_GLSL
#define COMMON_GLSL

const float PI = 3.14159265359;

vec3 NormalSampleToWorldSpace(vec3 normalMapSample, vec3 normal, vec3 tangent, vec3 bitangent)
{	
	// Uncompress each component from [0,1] to [-1,1].
	normalMapSample = normalize(normalMapSample * 2.0 - 1.0);

	// Build orthonormal basis.
	vec3 N = normal;
	// re-orthogonalize T with respect to N
	vec3 T = tangent;
	//T = normalize(T - dot(T, N) * N);
	vec3 B = bitangent;
	//vec3 B = normalize(cross(T, N));

	mat3 TBN = mat3(T, B, N); 

	// Transform from tangent space to world space.
	return normalize(TBN * normalMapSample);
}

vec3 GammaCorrection(vec3 colour)
{
    return pow(colour, vec3(1.0/2.2));
}

vec3 GetNormalOfFace(float face, vec2 uv)
{    
    uv.y = 1 - uv.y;
    vec2 debiased = uv * 2.0 - 1.0;
    
	vec3 direction = vec3(0);
    if(face == 0)
    {
		direction = vec3(1, -debiased.y, -debiased.x);
    }
    else if(face == 1)
    { 
		direction = vec3(-1, -debiased.y, debiased.x);
	}
    else if(face == 2)
    {
		direction = vec3(debiased.x, 1, debiased.y);
    }
    else if(face == 3)
    {
		direction = vec3(debiased.x, -1, -debiased.y);
    }    
    else if(face == 4)
    {
		direction = vec3(debiased.x, -debiased.y, 1);
    }    
    else /*if(constants.face == 5)*/
    {
		direction = vec3(-debiased.x, -debiased.y, -1);
    }

    return normalize(direction);
}

// Convert screen space coordinates to world space.
vec4 ScreenToWorld(vec2 screenPos, float depth, mat4 InverseProjection, mat4 InverseView)
{
	vec2 transformedScreenPos = vec2(screenPos.x, screenPos.y) * 2.0f - 1.0f;

	// Convert to clip space
	vec4 clip = vec4(transformedScreenPos, depth, 1.0f);

	// Convert to view space
	vec4 view = InverseProjection * clip;
	view /= view.w;

	// Convert to world space
	vec4 world = InverseView * view;

	world.w = view.z;

	return world;
}

#endif