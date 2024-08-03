#version 430

// #define BLOB_COUNT 256
#define BLOB_COUNT 4

#define EPS 0.000001f
#define INT_MAX 0x7FFFFFFF
#define UINT_MAX 0xFFFFFFFFu
#define FLT_MAX 3.402823466e+38
#define FLT_MIN 1.175494351e-38
#define DBL_MAX 1.7976931348623158e+308
#define DBL_MIN 2.2250738585072014e-308
#define PI 3.1415f
#define TWOPI 6.2831f
#define OCTAVES 8
#define STRENGH 1

// const float kernel = 10.0;
// const float weight = 1.0;

layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

layout(std430, binding = 1) buffer blobsPosSrcLayout {
    uvec2 src[BLOB_COUNT];
};

// layout(std430, binding = 2) writeonly buffer blobsPosDstLayout {
//     uvec2 dst[BLOB_COUNT];
// };

layout(std430, binding = 2) buffer blobsRotLayout {
    uint rots[BLOB_COUNT];
};

uniform uint iFrame;
uniform vec2 iResolution;
uniform float iTime;

// vec2 hash(vec2 p)
// {
// 	vec3 p3 = fract(vec3(p.xyx) * vec3(.1031, .1030, .0973));
//     p3 += dot(p3, p3.yzx+33.33);
//     return fract((p3.xx+p3.yz)*p3.zy);
// }

// uint hash(uint x)
// {
//     x += ( x << 10u );
//     x ^= ( x >>  6u );
//     x += ( x <<  3u );
//     x ^= ( x >> 11u );
//     x += ( x << 15u );
//     return x;
// }

uint hash(uint state)
{
    state ^= 2747636419u;
    state *= 2654435769u;
    state ^= state >> 16;
    state *= 2654435769u;
    state ^= state >> 16;
    state *= 2654435769u;
    return state;
}

uint hash(uvec2 p)
{
    return hash(p.x ^ hash(p.y));
}

vec4 permute(vec4 x){return mod(((x*34.0)+1.0)*x, 289.0);}
vec4 taylorInvSqrt(vec4 r){return 1.79284291400159 - 0.85373472095314 * r;}

float randf(uint m)
{
    const uint ieeeMantissa = 0x007FFFFFu; // binary32 mantissa bitmask
    const uint ieeeOne      = 0x3F800000u; // 1.0 in IEEE binary32

    m &= ieeeMantissa; // Keep only mantissa bits (fractional part)
    m |= ieeeOne; // Add fractional part to 1.0

    float  f = uintBitsToFloat( m ); // Range [1:2]
    return f - 1.0; // Range [0:1]
}

float randf(uvec2 p) { return randf(hash(floatBitsToUint(p))); }

float snoise(vec3 v){ 
  const vec2  C = vec2(1.0/6.0, 1.0/3.0) ;
  const vec4  D = vec4(0.0, 0.5, 1.0, 2.0);

// First corner
  vec3 i  = floor(v + dot(v, C.yyy) );
  vec3 x0 =   v - i + dot(i, C.xxx) ;

// Other corners
  vec3 g = step(x0.yzx, x0.xyz);
  vec3 l = 1.0 - g;
  vec3 i1 = min( g.xyz, l.zxy );
  vec3 i2 = max( g.xyz, l.zxy );

  //  x0 = x0 - 0. + 0.0 * C 
  vec3 x1 = x0 - i1 + 1.0 * C.xxx;
  vec3 x2 = x0 - i2 + 2.0 * C.xxx;
  vec3 x3 = x0 - 1. + 3.0 * C.xxx;

// Permutations
  i = mod(i, 289.0 ); 
  vec4 p = permute( permute( permute( 
             i.z + vec4(0.0, i1.z, i2.z, 1.0 ))
           + i.y + vec4(0.0, i1.y, i2.y, 1.0 )) 
           + i.x + vec4(0.0, i1.x, i2.x, 1.0 ));

// Gradients
// ( N*N points uniformly over a square, mapped onto an octahedron.)
  float n_ = 1.0/7.0; // N=7
  vec3  ns = n_ * D.wyz - D.xzx;

  vec4 j = p - 49.0 * floor(p * ns.z *ns.z);  //  mod(p,N*N)

  vec4 x_ = floor(j * ns.z);
  vec4 y_ = floor(j - 7.0 * x_ );    // mod(j,N)

  vec4 x = x_ *ns.x + ns.yyyy;
  vec4 y = y_ *ns.x + ns.yyyy;
  vec4 h = 1.0 - abs(x) - abs(y);

  vec4 b0 = vec4( x.xy, y.xy );
  vec4 b1 = vec4( x.zw, y.zw );

  vec4 s0 = floor(b0)*2.0 + 1.0;
  vec4 s1 = floor(b1)*2.0 + 1.0;
  vec4 sh = -step(h, vec4(0.0));

  vec4 a0 = b0.xzyw + s0.xzyw*sh.xxyy ;
  vec4 a1 = b1.xzyw + s1.xzyw*sh.zzww ;

  vec3 p0 = vec3(a0.xy,h.x);
  vec3 p1 = vec3(a0.zw,h.y);
  vec3 p2 = vec3(a1.xy,h.z);
  vec3 p3 = vec3(a1.zw,h.w);

//Normalise gradients
  vec4 norm = taylorInvSqrt(vec4(dot(p0,p0), dot(p1,p1), dot(p2, p2), dot(p3,p3)));
  p0 *= norm.x;
  p1 *= norm.y;
  p2 *= norm.z;
  p3 *= norm.w;

// Mix final noise value
  vec4 m = max(0.6 - vec4(dot(x0,x0), dot(x1,x1), dot(x2,x2), dot(x3,x3)), 0.0);
  m = m * m;
  return 42.0 * dot( m*m, vec4( dot(p0,x0), dot(p1,x1), 
                                dot(p2,x2), dot(p3,x3) ) );
}

float fbm(vec3 x) {
	float v = 0.0;
	float a = 0.5;
	vec3 shift = vec3(100);
	for (int i = 0; i < OCTAVES; ++i) {
		v += a * snoise(x);
		x = x * 2.0 + shift;
		a *= 0.5;
	}
	return v;
}

uint rand(uvec3 p)
{
    return hash(
        p.y * uint(iResolution.x) + p.x + 
        hash(int(p.z + iTime * 100000))
    );
}

float random_angle(uint id)
{
    return float(rand(uvec3(src[id].xy, uint(id)))) / FLT_MAX;
}

// float fmodf(float a, float b)
// {
//     return mod((mod(a, b) + b), b);
// }

void main()
{
    bool even_frame = iFrame % 2u == 0;
    ivec2 iRes = ivec2(iResolution);

    for (int i = 0; i < BLOB_COUNT; i++)
    {
        // float a = fbm(vec3(src[i].xy, iTime));
        float r = uintBitsToFloat(rots[i]);
        int x = int(float(src[i].x) + cos(r * TWOPI));
        int y = int(float(src[i].y) + sin(r * TWOPI));

    	if (x <= 0 || y <= 0 || x >= iRes.x || y >= iRes.y)
        {
            float a = random_angle(i);

    		x = min(iRes.x - 1, max(0, x));
    		y = min(iRes.y - 1, max(0, y));

            atomicExchange(rots[i], floatBitsToUint(a));
    	}

        atomicExchange(src[i].x, uint(x));
        atomicExchange(src[i].y, uint(y));
    }
}
