#include "pch.h"
#include "CppUnitTest.h"

#include "../SIMD Math Library/Vector.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace SIMDTests
{
	TEST_CLASS(VectorTests)
	{
	public:
		TEST_METHOD(Construction)
		{
			const float x = 10.0f;
			const float y = 20.0f;
			const float z = 30.0f;
			const float w = 40.0f;

			SIMD::Vector vector(x, y, z, w);

			Assert::AreEqual(x, vector.x, L"X is incorrect");
			Assert::AreEqual(y, vector.y, L"Y is incorrect");
			Assert::AreEqual(z, vector.z, L"Z is incorrect");
			Assert::AreEqual(w, vector.w, L"W is incorrect");

			SIMD::Vector vectorSSE(_mm_set_ps(w, z, y, x));

			Assert::AreEqual(x, vectorSSE.x, L"X is incorrect");
			Assert::AreEqual(y, vectorSSE.y, L"Y is incorrect");
			Assert::AreEqual(z, vectorSSE.z, L"Z is incorrect");
			Assert::AreEqual(w, vectorSSE.w, L"W is incorrect");
		}

		TEST_METHOD(Addition)
		{
			const float x1 = 10.0f;
			const float y1 = 20.0f;
			const float z1 = 30.0f;
			const float w1 = 40.0f;

			SIMD::Vector vector1(x1, y1, z1, w1);

			const float x2 = 50.0f;
			const float y2 = 13.0f;
			const float z2 = 10.0f;
			const float w2 = 16.86f;
			SIMD::Vector vector2(x2, y2, z2, w2);

			const float expectedX = x1 + x2;
			const float expectedY = y1 + y2;
			const float expectedZ = z1 + z2;
			const float expectedW = w1 + w2;

			SIMD::Vector result = vector1 + vector2;
			Assert::AreEqual(expectedX, result.x, L"X is incorrect");
			Assert::AreEqual(expectedY, result.y, L"Y is incorrect");
			Assert::AreEqual(expectedZ, result.z, L"Z is incorrect");
			Assert::AreEqual(expectedW, result.w, L"W is incorrect");
		}

		TEST_METHOD(Subtraction)
		{
			const float x1 = 10.0f;
			const float y1 = 20.0f;
			const float z1 = 30.0f;
			const float w1 = 40.0f;
			SIMD::Vector vector1(x1, y1, z1, w1);

			const float x2 = 50.0f;
			const float y2 = 13.0f;
			const float z2 = 10.0f;
			const float w2 = 16.86f;
			SIMD::Vector vector2(x2, y2, z2, w2);

			const float expectedX = x1 - x2;
			const float expectedY = y1 - y2;
			const float expectedZ = z1 - z2;
			const float expectedW = w1 - w2;

			SIMD::Vector result = vector1 - vector2;
			Assert::AreEqual(expectedX, result.x, L"X is incorrect");
			Assert::AreEqual(expectedY, result.y, L"Y is incorrect");
			Assert::AreEqual(expectedZ, result.z, L"Z is incorrect");
			Assert::AreEqual(expectedW, result.w, L"W is incorrect");
		}

		TEST_METHOD(Multiplication)
		{
			const float x1 = 10.0f;
			const float y1 = 20.0f;
			const float z1 = 30.0f;
			const float w1 = 40.0f;
			SIMD::Vector vector1(x1, y1, z1, w1);

			const float x2 = 7.5f;
			const float y2 = 0.25f;
			const float z2 = 4.0f;
			const float w2 = 2.0f;
			SIMD::Vector vector2(x2, y2, z2, w2);

			const float expectedX = x1 * x2;
			const float expectedY = y1 * y2;
			const float expectedZ = z1 * z2;
			const float expectedW = w1 * w2;

			SIMD::Vector result = vector1 * vector2;

			Assert::AreEqual(expectedX, result.x, L"X is incorrect");
			Assert::AreEqual(expectedY, result.y, L"Y is incorrect");
			Assert::AreEqual(expectedZ, result.z, L"Z is incorrect");
			Assert::AreEqual(expectedW, result.w, L"W is incorrect");
		}

		TEST_METHOD(Division)
		{
			const float x1 = 10.0f;
			const float y1 = 20.0f;
			const float z1 = 30.0f;
			const float w1 = 40.0f;
			SIMD::Vector vector1(x1, y1, z1, w1);

			const float x2 = 7.5f;
			const float y2 = 0.25f;
			const float z2 = 4.0f;
			const float w2 = 2.0f;
			SIMD::Vector vector2(x2, y2, z2, w2);

			const float expectedX = x1 / x2;
			const float expectedY = y1 / y2;
			const float expectedZ = z1 / z2;
			const float expectedW = w1 / w2;

			SIMD::Vector result = vector1 / vector2;

			Assert::AreEqual(expectedX, result.x, L"X is incorrect");
			Assert::AreEqual(expectedY, result.y, L"Y is incorrect");
			Assert::AreEqual(expectedZ, result.z, L"Z is incorrect");
			Assert::AreEqual(expectedW, result.w, L"W is incorrect");
		}

		TEST_METHOD(CrossProduct3)
		{
			SIMD::Vector vector1(5.0f, 4.0f, 7.0f, 0.0f);
			SIMD::Vector vector2(9.0f, 3.0f, 11.0f, 0.0f);

			SIMD::Vector result = SIMD::Vector::CrossProduct3(vector1, vector2);

			Assert::AreEqual(23.0f, result.x, L"X is incorrect");
			Assert::AreEqual(8.0f, result.y, L"Y is incorrect");
			Assert::AreEqual(-21.0f, result.z, L"Z is incorrect");
			Assert::AreEqual(0.0f, result.w, L"W is incorrect");
		}

		TEST_METHOD(DotProduct)
		{
			SIMD::Vector vector1(5.0f, 4.0f, 7.0f, 0.0f);
			SIMD::Vector vector2(9.0f, 3.0f, 11.0f, 0.0f);

			float expectedResult = vector1.x * vector2.x + vector1.y * vector2.y + vector1.z * vector2.z + vector1.w * vector2.w;

			float result = SIMD::Vector::DotProduct(vector1, vector2);

			Assert::AreEqual(expectedResult, result, L"Result is incorrect");
		}

		TEST_METHOD(Equality)
		{
			const float x1 = 10.0f;
			const float y1 = 20.0f;
			const float z1 = 30.0f;
			const float w1 = 40.0f;

			SIMD::Vector vector1(x1, y1, z1, w1);
			SIMD::Vector vector2(x1, y1, z1, w1);

			Assert::IsTrue(SIMD::Vector::ExactCompare(vector1, vector2), L"Vectors are not equal");
		}
	};
}
