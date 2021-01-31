#include "BVHNode.h"

BVHNode::BVHNode(Hittable** l, size_t n, float time0, float time1)
{
    int axis = int(3 * Util::RandomFloat());

    if (axis == 0)
    {
        std::sort(l, l + n, AABB::AABBAxisXComparison);
        //qsort(l, n, sizeof(Hittable*), AABBAxisXComparison);
    }
    else if (axis == 1)
    {
        std::sort(l, l + n, AABB::AABBAxisYComparison);
        // qsort(l, n, sizeof(Hittable*), AABBAxisYComparison);
    }
    else
    {
        std::sort(l, l + n, AABB::AABBAxisZComparison);
        //qsort(l, n, sizeof(Hittable*), AABBAxisZComparison);
    }

    if (n == 1)
    {
        left = right = l[0];
    }
    else if (n == 2)
    {
        left = l[0];
        right = l[1];
    }
    else
    {
        left = new BVHNode(l, n / 2, time0, time1);
        right = new BVHNode(l + n / 2, n - n / 2, time0, time1);
    }

    AABB boxLeft, boxRight;
    if (!left->BoundingBox(time0, time1, boxLeft) || !right->BoundingBox(time0, time1, boxRight))
    {
        std::cerr << "No bounding box";
    }

    box = AABB::SurroundingBox(boxLeft, boxRight);
}

bool BVHNode::Hit(const Ray& r, float tMin, float tMax, HitRecord& hitRecord) const
{
    if (box.RayIntersection(r, tMin, tMax))
    {
        HitRecord leftRec, rightRec;
        bool hitLeft = left->Hit(r, tMin, tMax, leftRec);
        bool hitRight = right->Hit(r, tMin, tMax, rightRec);

        if (hitLeft && hitRight)
        {
            if (leftRec.t < rightRec.t)
            {
                hitRecord = leftRec;
            }
            else
            {
                hitRecord = rightRec;
            }

            return true;
        }
        else if (hitLeft)
        {
            hitRecord = leftRec;
            return true;
        }
        else if (hitRight)
        {
            hitRecord = rightRec;
            return true;
        }
    }

    return false;
}

bool BVHNode::BoundingBox(float t0, float t1, AABB& b) const
{
    b = box;
    return true;
}