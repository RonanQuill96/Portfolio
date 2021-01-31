#include "ConstantColour.h"

ConstantColour::ConstantColour(Vector3 c) 
	: colour(c) {}

Vector3 ConstantColour::Value(float u, float v, const Vector3& p) const
{
	return colour;
}
