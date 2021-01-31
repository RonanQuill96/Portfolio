#pragma once

#include <cmath>
#include <string>
#include <xmmintrin.h>

namespace SIMD
{
	class Vector
	{
	public:
		union
		{
			__m128 v;
			struct
			{
				float x, y, z, w;
			};
		};

		Vector()
		{
			x = 0.0f;
			y = 0.0f;
			z = 0.0f;
			w = 0.0f;
		}

		Vector(float pX, float pY, float pZ, float pW)
		{
			x = pX;
			y = pY;
			z = pZ;
			w = pW;
		}

		Vector(__m128 pV)
		{
			v = pV;
		}			

		Vector operator-() const
		{
			return Vector(-x, -y, -z, -w);
		}

		Vector operator+(const Vector& param) const
		{
			return _mm_add_ps(v, param.v);
		}

		Vector operator-(const Vector& param) const
		{
			return _mm_sub_ps(v, param.v);
		}

		Vector operator*(const Vector& param) const
		{
			return _mm_mul_ps(v, param.v);
		}
		
		Vector operator/(const Vector& param) const
		{
			return _mm_div_ps(v, param.v);
		}

		float Lenght() const
		{
			return std::sqrt(LengthSquared());
		}

		float LengthSquared() const
		{
			Vector temp = *this;
			temp = temp * temp;
			return temp.x + temp.y + temp.z + temp.w;
		}

		Vector Sqrt() const
		{
			return _mm_sqrt_ps(v);
		}

		std::string ToString() const
		{
			return std::to_string(x) + " " + std::to_string(y) + " " + std::to_string(z) + " " + std::to_string(w);
		}

		//Static functions
		template<unsigned int X, unsigned int Y, unsigned int Z, unsigned int W>
		static inline Vector Shuffle(const Vector& param)
		{
			return _mm_shuffle_ps(param.v, param.v, _MM_SHUFFLE(W, Z, Y, X));
		}

		template<unsigned int X, unsigned int Y, unsigned int Z, unsigned int W>
		static inline Vector Shuffle(const Vector& V1, const Vector& V2)
		{
			return _mm_shuffle_ps(V1.v, V2.v, _MM_SHUFFLE(W, Z, Y, X));
		}

		static bool ExactCompare(const Vector& v1, const Vector& v2)
		{
			return (v1 - v2).LengthSquared() == 0.0f;
		}

		static Vector CrossProduct3(const Vector& v1, const Vector& v2)
		{
			// y z x w
			Vector temp1 = Shuffle<1, 2, 0, 3>(v1);
			// z x y w
			Vector temp2 = Shuffle<2, 0, 1, 3>(v2);

			Vector result = temp1 * temp2;

			// z x y w
			temp1 = Shuffle<1, 2, 0, 3>(temp1);
			// y z x w
			temp2 = Shuffle<2, 0, 1, 3>(temp2);

			result = result - (temp1 * temp2);

			result.w = 0.0f;

			return result;
		}

		static float DotProduct(const Vector& v1, const Vector& v2)
		{
			Vector dot = v1 * v2;

			return dot.x + dot.y + dot.z + dot.w;
		}
	};
}