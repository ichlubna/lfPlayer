#include "bindingsHeader.glsl"

layout(constant_id = 1) const int LOCAL_SIZE_X = 16;
layout(constant_id = 2) const int LOCAL_SIZE_Y = 16;
layout(local_size_x_id = 1, local_size_y_id = 2) in;
