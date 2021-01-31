#include "Box.h"

Box::Box(Vector3 p0, Vector3 p1, Material* material)
    : box_min(p0), box_max(p1)
{
    constexpr size_t SideCount = 6;
    Hittable** temp_sides = new Hittable * [SideCount];
    int i = 0;

    temp_sides[i++] = new XYRectangle(p0.x, p1.x, p0.y, p1.y, p1.z, material);
    temp_sides[i++] = new XYRectangle(p0.x, p1.x, p0.y, p1.y, p0.z, material);

    temp_sides[i++] = new XZRectangle(p0.x, p1.x, p0.z, p1.z, p1.y, material);
    temp_sides[i++] = new XZRectangle(p0.x, p1.x, p0.z, p1.z, p0.y, material);

    temp_sides[i++] = new YZRectangle(p0.y, p1.y, p0.z, p1.z, p1.x, material);
    temp_sides[i++] = new YZRectangle(p0.y, p1.y, p0.z, p1.z, p0.x, material);

    sides = new HittableList(temp_sides, SideCount);
}

bool Box::Hit(const Ray& r, float tMin, float tMax, HitRecord& hitRecord) const
{
    return sides->Hit(r, tMin, tMax, hitRecord);
}

bool Box::BoundingBox(float t0, float t1, AABB& box) const
{
    box = AABB(box_min, box_max);
    return true;
}
