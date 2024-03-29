#version 450
#extension GL_ARB_separate_shader_objects : enable

#include "bindingsHeader.glsl"
#include "generalHeader.glsl"

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 outColor;

void main()
{
    outColor = vec4(0);
    int minSample = int(round(SAMPLE_COUNT * medianLoad(1 * MAP_HALF_PX_SIZE * 2, uv, OUTPUT_TEXTURE_ID)));
    if(uniforms.switchView == 0)
    {
        for(int j = 0; j < 4; j++)
            outColor += lfFrameWeight(j) * texture(sampler2D(textures[2 + j], textSampler), lfFrameOffset(j) * calculateFocus(minSample) * rangeLimit + uv);
    }
    else
    {
        outColor = vec4(float(minSample) / SAMPLE_COUNT);
        outColor.w = 1.0f;
    }

    if(uniforms.screenshot == 1)
    {
        uvec2 pixelCoords = uvec2(uv*ivec2(uniforms.windowWidth, uniforms.windowHeight));
        uint index = pixelCoords.y*uniforms.windowWidth + pixelCoords.x;
        shaderStorage.data[index] = int(packUnorm4x8(outColor)); 
    } 
}
