#ifndef BTH_PERLIN_H
#define BTH_PERLIN_H
// implementation taken from https://gist.github.com/nowl/828013

#ifndef SEED
#define SEED 0
#endif

// defaults to smoother lerp
#ifndef LERP
#define LERP(x, y ,s) (lerp((x), (y), (s) * (s) * (3 - 2 * (s))))
#endif

float perlin_noise2d(float x, float y);
float perlin2d(float x, float y, float freq, int depth);

#endif

#ifdef BTH_PERLIN_IMPLEMENTATION

#ifndef PERMUTATIONS
static int PERMUTS[] = {
    208,34,231,213,32,248,233,56,161,78,24,140,71,48,140,254,245,255,247,247,
    40,185,248,251,245,28,124,204,204,76,36,1,107,28,234,163,202,224,245,128,
    167,204,9,92,217,54,239,174,173,102,193,189,190,121,100,108,167,44,43,77,
    180,204,8,81,70,223,11,38,24,254,210,210,177,32,81,195,243,125,8,169,112,
    32,97,53,195,13,203,9,47,104,125,117,114,124,165,203,181,235,193,206,70,
    180,174,0,167,181,41,164,30,116,127,198,245,146,87,224,149,206,57,4,192,
    210,65,210,129,240,178,105,228,108,245,148,140,40,35,195,38,58,65,207,215,
    253,65,85,208,76,62,3,237,55,89,232,50,217,64,244,157,199,121,252,90,17,
    212,203,149,152,140,187,234,177,73,174,193,100,192,143,97,53,145,135,19,
    103,13,90,135,151,199,91,239,247,33,39,145,101,120,99,3,186,86,99,41,237,
    203,111,79,220,135,158,42,30,154,120,67,87,167,135,176,183,191,253,115,184,
    21,233,58,129,233,142,39,128,211,118,137,139,255,114,20,218,113,154,27,127,
    246,250,1,8,198,250,209,92,222,173,21,88,102,219
};
#endif

int noise2(int x, int y)
{
    int tmp = PERMUTS[(y + SEED) % 256];
    return PERMUTS[(tmp + x) % 256];
}

float lerp(float x, float y, float s)
{
    return x + s * (y-x);
}

float perlin_noise2d(float x, float y)
{
    int x_int = x;
    int y_int = y;
    float x_frac = x - x_int;
    float y_frac = y - y_int;
    int s = noise2(x_int, y_int);
    int t = noise2(x_int + 1, y_int);
    int u = noise2(x_int, y_int + 1);
    int v = noise2(x_int + 1, y_int + 1);
    float low = LERP(s, t, x_frac);
    float high = LERP(u, v, x_frac);

    return LERP(low, high, y_frac);
}

float perlin2d(float x, float y, float freq, int depth)
{
    float xa = x * freq;
    float ya = y * freq;
    float amp = 1.0;
    float fin = 0;
    float div = 0.0;

    int i;
    for(int i = 0; i < depth; i++)
    {
        div += 256 * amp;
        fin += perlin_noise2d(xa, ya) * amp;
        amp /= 2;
        xa *= 2;
        ya *= 2;
    }

    return fin/div;
}

#endif
