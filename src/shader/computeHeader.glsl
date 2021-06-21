#include "bindingsHeader.glsl"

layout(constant_id = 1) const int LOCAL_SIZE_X = 16;
layout(constant_id = 2) const int LOCAL_SIZE_Y = 16;
layout(local_size_x_id = 1, local_size_y_id = 2) in;

const int SAMPLE_COUNT = 256;
const float SAMPLE_STEP = 1.0f/SAMPLE_COUNT;
const int PX_RANGE = WIDTH/60;

ivec2 outCoord = ivec2(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y);

int getPxFocus(int fi)
{
    float f = fi*SAMPLE_STEP;
    float powered = pow((f-1),3)+1;
    //float loged = log(f+0.1)+1;
    return int(round(powered*PX_RANGE));
}

vec2 normalizeCoord(ivec2 coord)
{
    return clamp(vec2(coord)/vec2(WIDTH,HEIGHT), 0.0, 1.0);
}


