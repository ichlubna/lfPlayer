#include "gpu.h"
#include<iostream>
void Gpu::updateFrameIndices(std::vector<LfCurrentFrame> &frames)
{
    for(size_t i=0; i<frames.size(); i++)
    {
        bool changed = currentFrames[i].index != frames[i].index;
        currentFrames[i] = {frames[i].index, frames[i].weight, frames[i].offset, changed};
        size_t rowId = i*4;
        *uniforms.lfAttribs[rowId] = frames[i].weight;
        *uniforms.lfAttribs[rowId+1] = frames[i].offset.x;
        *uniforms.lfAttribs[rowId+2] = frames[i].offset.y;
    }
}
