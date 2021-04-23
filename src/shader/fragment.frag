#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(constant_id = 0) const int MAX_TEXTURES = 64;
layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 outColor;
//layout(set = 0, binding = 0) uniform texture2D textures[MAX_TEXTURES];
layout(set = 0, binding = 0, rgba8) uniform readonly image2D textures[MAX_TEXTURES];
layout(set = 0, binding = 1) uniform sampler textSampler;

void main()
{
    ivec2 coord = ivec2(uv*ivec2(1920,1080));
    outColor = imageLoad(textures[0], coord);//coord);//texture(sampler2D(Texture(textures[0]), textSampler),uv);
}
