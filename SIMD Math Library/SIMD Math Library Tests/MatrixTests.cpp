#include "pch.h"
#include "CppUnitTest.h"

#include "../SIMD Math Library/Matrix.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace SIMDTests
{
	TEST_CLASS(MatrixTests)
	{
	public:

		TEST_METHOD(Multiplication)
		{
			SIMD::Matrix M1 = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
			SIMD::Matrix M2 = {16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1};

			SIMD::Matrix result = M1 * M2;

			SIMD::Vector expectedRow0 = {80.0f, 70.0f, 60.0f, 50.0f};
			SIMD::Vector expectedRow1 = {240.0f, 214.0f, 188.0f, 162.0f};
			SIMD::Vector expectedRow2 = {400.0f, 358.0f, 316.0f, 274.0f};
			SIMD::Vector expectedRow3 = {560.0f, 502.0f, 444.0f, 386.0f};

			Assert::IsTrue(SIMD::Vector::ExactCompare(expectedRow0, result.m[0]), L"Vectors are not equal");
			Assert::IsTrue(SIMD::Vector::ExactCompare(expectedRow1, result.m[1]), L"Vectors are not equal");
			Assert::IsTrue(SIMD::Vector::ExactCompare(expectedRow2, result.m[2]), L"Vectors are not equal");
			Assert::IsTrue(SIMD::Vector::ExactCompare(expectedRow3, result.m[3]), L"Vectors are not equal");
		}

		TEST_METHOD(Transpose)
		{
			SIMD::Matrix M1 = { 1, 2, 3, 4, 
								5, 6, 7, 8, 
								9, 10, 11, 12, 
								13, 14, 15, 16 };

			SIMD::Matrix result = SIMD::Matrix::Transpose(M1);

			SIMD::Vector expectedRow0 = { 1, 5, 9, 13 };
			SIMD::Vector expectedRow1 = { 2, 6, 10, 14 };
			SIMD::Vector expectedRow2 = { 3, 7, 11, 15 };
			SIMD::Vector expectedRow3 = { 4, 8, 12, 16 };

			Assert::IsTrue(SIMD::Vector::ExactCompare(expectedRow0, result.m[0]), L"Vectors are not equal");
			Assert::IsTrue(SIMD::Vector::ExactCompare(expectedRow1, result.m[1]), L"Vectors are not equal");
			Assert::IsTrue(SIMD::Vector::ExactCompare(expectedRow2, result.m[2]), L"Vectors are not equal");
			Assert::IsTrue(SIMD::Vector::ExactCompare(expectedRow3, result.m[3]), L"Vectors are not equal");
		}

		TEST_METHOD(Inverse)
		{
			SIMD::Matrix M1 = { 4, 0, 0, 0,
								0, 0, 2, 0,
								0, 1, 2, 0,
								1, 0, 0, 1 };

			SIMD::Matrix result = SIMD::Matrix::Inverse(M1);

			SIMD::Vector expectedRow0 = { 0.25f, 0, 0, 0 };
			SIMD::Vector expectedRow1 = { 0, -1, 1, 0 };
			SIMD::Vector expectedRow2 = { 0, 0.5f, 0, 0 };
			SIMD::Vector expectedRow3 = { -0.25, 0, 0, 1 };

			Assert::IsTrue(SIMD::Vector::ExactCompare(expectedRow0, result.m[0]), L"Vectors are not equal");
			Assert::IsTrue(SIMD::Vector::ExactCompare(expectedRow1, result.m[1]), L"Vectors are not equal");
			Assert::IsTrue(SIMD::Vector::ExactCompare(expectedRow2, result.m[2]), L"Vectors are not equal");
			Assert::IsTrue(SIMD::Vector::ExactCompare(expectedRow3, result.m[3]), L"Vectors are not equal");
		}
	};
}
