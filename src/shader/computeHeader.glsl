layout(constant_id = 0) const int MAX_TEXTURES = 64;
layout(constant_id = 1) const int LOCAL_SIZE_X = 16;
layout(constant_id = 2) const int LOCAL_SIZE_Y = 16;
layout(local_size_x_id = 1, local_size_y_id = 2) in;
layout(set = 0, binding = 0, rgba8) uniform writeonly image2D textures[MAX_TEXTURES];
layout(set = 0, binding = 1) uniform sampler textSampler;
layout(set = 0, binding = 2) uniform uniformBuffer
{
    float focus;
} uniforms;
