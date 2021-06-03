#include "gpu.h"

void Gpu::updateFrameIndices(std::vector<LfCurrentFrame> &frames)
{
    for(size_t i=0; i<frames.size(); i++)
    {
        bool changed = currentFrames[i].index != frames[i].index;
        currentFrames[i] = {frames[i].index, frames[i].weight, changed}; 
    }
}
