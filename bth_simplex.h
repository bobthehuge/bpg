/**
 * DISCLAIMER: THIS HEADER-ONLY IS A BASIC TRANSLATION OF A C++ PROJECT.
 * IT FITS IT'S PURPOSE AS A SIMPLE YET COMPLETE SIMPLEX LIBRARY TO USE IN MY
 * PROJECTS BUT IT MIGHT NOT BE THE BEST C IMPLEMENTATION. I BARELY TOUCHED ANY
 * OF THE DOCUMENTATION WORK DONE BY THE ORIGINAL AUTHOR, ALL CREDITS GO TO HIM
 *
 * @file    SimplexNoise.cpp
 * @brief   A Perlin Simplex Noise C++ Implementation (1D, 2D, 3D).
 *
 * Copyright (c) 2014-2018 Sebastien Rombauts (sebastien.rombauts@gmail.com)
 *
 * This C++ implementation is based on the speed-improved Java version 2012-03-09
 * by Stefan Gustavson (original Java source code in the public domain).
 * http://webstaff.itn.liu.se/~stegu/simplexnoise/SimplexNoise.java:
 * - Based on example code by Stefan Gustavson (stegu@itn.liu.se).
 * - Optimisations by Peter Eastman (peastman@drizzle.stanford.edu).
 * - Better rank ordering method by Stefan Gustavson in 2012.
 *
 * This implementation is "Simplex Noise" as presented by
 * Ken Perlin at a relatively obscure and not often cited course
 * session "Real-Time Shading" at Siggraph 2001 (before real
 * time shading actually took on), under the title "hardware noise".
 * The 3D function is numerically equivalent to his Java reference
 * code available in the PDF course notes, although I re-implemented
 * it from scratch to get more readable code. The 1D, 2D and 4D cases
 * were implemented from scratch by me from Ken Perlin's text.
 *
 * Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
 * or copy at http://opensource.org/licenses/MIT)
 */

#ifndef BTH_SIMPLEX_H
#define BTH_SIMPLEX_H

// Parameters of Fractional Brownian Motion (fBm) : sum of N "octaves" of noise

// Frequency ("width") of the first octave of noise
#ifndef BTH_SIMPLEX_FBM_FREQ
#define BTH_SIMPLEX_FBM_FREQ ldexp(1, -5)
#endif

// Amplitude ("height") of the first octave of noise
#ifndef BTH_SIMPLEX_FBM_AMPL
#define BTH_SIMPLEX_FBM_AMPL 0.5
#endif

// Lacunarity specifies the frequency multiplier between successive octaves
#ifndef BTH_SIMPLEX_FBM_LACU
#define BTH_SIMPLEX_FBM_LACU 2.0
#endif

// Persistence is the loss of amplitude between successive octaves 
// (usually 1/lacunarity)
#ifndef BTH_SIMPLEX_FBM_PERS
#define BTH_SIMPLEX_FBM_PERS 0.5
#endif

#include <stdlib.h>  // size_t

float perlin_fbm2d(size_t octaves, float x, float y);

#endif

#ifdef BTH_SIMPLEX_IMPLEMENTATION

#include <stdint.h>  // int32_t/uint8_t

/**
 * Computes the largest integer value not greater than the float one
 *
 * This method is faster than using (int32_t)std::floor(fp).
 *
 * I measured it to be approximately twice as fast:
 *  float:  ~18.4ns instead of ~39.6ns on an AMD APU),
 *  double: ~20.6ns instead of ~36.6ns on an AMD APU),
 * Reference: http://www.codeproject.com/Tips/700780/Fast-floor-ceiling-functions
 *
 * @param[in] fp    float input value
 *
 * @return largest integer value not greater than fp
 */
static inline int32_t fastfloor(float fp)
{
    int32_t i = (int32_t)(fp);
    return (fp < i) ? (i - 1) : (i);
}

