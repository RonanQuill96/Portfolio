#ifndef LIGHTING_COMMON
#define LIGHTING_COMMON

struct PointLightData
{
    vec3 lightColour;
	float lightIntensity;

	vec3 position;
	float range;
};

float LinearDepth(float depthSample, float zNear, float zFar)
{
    float depthRange = depthSample;//2.0 * depthSample - 1.0;
    // Near... Far... wherever you are...
    float linear = 2.0 * zNear * zFar / (zFar + zNear - depthRange * (zFar - zNear));
    return linear;
}


#endif