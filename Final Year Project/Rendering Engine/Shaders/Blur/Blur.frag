#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, location = 0) in vec3 position;
layout(set = 0, location = 1) in vec2 fragTexCoord;

layout(set = 0, binding = 0) uniform sampler2D aoMap;

layout (location = 0) out float outColor; 

layout(push_constant) uniform PushConstantObject
{
    vec2 imageResolution;
    vec2 invImageResolution;

	bool horizontal;
} pushConstants;

float weight[5] = float[] (0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

void main(void)
{
    float result = texture(aoMap, fragTexCoord).r * weight[0]; // current fragment's contribution
    if(pushConstants.horizontal)
    {
        for(int i = 1; i < 5; ++i)
        {
            result += texture(aoMap, fragTexCoord + vec2(pushConstants.invImageResolution.x * i, 0.0)).r * weight[i];
            result += texture(aoMap, fragTexCoord - vec2(pushConstants.invImageResolution.x * i, 0.0)).r * weight[i];
        }
    }
    else
    {
        for(int i = 1; i < 5; ++i)
        {
            result += texture(aoMap, fragTexCoord + vec2(0.0, pushConstants.invImageResolution.y * i)).r * weight[i];
            result += texture(aoMap, fragTexCoord - vec2(0.0, pushConstants.invImageResolution.y * i)).r * weight[i];
        }
    }

    outColor = result.r;
}