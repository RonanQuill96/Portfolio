#pragma once

#include "Hittable.h"
#include "HittableList.h"

#include "XYRectangle.h"
#include "XZRectangle.h"
#include "YZRectangle.h"

class Box : public Hittable
{
public:
    Box() = default;
    Box(Vector3 p0, Vector3 p1, Material* material);

    bool Hit(const Ray& r, float tMin, float tMax, HitRecord& hitRecord) const override;
    bool BoundingBox(float t0, float t1, AABB& box) const override;

private:
    Vector3 box_min;
    Vector3 box_max;

    HittableList* sides;
};