/**
 * Permutation table. This is just a random jumble of all numbers 0-255.
 *
 * This produce a repeatable pattern of 256, but Ken Perlin stated
 * that it is not a problem for graphic texture as the noise features disappear
 * at a distance far enough to be able to see a repeatable pattern of 256.
 *
 * This needs to be exactly the same for all instances on all platforms,
 * so it's easiest to just keep it as static explicit data.
 * This also removes the need for any initialisation of this class.
 *
 * Note that making this an uint32_t[] instead of a uint8_t[] might make the
 * code run faster on platforms with a high penalty for unaligned single
 * byte addressing. Intel x86 is generally single-byte-friendly, but
 * some other CPUs are faster with 4-aligned reads.
 * However, a char[] is smaller, which avoids cache trashing, and that
 * is probably the most important aspect on most architectures.
 * This array is accessed a *lot* by the noise functions.
 * A vector-valued noise over 3D accesses it 96 times, and a
 * float-valued 4D noise 64 times. We want this to fit in the cache!
 */

#ifndef PERMUTATIONS
#define PERMUTATIONS
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

/**
 * Helper function to hash an integer using the above permutation table
 *
 *  This inline function costs around 1ns, and is called N+1 times for a noise 
 *  of N dimension.
 *
 *  Using a real hash function would be better to improve the "repeatability of
 *  256" of the above permutation table, but fast integer Hash functions uses
 *  more time and have bad random properties.
 *
 * @param[in] i Integer value to hash
 *
 * @return 8-bits hashed value
 */
static inline uint8_t hash(int32_t i)
{
    return PERMUTS[i % 256];
}

/* NOTE Gradient table to test if lookup-table are more efficient than calculs
static const float gradients1D[16] = {
        -8.f, -7.f, -6.f, -5.f, -4.f, -3.f, -2.f, -1.f,
         1.f,  2.f,  3.f,  4.f,  5.f,  6.f,  7.f,  8.f
};
*/

/**
 * Helper function to compute gradients-dot-residual vectors (1D)
 *
 * @note that these generate gradients of more than unit length. To make
 * a close match with the value range of classic Perlin noise, the final
 * noise values need to be rescaled to fit nicely within [-1,1].
 * (The simplex noise functions as such also have different scaling.)
 * Note also that these noise functions are the most practical and useful
 * signed version of Perlin noise.
 *
 * @param[in] hash  hash value
 * @param[in] x     distance to the corner
 *
 * @return gradient value
 */
float grad1d(int32_t hash, float x)
{
    // Convert low 4 bits of hash code
    const int32_t h = hash & 0x0F;

    // Gradient value 1.0, 2.0, ..., 8.0
    float grad = 1.0f + (h & 7);

    // Set a random sign for the gradient
    if ((h & 8) != 0) grad = -grad;

    // NOTE : Test of Gradient look-up table instead of the above
    // float grad = gradients1D[h];

    // Multiply the gradient with the distance
    return (grad * x);
}

/**
 * Helper functions to compute gradients-dot-residual vectors (2D)
 *
 * @param[in] hash  hash value
 * @param[in] x     x coord of the distance to the corner
 * @param[in] y     y coord of the distance to the corner
 *
 * @return gradient value
 */
float grad2d(int32_t hash, float x, float y)
{
    // Convert low 3 bits of hash code
    const int32_t h = hash & 0x3F;

    // into 8 simple gradient directions,
    const float u = h < 4 ? x : y;
    const float v = h < 4 ? y : x;

    // and compute the dot product with (x,y).
    return ((h & 1) ? -u : u) + ((h & 2) ? -2.0f * v : 2.0f * v);
}

/**
 * Helper functions to compute gradients-dot-residual vectors (3D)
 *
 * @param[in] hash  hash value
 * @param[in] x     x coord of the distance to the corner
 * @param[in] y     y coord of the distance to the corner
 * @param[in] z     z coord of the distance to the corner
 *
 * @return gradient value
 */
float grad3d(int32_t hash, float x, float y, float z)
{
    // Convert low 4 bits of hash code into 12 simple
    int h = hash & 15;

    // gradient directions, and compute dot product.
    float u = h < 8 ? x : y;

    // Fix repeats at h = 12 to 15
    float v = h < 4 ? y : h == 12 || h == 14 ? x : z;
    
    return ((h & 1) ? -u : u) + ((h & 2) ? -v : v);
}

/**
 * 1D Perlin simplex noise
 *
 *  Takes around 74ns on an AMD APU.
 *
 * @param[in] x float coordinate
 *
 * @return Noise value in the range[-1; 1], value of 0 on all integer 
 * coordinates.
 */
