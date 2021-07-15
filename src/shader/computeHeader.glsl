#include "bindingsHeader.glsl"
#include "generalHeader.glsl"

layout(constant_id = 1) const int LOCAL_SIZE_X = 16;
layout(constant_id = 2) const int LOCAL_SIZE_Y = 16;
layout(local_size_x_id = 1, local_size_y_id = 2) in;

ivec2 outCoord = ivec2(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y);
vec2 mapUV =  clamp(vec2(outCoord+0.5f)/vec2(MAP_WIDTH,MAP_HEIGHT), 0.0, 1.0);


