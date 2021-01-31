#pragma once

#include "Vector.h"

namespace SIMD
{
	struct Matrix
	{
		Vector m[4];

		Matrix()
		{
			this->m[0] = Vector(1.0f, 0.0f, 0.0f, 0.0f);
			this->m[1] = Vector(0.0f, 1.0f, 0.0f, 0.0f);
			this->m[2] = Vector(0.0f, 0.0f, 1.0f, 0.0f);
			this->m[3] = Vector(0.0f, 0.0f, 0.0f, 1.0f);
		}

		Matrix(float a, float b, float c, float d,
				float e, float f, float g, float h, 
				float i, float j, float k, float l,
				float m, float n, float o, float p)
		{
			this->m[0] = Vector(a, b, c, d);
			this->m[1] = Vector(e, f, g, h);
			this->m[2] = Vector(i, j, k, l);
			this->m[3] = Vector(m, n, o, p);
		}

		Matrix(Vector row0, Vector row1, Vector row2, Vector row3)
		{
			this->m[0] = row0;
			this->m[1] = row1;
			this->m[2] = row2;
			this->m[3] = row3;
		}

		Matrix operator+(const Matrix& param) const
		{
			Matrix result;

			result.m[0] = this->m[0] + param.m[0];
			result.m[1] = this->m[1] + param.m[1];
			result.m[2] = this->m[2] + param.m[2];
			result.m[3] = this->m[3] + param.m[3];

			return result;
		}

		Matrix operator-(const Matrix& param) const
		{
			Matrix result;

			result.m[0] = this->m[0] - param.m[0];
			result.m[1] = this->m[1] - param.m[1];
			result.m[2] = this->m[2] - param.m[2];
			result.m[3] = this->m[3] - param.m[3];

			return result;
		}

		Matrix operator*(const Matrix& param) const
		{
			Matrix result;

			//Simulate the dot product operations used in matrix multiplication while maximizing SIMD throughput
			//Row 0
			Vector w = this->m[0];
			Vector x = Vector::Shuffle<0, 0, 0, 0>(w.v);
			Vector y = Vector::Shuffle<1, 1, 1, 1>(w.v);
			Vector z = Vector::Shuffle<2, 2, 2, 2>(w.v);
			w = Vector::Shuffle<3, 3, 3, 3>(w.v);

			x = x * param.m[0];
			y = y * param.m[1];
			z = z * param.m[2];
			w = w * param.m[3];

			x = x + z;
			y = y + w;
			x = x + y;

			result.m[0] = x;

			//Row 1
			w = this->m[1];
			x = Vector::Shuffle<0, 0, 0, 0>(w.v);
			y = Vector::Shuffle<1, 1, 1, 1>(w.v);
			z = Vector::Shuffle<2, 2, 2, 2>(w.v);
			w = Vector::Shuffle<3, 3, 3, 3>(w.v);

			x = x * param.m[0];
			y = y * param.m[1];
			z = z * param.m[2];
			w = w * param.m[3];

			x = x + z;
			y = y + w;
			x = x + y;

			result.m[1] = x;

			//Row 2
			w = this->m[2];
			x = Vector::Shuffle<0, 0, 0, 0>(w.v);
			y = Vector::Shuffle<1, 1, 1, 1>(w.v);
			z = Vector::Shuffle<2, 2, 2, 2>(w.v);
			w = Vector::Shuffle<3, 3, 3, 3>(w.v);

			x = x * param.m[0];
			y = y * param.m[1];
			z = z * param.m[2];
			w = w * param.m[3];

			x = x + z;
			y = y + w;
			x = x + y;

			result.m[2] = x;

			//Row 3
			w = this->m[3];
			x = Vector::Shuffle<0, 0, 0, 0>(w.v);
			y = Vector::Shuffle<1, 1, 1, 1>(w.v);
			z = Vector::Shuffle<2, 2, 2, 2>(w.v);
			w = Vector::Shuffle<3, 3, 3, 3>(w.v);

			x = x * param.m[0];
			y = y * param.m[1];
			z = z * param.m[2];
			w = w * param.m[3];

			x = x + z;
			y = y + w;
			x = x + y;

			result.m[3] = x;

			return result;
		}

