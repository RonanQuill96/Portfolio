#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform UniformBufferObject 
{
    mat4 wvm;
    mat4 world;
    mat4 worldInvTans;
    mat4 view;
} ubo;

layout(set = 0, location = 0) in vec3 inPosition;
layout(set = 0, location = 1) in vec2 inTexCoord;
layout(set = 0, location = 2) in vec3 inNormal;
layout(set = 0, location = 3) in vec3 inTangent;
layout(set = 0, location = 4) in vec3 inBiTangent;

layout(set = 0, location = 0) out vec3 outPosition;
layout(set = 0, location = 1) out vec2 outTexCoord;
layout(set = 0, location = 2) out vec3 outNormal;
layout(set = 0, location = 3) out vec3 outTangent;
layout(set = 0, location = 4) out vec3 outBiTangent;
layout(set = 0, location = 5) out vec3 outScreenPosition;

void main() 
{
    gl_Position = ubo.wvm * vec4(inPosition, 1.0);
    outPosition = vec3(ubo.world * vec4(inPosition, 1.0));
    outScreenPosition = vec3(ubo.view * ubo.world * vec4(inPosition, 1.0)).xyz;

    outTexCoord = inTexCoord;

    outNormal = normalize(mat3(ubo.worldInvTans) * inNormal);
    outTangent = normalize(mat3(ubo.worldInvTans) * inTangent);
    outBiTangent = normalize(mat3(ubo.worldInvTans) * inBiTangent);
}