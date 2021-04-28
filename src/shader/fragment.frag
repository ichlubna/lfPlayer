#version 450
#extension GL_ARB_separate_shader_objects : enable

#include "bindingsHeader.glsl"

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 outColor;

void main()
{
    if(uniforms.switchView == 0)
        outColor = texture(sampler2D(textures[0], textSampler),uv);
    else
        outColor = texture(sampler2D(textures[1], textSampler),uv);
}