		//Static functions
		static Matrix Transpose(const Matrix& param)
		{
			Matrix result;

			// x.x,x.y,y.x,y.y
			Vector temp1 = Vector::Shuffle<0, 1, 0, 1>(param.m[0], param.m[1]);
			// x.z,x.w,y.z,y.w
			Vector temp3 = Vector::Shuffle<2, 3, 2, 3>(param.m[0], param.m[1]);
			// z.x,z.y,w.x,w.y
			Vector temp2 = Vector::Shuffle<0, 1, 0, 1>(param.m[2], param.m[3]);
			// z.z,z.w,w.z,w.w
			Vector temp4 = Vector::Shuffle<2, 3, 2, 3>(param.m[2], param.m[3]);

			// x.x,y.x,z.x,w.x
			result.m[0] = Vector::Shuffle<0, 2, 0, 2>(temp1, temp2);
			// x.y,y.y,z.y,w.y
			result.m[1] = Vector::Shuffle<1, 3, 1, 3>(temp1, temp2);
			// x.z,y.z,z.z,w.z
			result.m[2] = Vector::Shuffle<0, 2, 0, 2>(temp3, temp4);
			// x.w,y.w,z.w,w.w
			result.m[3] = Vector::Shuffle<1, 3, 1, 3>(temp3, temp4);

			return result;
		}

