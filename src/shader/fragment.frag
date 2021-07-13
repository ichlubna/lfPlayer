#version 450
#extension GL_ARB_separate_shader_objects : enable

#include "bindingsHeader.glsl"
#include "generalHeader.glsl"

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 outColor;

void main()
{
    int minSample = int(round(SAMPLE_COUNT*medianLoad(1, uv, OUTPUT_TEXTURE_ID)));  
    if(uniforms.switchView == 0)
    {
        for(int j=0; j<4; j++)
            outColor += lfFrameWeight(j)*texture(sampler2D(textures[2+j], textSampler),lfFrameOffset(j)*calculateFocus(minSample)*rangeLimit*vec2(ASPECT_RATIO, 1.0f)+uv);
        //outColor = texture(sampler2D(textures[OUTPUT_TEXTURE_ID], textSampler),uv);
    }
    else
    {   
        outColor = vec4(float(minSample)/SAMPLE_COUNT);
        outColor.w = 1.0f;
    }
}
