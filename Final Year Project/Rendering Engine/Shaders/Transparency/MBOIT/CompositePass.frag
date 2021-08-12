#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, location = 0) in vec3 position;
layout(set = 0, location = 1) in vec2 fragTexCoord;

layout(set = 0, binding = 0) uniform sampler2D opaqueTexture;
layout(set = 0, binding = 1) uniform sampler2D transparentTexture;
layout(set = 0, binding = 2) uniform sampler2D b0Texture;

layout (location = 0) out vec4 outColour;

void main()
{
    vec3 opaque = texture(opaqueTexture, fragTexCoord).rgb;
    vec4 transparentTexture = texture(transparentTexture, fragTexCoord);
    float b0 = texture(b0Texture, fragTexCoord).r;

    if (b0 < 0.00100050033f) 
    {
        outColour = vec4(opaque, 1.0);
        return;
    }

    float total_transmittance = exp(-b0);
    if (isinf(b0)) 
    {
        total_transmittance = 1e7;
    }

    float weightFactor = (1 - total_transmittance) / transparentTexture.a;

    vec3 colour = total_transmittance * opaque + (weightFactor * vec3(transparentTexture.rgb));

    outColour = vec4(colour, 1.0);
}