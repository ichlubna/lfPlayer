#version 450
#extension GL_ARB_separate_shader_objects : enable

#include "bindingsHeader.glsl"

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 outColor;

void main()
{
    if(uniforms.switchView == 0)
        outColor = texture(sampler2D(textures[OUTPUT_TEXTURE_ID], textSampler),uv);
    else
        outColor = vec4(texture(sampler2D(textures[FOCUSMAP_TEXTURE_ID], textSampler),uv).r);
}
