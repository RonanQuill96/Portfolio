#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, location = 0) in vec3 position;
layout(set = 0, location = 1) in vec2 fragTexCoord;

layout(set = 0, binding = 0) uniform sampler2D depth;
layout(set = 0, binding = 1) uniform sampler2D randomNoise;

layout (location = 0) out float outColor; 

const float PI = 3.14159265;

layout(push_constant) uniform PushConstantObject
{
    mat4 inverseProjection;

	vec2 FocalLen;
    vec2 UVToViewA;
    vec2 UVToViewB;

    vec2 LinMAD;// = vec2(0.1-10.0, 0.1+10.0) / (2.0*0.1*10.0);
} pushConstants;

layout(set = 0, binding = 2) uniform GTOAParameters
{
    vec2 AORes;
    vec2 InvAORes;

    vec2 NoiseScale;
    float AOStrength;
    float R;

    float R2;
    float NegInvR2;
    float TanBias;
    float MaxRadiusPixels;

    int NumDirections;
    int NumSamples;
    float pad1;
    float pad2;
} gtaoParameters;


vec4 ClipToView(vec4 clip)
{
    //View space transform
    vec4 view = pushConstants.inverseProjection * clip;

    //Perspective projection
    view = view / view.w;
    
    return view;
}

vec4 Screen2View(vec4 screen)
{
    //Convert to NDC
    vec2 texCoord = screen.xy / gtaoParameters.AORes.xy; //May not be correct for downscaled version

    //Convert to clipSpace
    vec4 clip = vec4(vec2(texCoord.x, texCoord.y) * 2.0 - 1.0, screen.z, screen.w);
    //Not sure which of the two it is just yet

    return ClipToView(clip);
}

float ViewSpaceZFromDepth(float d)
{
	// [0,1] -> [-1,1] clip space
	//d = d * 2.0 - 1.0;

	// Get view space Z
	return -1.0 / (pushConstants.LinMAD.x * d + pushConstants.LinMAD.y);
}

vec3 UVToViewSpace(vec2 uv, float z)
{
	uv = pushConstants.UVToViewA * uv + pushConstants.UVToViewB;
	return vec3(uv * z, z);
}

vec3 GetViewPos(vec2 uv)
{
	//float z = ViewSpaceZFromDepth(texture(depth, uv).r);
	float z = texture(depth, uv).r;
	//return UVToViewSpace(uv, z);
    return Screen2View(vec4(uv, z, 1.0)).xyz;
}

float TanToSin(float x)
{
	return x * inversesqrt(x*x + 1.0);
}

float InvLength(vec2 V)
{
	return inversesqrt(dot(V,V));
}

float Tangent(vec3 V)
{
	return V.z * InvLength(V.xy);
}

float BiasedTangent(vec3 V)
{
	return V.z * InvLength(V.xy) + gtaoParameters.TanBias;
}

float Tangent(vec3 P, vec3 S)
{
    return -(P.z - S.z) * InvLength(S.xy - P.xy);
}

float Length2(vec3 V)
{
	return dot(V,V);
}

vec3 MinDiff(vec3 P, vec3 Pr, vec3 Pl)
{
    vec3 V1 = Pr - P;
    vec3 V2 = P - Pl;
    return (Length2(V1) < Length2(V2)) ? V1 : V2;
}

vec2 SnapUVOffset(vec2 uv)
{
    return round(uv * gtaoParameters.AORes) * gtaoParameters.InvAORes;
}

float Falloff(float d2)
{
	return d2 * gtaoParameters.NegInvR2 + 1.0f;
}

float HorizonOcclusion(	vec2 deltaUV,
						vec3 P,
						vec3 dPdu,
						vec3 dPdv,
						float randstep,
						float numSamples)
{
	float ao = 0;

	// Offset the first coord with some noise
	vec2 uv = fragTexCoord + SnapUVOffset(randstep*deltaUV);
	deltaUV = SnapUVOffset( deltaUV );

	// Calculate the tangent vector
	vec3 T = deltaUV.x * dPdu + deltaUV.y * dPdv;

	// Get the angle of the tangent vector from the viewspace axis
	float tanH = BiasedTangent(T);
	float sinH = TanToSin(tanH);

	float tanS;
	float d2;
	vec3 S;

	// Sample to find the maximum angle
	for(float s = 1; s <= numSamples; ++s)
	{
		uv += deltaUV;
		S = GetViewPos(uv);
		tanS = Tangent(P, S);
		d2 = Length2(S - P);

		// Is the sample within the radius and the angle greater?
		if(d2 < gtaoParameters.R2 && tanS > tanH)
		{
			float sinS = TanToSin(tanS);
			// Apply falloff based on the distance
			ao += Falloff(d2) * (sinS - sinH);

			tanH = tanS;
			sinH = sinS;
		}
	}
	
	return ao;
}

