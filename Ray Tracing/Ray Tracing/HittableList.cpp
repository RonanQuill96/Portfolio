#include "HittableList.h"

HittableList::HittableList(Hittable** l, int n)
{
	list = l;
	size = n;
}

bool HittableList::Hit(const Ray& r, float tMin, float tMax, HitRecord& hitRecord) const
{
	HitRecord tempHitRecord;
	bool hitAnything = false;
	double closestDistance = tMax;

	for (int i = 0; i < size; i++)
	{
		if (list[i]->Hit(r, tMin, closestDistance, tempHitRecord))
		{
			hitAnything = true;
			closestDistance = tempHitRecord.t;
			hitRecord = tempHitRecord;
		}
	}

	return hitAnything;
}

bool HittableList::BoundingBox(float t0, float t1, AABB& box) const
{
	if (size < 1)
	{
		return false;
	}

	AABB tempBox;
	bool firstTrue = list[0]->BoundingBox(t0, t1, tempBox);

	if (!firstTrue)
	{
		return false;
	}
	else
	{
		box = tempBox;
	}

	for (int i = 1; i < size; i++)
	{
		if (list[i]->BoundingBox(t0, t1, tempBox))
		{
			box = AABB::SurroundingBox(box, tempBox);
		}
		else
		{
			return false;
		}
	}
}