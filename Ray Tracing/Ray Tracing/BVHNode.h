#pragma once

#include "AABB.h"
#include "HittableList.h"
#include "Util.h"

#include <iostream>

class BVHNode : public HittableList
{
public:
    BVHNode() = default;
    BVHNode(Hittable** l, size_t n, float time0, float time1);

    bool Hit(const Ray& r, float tMin, float tMax, HitRecord& hitRecord) const override;
    bool BoundingBox(float t0, float t1, AABB& b) const override;

private:
    Hittable* left;
    Hittable* right;
    AABB box;
};