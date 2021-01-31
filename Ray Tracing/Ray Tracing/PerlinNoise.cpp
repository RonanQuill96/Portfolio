#include "PerlinNoise.h"

PerlinNoise::PerlinNoise()
{
	randFloat = PerlinNoise::PerlinGenerate();
	permx = PerlinNoise::GeneratePermutation();
	permy = PerlinNoise::GeneratePermutation();
	permz = PerlinNoise::GeneratePermutation();
}

float PerlinNoise::Generate(const Vector3& p) const
{
	float u = p.x - std::floor(p.x);
	float v = p.y - std::floor(p.y);
	float w = p.z - std::floor(p.z);

	int i = static_cast<int>(std::floor(p.x));
	int j = static_cast<int>(std::floor(p.y));
	int k = static_cast<int>(std::floor(p.z));

	float c[2][2][2];

	for (int di = 0; di < 2; di++)
	{
		for (int dj = 0; dj < 2; dj++)
		{
			for (int dk = 0; dk < 2; dk++)
			{
				c[di][dj][dk] = randFloat[
					permx[(i + di) & 255] ^
						permy[(j + dj) & 255] ^
						permz[(k + dk) & 255]
				];
			}
		}
	}

	return TrilinearInterpolation(c, u, v, w);
}

float PerlinNoise::Turbulence(const Vector3& p, int depth) const
{
	float accum = 0.0f;
	Vector3 tempP = p;
	float weight = 1.0f;
	for (int i = 0; i < depth; i++)
	{
		accum += weight * Generate(tempP);
		weight *= 0.5f;
		tempP *= 2.0f;
	}

	return std::abs(accum);
}

float PerlinNoise::TrilinearInterpolation(float c[2][2][2], float u, float v, float w) const
{
	auto accum = 0.0f;

	for (int i = 0; i < 2; i++)
	{
		for (int j = 0; j < 2; j++)
		{
			for (int k = 0; k < 2; k++)
			{
				accum += 
					static_cast<float>(
						(i * u + (1 - i) * (1 - u)) *
						(j * v + (1 - j) * (1 - v)) *
						(k * w + (1 - k) * (1 - w))
					) * c[i][j][k];
			}
		}
	}

	return accum;
}

float* PerlinNoise::PerlinGenerate()
{
	float* randFloat = new float[256];
	for (int i = 0; i < 256; ++i)
	{
		randFloat[i] = Util::RandomFloat();
	}
	return randFloat;
}

void PerlinNoise::Permute(int* p, int n)
{
	for (int i = n - 1; i > 0; --i)
	{
		int target = int(Util::RandomFloat() * (i + 1));
		std::swap(p[i], p[target]);
	}
}

int* PerlinNoise::GeneratePermutation()
{
	int* p = new int[256];

	for (int i = 0; i < 256; ++i)
	{
		p[i] = i;
	}

	Permute(p, 256);

	return p;
}