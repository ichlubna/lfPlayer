layout(constant_id = 0) const int MAX_TEXTURES = 64;
layout(set = 0, binding = 0) uniform uniformBuffer
{
    float focus;
    int switchView;
} uniforms;
layout(set = 0, binding = 1) uniform sampler textSampler;
layout(set = 0, binding = 2, rgba8) uniform writeonly image2D images[MAX_TEXTURES];
layout(set = 0, binding = 3) uniform texture2D textures[MAX_TEXTURES];
