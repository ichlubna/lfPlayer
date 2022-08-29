#version 450
#extension GL_ARB_separate_shader_objects : enable

#include "bindingsHeader.glsl"
#include "generalHeader.glsl"

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 outColor;

void main()
{
    outColor = vec4(0);
    int minSample = int(round(SAMPLE_COUNT*medianLoad(1*MAP_HALF_PX_SIZE*2, uv, OUTPUT_TEXTURE_ID)));  
    if(uniforms.switchView == 0)
    {
        for(int j=0; j<4; j++)
            outColor += lfFrameWeight(j)*texture(sampler2D(textures[2+j], textSampler),lfFrameOffset(j)*calculateFocus(minSample)*rangeLimit*vec2(ASPECT_RATIO, 1.0f)+uv);
    }
    else
    {   
        outColor = vec4(float(minSample)/SAMPLE_COUNT);
        outColor.w = 1.0f;
    }
}
