float rangeLimit = intBitsToFloat(shaderStorage.data[0]);

float SAMPLE_STEP = 1.0f / SAMPLE_COUNT;
float calculateFocus(int fi)
{
    float f = fi * SAMPLE_STEP;
    float transformed = f;
    //transformed = pow((f-1.f),3.f)+1.f;
    //float a=0.3;
    //float c=10;
    float a = 0.5;
    float c = 1;
    float b = -atan(-c / a);
    float d = atan((1 - c) / a) + b;
    //transformed = a*tan(d*f-b)+c;
    return transformed;
}

void minmax(inout int u, inout int v)
{
    int save = u;
    u = min(save, v);
    v = max(save, v);
}
void minmax(inout ivec2 u, inout ivec2 v)
{
    ivec2 save = u;
    u = min(save, v);
    v = max(save, v);
}
void minmax(inout ivec4 u, inout ivec4 v)
{
    ivec4 save = u;
    u = min(save, v);
    v = max(save, v);
}
void minmax3(inout ivec4 e[3])
{
    minmax(e[0].x, e[0].y);
    minmax(e[0].x, e[0].z);
    minmax(e[0].y, e[0].z);
}
void minmax4(inout ivec4 e[3])
{
    minmax(e[0].xy, e[0].zw);
    minmax(e[0].xz, e[0].yw);
}
void minmax5(inout ivec4 e[3])
{
    minmax(e[0].xy, e[0].zw);
    minmax(e[0].xz, e[0].yw);
    minmax(e[0].x, e[1].x);
    minmax(e[0].w, e[1].x);
}
void minmax6(inout ivec4 e[3])
{
    minmax(e[0].xy, e[0].zw);
    minmax(e[0].xz, e[0].yw);
    minmax(e[1].x, e[1].y);
    minmax(e[0].xw, e[1].xy);
}

float medianLoad(vec2 size, vec2 coord, int imgId)
{
    ivec4 e[3];
    int i = 0;
    ivec2 offset;
    for(offset.y = -1; offset.y <= 1; ++offset.y)
    {
        for(offset.x = -1; offset.x <= 1; ++offset.x)
        {
            e[i / 4][i % 4] = int(round(texture(sampler2D(textures[imgId], textSampler), coord + offset * (size)).r * 255));

            ++i;
        }
    }
    minmax6(e);
    e[0].x = e[2].x;
    minmax5(e);
    e[0].x = e[1].w;
    minmax4(e);
    e[0].x = e[1].z;
    minmax3(e);
    return e[0].y / 255.0f;
}

