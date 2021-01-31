#pragma once

#include "Vector3.h"
#include "Util.h"

#include <array>

class PerlinNoise
{
public:
	PerlinNoise();

	float Generate(const Vector3& p) const;
	float Turbulence(const Vector3& p, int depth = 7) const;
	float TrilinearInterpolation(float c[2][2][2], float u, float v, float w) const;
	
private:
	float* randFloat;
	int* permx;
	int* permy;
	int* permz;

	void Permute(int* p, int n);

	float* PerlinGenerate();
	int* GeneratePermutation();
};

