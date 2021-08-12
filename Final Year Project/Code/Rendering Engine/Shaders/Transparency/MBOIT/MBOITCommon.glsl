#ifndef MBOIT_COMMON_GLSL
#define MBOIT_COMMON_GLSL

#include "../../common.glsl"

float WarpDepth(float depth, float lnDepthMin, float lnDepthMax)
{
    return ((log(depth) - lnDepthMin) / (lnDepthMax - lnDepthMin)) * 2.0 - 1.0;
}

const float ABSORBANCE_MAX_VALUE = 10.0;

void GenerateMoments(float depth, float transmittance, out float b_0, out vec4 b1234, out vec4 b56)
{
    float absorbance = -log(transmittance);

    if (absorbance > ABSORBANCE_MAX_VALUE) 
    {
        absorbance = ABSORBANCE_MAX_VALUE;
    }

	b_0 = absorbance;
	float depth_pow2 = depth * depth;
	float depth_pow4 = depth_pow2 * depth_pow2;
	float depth_pow6 = depth_pow4 * depth_pow2;
	b1234 = vec4(depth, depth_pow2, depth_pow2 * depth, depth_pow4) * absorbance;   
	b56 = vec4(depth_pow4 * depth, depth_pow6, 0.0, 0.0) * absorbance;   
}

float saturate(float x) 
{
    if (isinf(x)) x = 1.0;
    return clamp(x, 0.0, 1.0);
}

vec3 mul(vec3 v, mat3 m) {
    return m * v;
}

float atan2(in float y, in float x)
{
    bool s = (abs(x) > abs(y));
    return mix(PI/2.0 - atan(x,y), atan(y,x), s);
}

/*! Code taken from the blog "Moments in Graphics" by Christoph Peters.
    http://momentsingraphics.de/?p=105
    This function computes the three real roots of a cubic polynomial
    Coefficient[0]+Coefficient[1]*x+Coefficient[2]*x^2+Coefficient[3]*x^3.*/
vec3 SolveCubic(vec4 Coefficient) 
{
    // Normalize the polynomial
    Coefficient.xyz /= Coefficient.w;
    // Divide middle coefficients by three
    Coefficient.yz /= 3.0f;
    // Compute the Hessian and the discrimant
    vec3 Delta = vec3(
        fma(-Coefficient.z, Coefficient.z, Coefficient.y),
        fma(-Coefficient.y, Coefficient.z, Coefficient.x),
        dot(vec2(Coefficient.z, -Coefficient.y), Coefficient.xy)
        );
    float Discriminant = dot(vec2(4.0f*Delta.x, -Delta.y), Delta.zy);
    // Compute coefficients of the depressed cubic
    // (third is zero, fourth is one)
    vec2 Depressed = vec2(
        fma(-2.0f*Coefficient.z, Delta.x, Delta.y),
        Delta.x
        );
    // Take the cubic root of a normalized complex number
    float Theta = atan2(sqrt(Discriminant), -Depressed.x) / 3.0f;
    vec2 CubicRoot = vec2(cos(Theta), sin(Theta));
    // Compute the three roots, scale appropriately and
    // revert the depression transform
    vec3 Root = vec3(
        CubicRoot.x,
        dot(vec2(-0.5f, -0.5f*sqrt(3.0f)), CubicRoot),
        dot(vec2(-0.5f, 0.5f*sqrt(3.0f)), CubicRoot)
        );
    Root = fma(vec3(2.0f*sqrt(-Depressed.y)), Root, vec3(-Coefficient.z));
    return Root;
}

/*! This function reconstructs the transmittance at the given depth from six
    normalized power moments and the given zeroth moment.*/
