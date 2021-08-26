//discards the 4th attribute
float colorDistance(vec4 a, vec4 b)
{
    //return max(max(max(abs(a.r-b.r), abs(a.g-b.g)), abs(a.b-b.b)), abs(a.a-b.a)); //Chebyshev
    return max(max(abs(a.r-b.r), abs(a.g-b.g)), abs(a.b-b.b)); //Chebyshev
}

float blockDistance(mat4 a, mat4 b)
{
    float totalDistance = 0; 
    for(int i=0; i<4; i++)
        totalDistance += colorDistance(a[i], b[i]);
    vec4 x=vec4(a[0][3], a[1][3], a[2][3], 1.0f);
    vec4 y=vec4(b[0][3], b[1][3], b[2][3], 1.0f);
    totalDistance += colorDistance(x,y);
    return totalDistance;
}

struct Welford
{
    mat4 mean;
    float m2;
    int n;
};
Welford welford = Welford(mat4(0),0,0);
void addWelford(mat4 block)
{
    welford.n++;
    mat4 delta = block-welford.mean;
    float dist = blockDistance(block, welford.mean);
    welford.mean += delta/welford.n;
    welford.m2 = dist*blockDistance(block, welford.mean);
}
float finishWelford()
{
    float result = welford.m2;// /4*welford.n
    welford.mean = mat4(0);
    welford.m2 = 0;
    welford.n = 0;
    return result; 
}