		static Matrix Inverse(const Matrix& param)
		{
			Matrix MT = Transpose(param);

			Vector V00 = Vector::Shuffle<0, 0, 1, 1>(MT.m[2]);
			Vector V10 = Vector::Shuffle<2, 3, 2, 3>(MT.m[3]);
			Vector V01 = Vector::Shuffle<0, 0, 1, 1>(MT.m[0]);
			Vector V11 = Vector::Shuffle<2, 3, 2, 3>(MT.m[1]);

			Vector V02 = Vector::Shuffle<0, 2, 0, 2>(MT.m[2], MT.m[0]);
			Vector V12 = Vector::Shuffle<1, 3, 1, 3>(MT.m[3], MT.m[1]);

			Vector D0 = V00 * V10;
			Vector D1 = V01 * V11;
			Vector D2 = V02 * V12;

			V00 = Vector::Shuffle<2, 3, 2, 3>(MT.m[2]);
			V10 = Vector::Shuffle<0, 0, 1, 1>(MT.m[3]);
			V01 = Vector::Shuffle<2, 3, 2, 3>(MT.m[0]);
			V11 = Vector::Shuffle<0, 0, 1, 1>(MT.m[1]);

			V02 = Vector::Shuffle<1, 3, 1, 3>(MT.m[2], MT.m[0]);
			V12 = Vector::Shuffle<0, 2, 0, 2>(MT.m[3], MT.m[1]);
			
			V00 = V00 * V10;
			V01 = V01 * V11;
			V02 = V02 * V12;

			D0 = D0 - V00;
			D1 = D1 - V01;
			D2 = D2 - V02;

			V11 = Vector::Shuffle<1, 3, 1, 1>(D0, D2);
			V00 = Vector::Shuffle<1, 2, 0, 1>(MT.m[1]);
			V10 = Vector::Shuffle<2, 0, 3, 0>(V11, D0);
			V01 = Vector::Shuffle<2, 0, 1, 0>(MT.m[0]);
			V11 = Vector::Shuffle<1, 2, 1, 2>(V11, D0);

			Vector V13 = Vector::Shuffle<1, 3, 3, 3>(D1, D2);
			V02 = Vector::Shuffle<1, 2, 0, 1>(MT.m[3]);
			V12 = Vector::Shuffle<2, 0, 3, 0>(V13, D1);
			Vector V03 = Vector::Shuffle<2, 0, 1, 0>(MT.m[2]);
			V13 = Vector::Shuffle<1, 2, 1, 2>(V13, D1);

			Vector C0 = V00 * V10;
			Vector C2 = V01 * V11;
			Vector C4 = V02 * V12;
			Vector C6 = V03 * V13;

			V11 = Vector::Shuffle<0, 1, 0, 0>(D0, D2);
			V00 = Vector::Shuffle<2, 3, 1, 2>(MT.m[1]);
			V10 = Vector::Shuffle<3, 0, 1, 2>(D0, V11);
			V01 = Vector::Shuffle<3, 2, 3, 1>(MT.m[0]);
			V11 = Vector::Shuffle<2, 1, 2, 0>(D0, V11);

			V13 = Vector::Shuffle<0, 1, 2, 2>(D1, D2);
			V02 = Vector::Shuffle<2, 3, 1, 2>(MT.m[3]);
			V12 = Vector::Shuffle<3, 0, 1, 2>(D1, V13);
			V03 = Vector::Shuffle<3, 2, 3, 1>(MT.m[2]);
			V13 = Vector::Shuffle<2, 1, 2, 0>(D1, V13);

			V00 = V00 * V10;
			V01 = V01 * V11;
			V02 = V02 * V12;
			V03 = V03 * V13;

			C0 = C0 - V00;
			C2 = C2 - V01;
			C4 = C4 - V02;
			C6 = C6 - V03;

			V00 = Vector::Shuffle<3, 0, 3, 0>(MT.m[1]);

			V10 = Vector::Shuffle<2, 2, 0, 1>(D0, D2);
			V10 = Vector::Shuffle<0, 3, 2, 0>(V10);
			V01 = Vector::Shuffle<1, 3, 0, 2>(MT.m[0]);

			V11 = Vector::Shuffle<0, 3, 0, 1>(D0, D2);
			V11 = Vector::Shuffle<3, 0, 1, 2>(V11);
			V02 = Vector::Shuffle<3, 0, 3, 0>(MT.m[3]);

			V12 = Vector::Shuffle<2, 2, 2, 3>(D1, D2);
			V12 = Vector::Shuffle<0, 3, 2, 0>(V12);
			V03 = Vector::Shuffle<1, 3, 0, 2>(MT.m[2]);

			V13 = Vector::Shuffle<0, 3, 2, 3>(D1, D2);
			V13 = Vector::Shuffle<3, 0, 1, 2>(V13);

			V00 = V00 * V10;
			V01 = V01 * V11;
			V02 = V02 * V12;
			V03 = V03 * V13;

			Vector C1 = C0 - V00;
			C0 = C0 + V00;
			Vector C3 = C2 + V01;
			C2 = C2 - V01;
			Vector C5 = C4 - V02;
			C4 = C4 + V02;
			Vector C7 = C6 + V03;
			C6 = C6 - V03;

			C0 = Vector::Shuffle<0, 2, 1, 3>(C0, C1);
			C2 = Vector::Shuffle<0, 2, 1, 3>(C2, C3);
			C4 = Vector::Shuffle<0, 2, 1, 3>(C4, C5);
			C6 = Vector::Shuffle<0, 2, 1, 3>(C6, C7);
			C0 = Vector::Shuffle<0, 2, 1, 3>(C0);
			C2 = Vector::Shuffle<0, 2, 1, 3>(C2);
			C4 = Vector::Shuffle<0, 2, 1, 3>(C4);
			C6 = Vector::Shuffle<0, 2, 1, 3>(C6);

			Matrix result;

			float determinant = Vector::DotProduct(C0, MT.m[0]);
			float inverseDeterminent = 1.0f / determinant;
			Vector inverseDeterminentV = { inverseDeterminent, inverseDeterminent, inverseDeterminent, inverseDeterminent };

			result.m[0] = C0 * inverseDeterminentV;
			result.m[1] = C2 * inverseDeterminentV;
			result.m[2] = C4 * inverseDeterminentV;
			result.m[3] = C6 * inverseDeterminentV;

			return result;
		}
	};
}