float computeTransmittanceAtDepthFrom6PowerMoments(float b_0, vec3 b_even, vec3 b_odd, float depth, float bias, float overestimation, float bias_vector[6])
{
    float b[6] = { b_odd.x, b_even.x, b_odd.y, b_even.y, b_odd.z, b_even.z };
    // Bias input data to avoid artifacts
    //[unroll]
    for (int i = 0; i != 6; ++i) 
    {
        b[i] = mix(b[i], bias_vector[i], bias);
    }

    vec4 z;
    z[0] = depth;

    // Compute a Cholesky factorization of the Hankel matrix B storing only non-
    // trivial entries or related products
    float InvD11 = 1.0f / fma(-b[0], b[0], b[1]);
    float L21D11 = fma(-b[0], b[1], b[2]);
    float L21 = L21D11*InvD11;
    float D22 = fma(-L21D11, L21, fma(-b[1], b[1], b[3]));
    float L31D11 = fma(-b[0], b[2], b[3]);
    float L31 = L31D11*InvD11;
    float InvD22 = 1.0f / D22;
    float L32D22 = fma(-L21D11, L31, fma(-b[1], b[2], b[4]));
    float L32 = L32D22*InvD22;
    float D33 = fma(-b[2], b[2], b[5]) - dot(vec2(L31D11, L32D22), vec2(L31, L32));
    float InvD33 = 1.0f / D33;

    // Construct the polynomial whose roots have to be points of support of the
    // canonical distribution: bz=(1,z[0],z[0]*z[0],z[0]*z[0]*z[0])^T
    vec4 c;
    c[0] = 1.0f;
    c[1] = z[0];
    c[2] = c[1] * z[0];
    c[3] = c[2] * z[0];
    // Forward substitution to solve L*c1=bz
    c[1] -= b[0];
    c[2] -= fma(L21, c[1], b[1]);
    c[3] -= b[2] + dot(vec2(L31, L32), c.yz);
    // Scaling to solve D*c2=c1
    c.yzw *= vec3(InvD11, InvD22, InvD33);
    // Backward substitution to solve L^T*c3=c2
    c[2] -= L32*c[3];
    c[1] -= dot(vec2(L21, L31), c.zw);
    c[0] -= dot(vec3(b[0], b[1], b[2]), c.yzw);

    // Solve the cubic equation
    z.yzw = SolveCubic(c);

    // Compute the absorbance by summing the appropriate weights
    vec4 weigth_factor;
    weigth_factor[0] = overestimation;
    //weigth_factor.yzw = (z.yzw > z.xxx) ? vec3 (0.0f, 0.0f, 0.0f) : vec3 (1.0f, 1.0f, 1.0f);
    //weigth_factor = vec4(overestimation, (z[1] < z[0])?1.0f:0.0f, (z[2] < z[0])?1.0f:0.0f, (z[3] < z[0])?1.0f:0.0f);
    weigth_factor.yzw = mix(vec3(1.0f, 1.0f, 1.0f), vec3(0.0f, 0.0f, 0.0f), ivec3(greaterThan(z.yzw, z.xxx)));
    // Construct an interpolation polynomial
    float f0 = weigth_factor[0];
    float f1 = weigth_factor[1];
    float f2 = weigth_factor[2];
    float f3 = weigth_factor[3];
    float f01 = (f1 - f0) / (z[1] - z[0]);
    float f12 = (f2 - f1) / (z[2] - z[1]);
    float f23 = (f3 - f2) / (z[3] - z[2]);
    float f012 = (f12 - f01) / (z[2] - z[0]);
    float f123 = (f23 - f12) / (z[3] - z[1]);
    float f0123 = (f123 - f012) / (z[3] - z[0]);
    vec4 polynomial;
    // f012+f0123 *(z-z2)
    polynomial[0] = fma(-f0123, z[2], f012);
    polynomial[1] = f0123;
    // *(z-z1) +f01
    polynomial[2] = polynomial[1];
    polynomial[1] = fma(polynomial[1], -z[1], polynomial[0]);
    polynomial[0] = fma(polynomial[0], -z[1], f01);
    // *(z-z0) +f0
    polynomial[3] = polynomial[2];
    polynomial[2] = fma(polynomial[2], -z[0], polynomial[1]);
    polynomial[1] = fma(polynomial[1], -z[0], polynomial[0]);
    polynomial[0] = fma(polynomial[0], -z[0], f0);
    float absorbance = dot(polynomial, vec4 (1.0, b[0], b[1], b[2]));
    // Turn the normalized absorbance into transmittance
    return saturate(exp(-b_0 * absorbance));
}

void ResolveMoments(out float transmittance_at_depth, out float total_transmittance, float depth, float moment_bias, float overestimation, float b0, vec4 b_1234, vec4 b_56)
{
    transmittance_at_depth = 1.0;
    total_transmittance = 1.0; 

    if(b0 < 0.00100050033f) //Completely transparent
    {
        discard;
    }
        
    total_transmittance = exp(-b0);

    vec3 b_even = vec3(b_1234.y, b_1234.w, b_56.y);
    vec3 b_odd = vec3(b_1234.x, b_1234.z, b_56.x);
    b_even /= b0;
    b_odd /= b0;

    const float bias_vector[6] = { 0, 0.48, 0, 0.451, 0, 0.45 };

    transmittance_at_depth = computeTransmittanceAtDepthFrom6PowerMoments(b0, b_even, b_odd, depth, moment_bias, overestimation, bias_vector);
}

#endif