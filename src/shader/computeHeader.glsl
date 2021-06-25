#include "bindingsHeader.glsl"

layout(constant_id = 1) const int LOCAL_SIZE_X = 16;
layout(constant_id = 2) const int LOCAL_SIZE_Y = 16;
layout(local_size_x_id = 1, local_size_y_id = 2) in;

const int SAMPLE_COUNT = 32;
const float SAMPLE_STEP = 1.0f/SAMPLE_COUNT;
const int PX_RANGE = SAMPLE_COUNT;//+WIDTH/60;

ivec2 outCoord = ivec2(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y);

int getPxFocus(int fi)
{
    float f = fi*SAMPLE_STEP;
    float transformed = pow((f-1),3)+1;
    //float a=0.3;
    //float c=10;
    float a=0.5;
    float c=1;
    float b = -atan(-c/a);
    float d = atan((1-c)/a)+b;
    //float transformed = a*tan(d*f-b)+c;
    //float transformed = f;
    return int(round(transformed*PX_RANGE));
}

vec2 normalizeCoord(ivec2 coord)
{
    return clamp(vec2(coord+0.5f)/vec2(WIDTH,HEIGHT), 0.0, 1.0);
}