float noise1d(float x)
{
    float n0, n1;   // Noise contributions from the two "corners"

    // No need to skew the input space in 1D

    // Corners coordinates (nearest integer values):
    int32_t i0 = fastfloor(x);
    int32_t i1 = i0 + 1;
    // Distances to corners (between 0 and 1):
    float x0 = x - i0;
    float x1 = x0 - 1.0f;

    // Calculate the contribution from the first corner
    float t0 = 1.0f - x0*x0;
    // if(t0 < 0.0f) t0 = 0.0f; // not possible
    t0 *= t0;
    n0 = t0 * t0 * grad1d(hash(i0), x0);

    // Calculate the contribution from the second corner
    float t1 = 1.0f - x1*x1;
    // if(t1 < 0.0f) t1 = 0.0f; // not possible
    t1 *= t1;
    n1 = t1 * t1 * grad1d(hash(i1), x1);

    // The maximum value of this noise is 8*(3/4)^4 = 2.53125
    // A factor of 0.395 scales to fit exactly within [-1,1]
    return 0.395f * (n0 + n1);
}

/**
 * 2D Perlin simplex noise
 *
 *  Takes around 150ns on an AMD APU.
 *
 * @param[in] x float coordinate
 * @param[in] y float coordinate
 *
 * @return Noise value in the range[-1; 1], value of 0 on all integer 
 * coordinates.
 */
float noise2d(float x, float y)
{
    float n0, n1, n2;   // Noise contributions from the three corners

    // Skewing/Unskewing factors for 2D

    // F2 = (sqrt(3) - 1) / 2
    // G2 = (3 - sqrt(3)) / 6   = F2 / (1 + 2 * K)

    static const float F2 = 0.366025403f;
    static const float G2 = 0.211324865f;

    // Skew the input space to determine which simplex cell we're in
    const float s = (x + y) * F2;  // Hairy factor for 2D
    const float xs = x + s;
    const float ys = y + s;
    const int32_t i = fastfloor(xs);
    const int32_t j = fastfloor(ys);

    // Unskew the cell origin back to (x,y) space
    const float t = (float)(i + j) * G2;
    const float X0 = i - t;
    const float Y0 = j - t;
    const float x0 = x - X0;  // The x,y distances from the cell origin
    const float y0 = y - Y0;

    // For the 2D case, the simplex shape is an equilateral triangle.
    // Determine which simplex we are in.
    // Offsets for second (middle) corner of simplex in (i,j) coords
    int32_t i1, j1;

    // lower triangle, XY order: (0,0)->(1,0)->(1,1)
    if (x0 > y0)
    {
        i1 = 1;
        j1 = 0;
    }
    // upper triangle, YX order: (0,0)->(0,1)->(1,1)
    else
    {
        i1 = 0;
        j1 = 1;
    }

    // A step of (1,0) in (i,j) means a step of (1-c,-c) in (x,y), and
    // a step of (0,1) in (i,j) means a step of (-c,1-c) in (x,y), where
    // c = (3-sqrt(3))/6

    // Offsets for middle corner in (x,y) unskewed coords
    const float x1 = x0 - i1 + G2;
    const float y1 = y0 - j1 + G2;

    // Offsets for last corner in (x,y) unskewed coords
    const float x2 = x0 - 1.0f + 2.0f * G2;
    const float y2 = y0 - 1.0f + 2.0f * G2;

    // Work out the hashed gradient indices of the three simplex corners
    const int gi0 = hash(i + hash(j));
    const int gi1 = hash(i + i1 + hash(j + j1));
    const int gi2 = hash(i + 1 + hash(j + 1));

    // Calculate the contribution from the first corner
    float t0 = 0.5f - x0*x0 - y0*y0;
    if (t0 < 0.0f)
    {
        n0 = 0.0f;
    }
    else
    {
        t0 *= t0;
        n0 = t0 * t0 * grad2d(gi0, x0, y0);
    }

    // Calculate the contribution from the second corner
    float t1 = 0.5f - x1*x1 - y1*y1;
    if (t1 < 0.0f)
    {
        n1 = 0.0f;
    }
    else
    {
        t1 *= t1;
        n1 = t1 * t1 * grad2d(gi1, x1, y1);
    }

    // Calculate the contribution from the third corner
    float t2 = 0.5f - x2*x2 - y2*y2;
    if (t2 < 0.0f)
    {
        n2 = 0.0f;
    }
    else
    {
        t2 *= t2;
        n2 = t2 * t2 * grad2d(gi2, x2, y2);
    }

    // Add contributions from each corner to get the final noise value.
    // The result is scaled to return values in the interval [-1,1].
    return 45.23065f * (n0 + n1 + n2);
}