vec2 RotateDirections(vec2 Dir, vec2 CosSin)
{
    return vec2(Dir.x*CosSin.x - Dir.y*CosSin.y,
                  Dir.x*CosSin.y + Dir.y*CosSin.x);
}

void ComputeSteps(inout vec2 stepSizeUv, inout float numSteps, float rayRadiusPix, float rand)
{
    // Avoid oversampling if numSteps is greater than the kernel radius in pixels
    numSteps = min(gtaoParameters.NumSamples, rayRadiusPix);

    // Divide by Ns+1 so that the farthest samples are not fully attenuated
    float stepSizePix = rayRadiusPix / (numSteps + 1);

    // Clamp numSteps if it is greater than the max kernel footprint
    float maxNumSteps = gtaoParameters.MaxRadiusPixels / stepSizePix;
    if (maxNumSteps < numSteps)
    {
        // Use dithering to avoid AO discontinuities
        numSteps = floor(maxNumSteps + rand);
        numSteps = max(numSteps, 1);
        stepSizePix = gtaoParameters.MaxRadiusPixels / numSteps;
    }

    // Step size in uv space
    stepSizeUv = stepSizePix * gtaoParameters.InvAORes;
}

void main(void)
{
	float numDirections = gtaoParameters.NumDirections;

	vec3 P, Pr, Pl, Pt, Pb;
	P 	= GetViewPos(fragTexCoord);

	// Sample neighboring pixels
    Pr 	= GetViewPos(fragTexCoord + vec2( gtaoParameters.InvAORes.x, 0));
    Pl 	= GetViewPos(fragTexCoord + vec2(-gtaoParameters.InvAORes.x, 0));
    Pt 	= GetViewPos(fragTexCoord + vec2( 0, gtaoParameters.InvAORes.y));
    Pb 	= GetViewPos(fragTexCoord + vec2( 0,-gtaoParameters.InvAORes.y));

    // Calculate tangent basis vectors using the minimu difference
    vec3 dPdu = MinDiff(P, Pr, Pl);
    vec3 dPdv = MinDiff(P, Pt, Pb) * (gtaoParameters.AORes.y * gtaoParameters.InvAORes.x);

    // Get the random samples from the noise texture
	vec3 random = texture(randomNoise, fragTexCoord.xy * gtaoParameters.NoiseScale).rgb;

	// Calculate the projected size of the hemisphere
    vec2 rayRadiusUV = 0.5 * gtaoParameters.R * pushConstants.FocalLen / -P.z;
    float rayRadiusPix = rayRadiusUV.x * gtaoParameters.AORes.x;

    float ao = 1.0;

    // Make sure the radius of the evaluated hemisphere is more than a pixel
    if(rayRadiusPix > 1.0)
    {
    	ao = 0.0;
    	float numSteps;
    	vec2 stepSizeUV;

    	// Compute the number of steps
    	ComputeSteps(stepSizeUV, numSteps, rayRadiusPix, random.z);

		float alpha = 2.0 * PI / numDirections;

		// Calculate the horizon occlusion of each direction
		for(float d = 0; d < numDirections; ++d)
		{
			float theta = alpha * d;

			// Apply noise to the direction
			vec2 dir = RotateDirections(vec2(cos(theta), sin(theta)), random.xy);
			vec2 deltaUV = dir * stepSizeUV;

			// Sample the pixels along the direction
			ao += HorizonOcclusion(	deltaUV,
									P,
									dPdu,
									dPdv,
									random.z,
									numSteps);
		}

		// Average the results and produce the final AO
		ao = 1.0 - ao / numDirections * gtaoParameters.AOStrength;
	}

	outColor = ao;
}