/**
 * 3D Perlin simplex noise
 *
 * @param[in] x float coordinate
 * @param[in] y float coordinate
 * @param[in] z float coordinate
 *
 * @return Noise value in the range[-1; 1], value of 0 on all integer coordinates.
 */
float noise3d(float x, float y, float z)
{
    float n0, n1, n2, n3; // Noise contributions from the four corners

    // Skewing/Unskewing factors for 3D
    static const float F3 = 1.0f / 3.0f;
    static const float G3 = 1.0f / 6.0f;

    // Skew the input space to determine which simplex cell we're in
    float s = (x + y + z) * F3; // Very nice and simple skew factor for 3D
    int i = fastfloor(x + s);
    int j = fastfloor(y + s);
    int k = fastfloor(z + s);
    float t = (i + j + k) * G3;
    float X0 = i - t; // Unskew the cell origin back to (x,y,z) space
    float Y0 = j - t;
    float Z0 = k - t;
    float x0 = x - X0; // The x,y,z distances from the cell origin
    float y0 = y - Y0;
    float z0 = z - Z0;

    // For the 3D case, the simplex shape is a slightly irregular tetrahedron.
    // Determine which simplex we are in.
    int i1, j1, k1; // Offsets for second corner of simplex in (i,j,k) coords
    int i2, j2, k2; // Offsets for third corner of simplex in (i,j,k) coords

    if (x0 >= y0)
    {
        if (y0 >= z0)
        {
            i1 = 1; j1 = 0; k1 = 0; i2 = 1; j2 = 1; k2 = 0; // X Y Z order
        } 
        else if (x0 >= z0)
        {
            i1 = 1; j1 = 0; k1 = 0; i2 = 1; j2 = 0; k2 = 1; // X Z Y order
        }
        else
        {
            i1 = 0; j1 = 0; k1 = 1; i2 = 1; j2 = 0; k2 = 1; // Z X Y order
        }
    }
    // x0<y0
    else
    {
        if (y0 < z0)
        {
            i1 = 0; j1 = 0; k1 = 1; i2 = 0; j2 = 1; k2 = 1; // Z Y X order
        }
        else if (x0 < z0)
        {
            i1 = 0; j1 = 1; k1 = 0; i2 = 0; j2 = 1; k2 = 1; // Y Z X order
        }
        else
        {
            i1 = 0; j1 = 1; k1 = 0; i2 = 1; j2 = 1; k2 = 0; // Y X Z order
        }
    }

    // A step of (1,0,0) in (i,j,k) means a step of (1-c,-c,-c) in (x,y,z),
    // a step of (0,1,0) in (i,j,k) means a step of (-c,1-c,-c) in (x,y,z), and
    // a step of (0,0,1) in (i,j,k) means a step of (-c,-c,1-c) in (x,y,z), 
    // where c = 1/6.

    // Offsets for second corner in (x,y,z) coords
    float x1 = x0 - i1 + G3;
    float y1 = y0 - j1 + G3;
    float z1 = z0 - k1 + G3;

    // Offsets for third corner in (x,y,z) coords
    float x2 = x0 - i2 + 2.0f * G3;
    float y2 = y0 - j2 + 2.0f * G3;
    float z2 = z0 - k2 + 2.0f * G3;

    // Offsets for last corner in (x,y,z) coords
    float x3 = x0 - 1.0f + 3.0f * G3;
    float y3 = y0 - 1.0f + 3.0f * G3;
    float z3 = z0 - 1.0f + 3.0f * G3;

    // Work out the hashed gradient indices of the four simplex corners
    int gi0 = hash(i + hash(j + hash(k)));
    int gi1 = hash(i + i1 + hash(j + j1 + hash(k + k1)));
    int gi2 = hash(i + i2 + hash(j + j2 + hash(k + k2)));
    int gi3 = hash(i + 1 + hash(j + 1 + hash(k + 1)));

    // Calculate the contribution from the four corners
    float t0 = 0.6f - x0*x0 - y0*y0 - z0*z0;
    if (t0 < 0)
    {
        n0 = 0.0;
    }
    else
    {
        t0 *= t0;
        n0 = t0 * t0 * grad3d(gi0, x0, y0, z0);
    }

    float t1 = 0.6f - x1*x1 - y1*y1 - z1*z1;
    if (t1 < 0)
    {
        n1 = 0.0;
    }
    else
    {
        t1 *= t1;
        n1 = t1 * t1 * grad3d(gi1, x1, y1, z1);
    }

    float t2 = 0.6f - x2*x2 - y2*y2 - z2*z2;
    if (t2 < 0)
    {
        n2 = 0.0;
    }
    else
    {
        t2 *= t2;
        n2 = t2 * t2 * grad3d(gi2, x2, y2, z2);
    }

    float t3 = 0.6f - x3*x3 - y3*y3 - z3*z3;
    if (t3 < 0)
    {
        n3 = 0.0;
    }
    else
    {
        t3 *= t3;
        n3 = t3 * t3 * grad3d(gi3, x3, y3, z3);
    }

    // Add contributions from each corner to get the final noise value.
    // The result is scaled to stay just inside [-1,1]
    return 32.0f*(n0 + n1 + n2 + n3);
}


/**
 * Fractal/Fractional Brownian Motion (fBm) summation of 1D Perlin Simplex 
 * noise
 *
 * @param[in] octaves   number of fraction of noise to sum
 * @param[in] x         float coordinate
 *
 * @return Noise value in the range[-1; 1], value of 0 on all integer 
 * coordinates.
 */
float perlin_fbm1d(size_t octaves, float x)
{
    float output    = 0;
    float denom     = 0;
    float frequency = BTH_SIMPLEX_FBM_FREQ;
    float amplitude = BTH_SIMPLEX_FBM_AMPL;

    for (size_t i = 0; i < octaves; i++) {
        output += (amplitude * noise1d(x * frequency));
        denom += amplitude;

        frequency *= BTH_SIMPLEX_FBM_LACU;
        amplitude *= BTH_SIMPLEX_FBM_PERS;
    }

    return (output / denom);
}

/**
 * Fractal/Fractional Brownian Motion (fBm) summation of 2D Perlin Simplex 
 * noise
 *
 * @param[in] octaves   number of fraction of noise to sum
 * @param[in] x         x float coordinate
 * @param[in] y         y float coordinate
 *
 * @return Noise value in the range[-1; 1], value of 0 on all integer 
 * coordinates.
 */
float perlin_fbm2d(size_t octaves, float x, float y)
{
    float output = 0;
    float denom  = 0;
    float frequency = BTH_SIMPLEX_FBM_FREQ;
    float amplitude = BTH_SIMPLEX_FBM_AMPL;

    for (size_t i = 0; i < octaves; i++) {
        output += (amplitude * noise2d(x * frequency, y * frequency));
        denom += amplitude;

        frequency *= BTH_SIMPLEX_FBM_LACU;
        amplitude *= BTH_SIMPLEX_FBM_PERS;
    }

    return ((output / denom) + 1.0) / 2.0;
}

/**
 * Fractal/Fractional Brownian Motion (fBm) summation of 3D Perlin Simplex*
 * noise
 *
 * @param[in] octaves   number of fraction of noise to sum
 * @param[in] x         x float coordinate
 * @param[in] y         y float coordinate
 * @param[in] z         z float coordinate
 *
 * @return Noise value in the range[-1; 1], value of 0 on all integer 
 * coordinates.
 */
float perlin_fbm3d(size_t octaves, float x, float y, float z)
{
    float output = 0;
    float denom  = 0;
    float frequency = BTH_SIMPLEX_FBM_FREQ;
    float amplitude = BTH_SIMPLEX_FBM_AMPL;

    for (size_t i = 0; i < octaves; i++) {
        output += 
            (amplitude * noise3d(x * frequency, y * frequency, z * frequency));
        denom += amplitude;

        frequency *= BTH_SIMPLEX_FBM_LACU;
        amplitude *= BTH_SIMPLEX_FBM_PERS;
    }

    return (output / denom);
}

#endif
