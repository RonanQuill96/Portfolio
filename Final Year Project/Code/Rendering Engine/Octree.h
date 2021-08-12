#pragma once

#include "AABB.h"
#include "BoundingVolumes.h"
#include "Frustum.h"

#include <queue>
#include <array>
#include <unordered_set>
#include <vector>
#include <memory>

#include "MeshComponent.h"

template<class DataType>
class Octree
{
public:
	explicit Octree(AABB bounds);
	explicit Octree(Octree* pParent);
	~Octree();

	void Update();

	void Insert(DataType* object);
	bool Remove(DataType* object);
	void AddToUpdateSet(DataType* object);

	void GetAllObjects(std::vector<DataType*>& objectVector) const;

	void FrustumCollision(const Frustum& frustum, std::vector<DataType*>& collisions) const;
	void AABBCollision(const AABB& bounds, std::vector<DataType*>& collisions) const;

	std::vector<AABB> GetAllRegions();
private:
	const unsigned int minObjectsPerRegion = 5;

	Octree* parent = nullptr;
	std::array<std::unique_ptr<Octree>, 8> child;

	AABB bounds;

	uint16_t childCount = 0;

	std::vector<DataType*> objects;

	std::vector<DataType*> updateSet; //holds all objects that need to be updated
	std::queue<DataType*> updateQueue; //holds only objects in this region that need updating

	static constexpr unsigned int minRegionSize = 1;

	int maxLifeSpan = 8;
	int currentLife = -1;
};


template<class DataType>
Octree<DataType>::Octree(AABB desiredBounds)
{
	bounds = desiredBounds;

	std::fill(child.begin(), child.end(), nullptr);// child = { nullptr };
}

template<class DataType>
Octree<DataType>::Octree(Octree* pParent)
{
	parent = pParent;
	std::fill(child.begin(), child.end(), nullptr);
}

template<class DataType>
Octree<DataType>::~Octree()
{
}

template<class DataType>
void Octree<DataType>::Insert(DataType* object)
{
	//If this node is empty there is no need to create extra nodes so just insert here
	if (objects.size() < minObjectsPerRegion && childCount == 0)
	{
		objects.emplace_back(object);
		return;
	}

	//if this region is the min size insert here
	glm::vec3 dimensions = bounds.GetDimensions();
	if (dimensions.x <= minRegionSize || dimensions.y <= minRegionSize || dimensions.z <= minRegionSize)
	{
		objects.emplace_back(object);
		return;
	}

	glm::vec3  half = dimensions * 0.5f; //divide by 2
	glm::vec3  center = bounds.minVertex + half;

	std::array<AABB, 8> octantBounds;

	octantBounds[0] = child[0] != nullptr ? child[0]->bounds : AABB(bounds.minVertex, center);
	octantBounds[1] = child[1] != nullptr ? child[1]->bounds : AABB(glm::vec3(center.x, bounds.minVertex.y, bounds.minVertex.z), glm::vec3(bounds.maxVertex.x, center.y, center.z));;
	octantBounds[2] = child[2] != nullptr ? child[2]->bounds : AABB(glm::vec3(center.x, bounds.minVertex.y, center.z), glm::vec3(bounds.maxVertex.x, center.y, bounds.maxVertex.z));
	octantBounds[3] = child[3] != nullptr ? child[3]->bounds : AABB(glm::vec3(bounds.minVertex.x, bounds.minVertex.y, center.z), glm::vec3(center.x, center.y, bounds.maxVertex.z));
	octantBounds[4] = child[4] != nullptr ? child[4]->bounds : AABB(glm::vec3(bounds.minVertex.x, center.y, bounds.minVertex.z), glm::vec3(center.x, bounds.maxVertex.y, center.z));
	octantBounds[5] = child[5] != nullptr ? child[5]->bounds : AABB(glm::vec3(center.x, center.y, bounds.minVertex.z), glm::vec3(bounds.maxVertex.x, bounds.maxVertex.y, center.z));
	octantBounds[6] = child[6] != nullptr ? child[6]->bounds : AABB(center, bounds.maxVertex);
	octantBounds[7] = child[7] != nullptr ? child[7]->bounds : AABB(glm::vec3(bounds.minVertex.x, center.y, center.z), glm::vec3(center.x, bounds.maxVertex.y, bounds.maxVertex.z));

	//if this region contains this object, insert into the correct child
	if (BoundingVolume::AABBContains(bounds, object->GetBoundingVolumeWorldSpace()))
	{
		for (size_t index = 0; index < 8; index++)
		{
			if (BoundingVolume::AABBContains(octantBounds[index], object->GetBoundingVolumeWorldSpace()))
			{
				if (child[index] == nullptr)
				{
					child[index] = std::make_unique<Octree>(this);
					child[index]->bounds = octantBounds[index];
					//childInUseMask |= static_cast<char>(1 << index);
					childCount++;
				}

				child[index]->Insert(object);

				return;
			}
		}

		//no region is suitable, insert here instead
		//if (objects.find(object) == objects.end())
		objects.emplace_back(object);
	}
	else if (parent != nullptr)
	{
		parent->Insert(object);
	}
	else
	{
		//TODO
		//we need to resize the tree
	}
}

template<class DataType>
bool Octree<DataType>::Remove(DataType* object)
{
	const auto updateSetPosition = std::find(updateSet.begin(), updateSet.end(), object);
	if (updateSetPosition != updateSet.end())
	{
		updateSet.erase(updateSetPosition);
	}

	//if this region contains this object, remove it from the correct child
	if (BoundingVolume::AABBContains(bounds, object->GetBoundingVolumeWorldSpace()))
	{
		//search this node
		const auto objectPosition = std::find(objects.begin(), objects.end(), object);
		if (objectPosition != objects.end())
		{
			objects.erase(objectPosition);
			return true;
		}

		//search children
		for (size_t index = 0; index < 8; index++)
		{
			if (child[index] != nullptr)
			{
				if (BoundingVolume::AABBContains(child[index]->bounds, object->GetBoundingVolumeWorldSpace()))
				{
					return child[index]->Remove(object);
				}
			}
		}
	}
	else if (parent != nullptr)
	{
		return parent->Remove(object);
	}

	return false;
}

template<class DataType>
void Octree<DataType>::AddToUpdateSet(DataType* object)
{
	if (std::find(updateSet.begin(), updateSet.end(), object) == updateSet.end())
	{
		updateSet.emplace_back(object);
	}
}

template<class DataType>
void Octree<DataType>::GetAllObjects(std::vector<DataType*>& objectVector) const
{
	if (objects.empty() && childCount == 0)
	{
		return;
	}

	for (DataType* object : objects)
	{
		objectVector.push_back(object);
	}

	//search children
	for (size_t index = 0; index < 8; index++)
	{
		if (child[index])
		{
			child[index]->GetAllObjects(objectVector);
		}
	}
}

template<class DataType>
void Octree<DataType>::FrustumCollision(const Frustum& frustum, std::vector<DataType*>& collisions) const
{
	if (objects.empty() && childCount == 0)
	{
		return;
	}

	for (DataType* object : objects)
	{
		if (BoundingVolume::AABBFustrumCollision(object->GetBoundingVolumeWorldSpace(), frustum))
		{
			collisions.push_back(object);
		}
	}

	//search children
	for (size_t index = 0; index < 8; index++)
	{
		if (child[index])
		{
			if (BoundingVolume::AABBFustrumCollision(child[index]->bounds, frustum))
			{
				child[index]->FrustumCollision(frustum, collisions);
			}
		}
	}
}

template<class DataType>
void Octree<DataType>::AABBCollision(const AABB& subject, std::vector<DataType*>& collisions) const
{
	if (objects.empty() && childCount == 0)
	{
		return;
	}

	for (DataType* object : objects)
	{
		if (BoundingVolume::AABBOverlap(object->GetBoundingVolumeWorldSpace(), subject))
		{
			collisions.push_back(object);
		}
	}

	//search children
	for (size_t index = 0; index < 8; index++)
	{
		if (child[index])
		{
			if (BoundingVolume::AABBOverlap(child[index]->bounds, subject))
			{
				child[index]->AABBCollision(subject, collisions);
			}
		}
	}
}

template<class DataType>
std::vector<AABB> Octree<DataType>::GetAllRegions()
{
	std::vector<AABB> regions = { bounds };

	if (objects.size() == 0 && childCount == 0)
		return regions;

	//search children
	for (size_t index = 0; index < 8; index++)
	{
		if (child[index])
		{
			std::vector<AABB> childRegions = child[index]->GetAllRegions();

			//merge these vectors
			regions.reserve(regions.size() + childRegions.size());
			regions.insert(regions.end(), childRegions.begin(), childRegions.end());
		}
	}

	return regions;
}

template<class DataType>
void Octree<DataType>::Update()
{
	if (objects.size() == 0 && childCount == 0)
	{
		if (currentLife == -1)
		{
			currentLife = maxLifeSpan;
		}
		else if (currentLife > 0)
		{
			currentLife--;
		}
	}
	else
	{
		if (currentLife != -1)
		{
			if (maxLifeSpan <= 64)
			{
				maxLifeSpan *= 2;
			}
			currentLife = -1;
		}
	}

	if (updateSet.size() > 0)
	{
		for (const auto object : objects)
		{
			auto position = std::find(updateSet.begin(), updateSet.end(), object);
			if (position != updateSet.end())
			{
				updateQueue.push(object);
				updateSet.erase(position);
			}
		}
	}

	for (auto& currentChild : child)
	{
		if (currentChild != nullptr)
		{
			currentChild->updateSet = updateSet;
			currentChild->Update();
		}
	}
	updateSet.clear();

	while (!updateQueue.empty())
	{
		DataType* object = updateQueue.front();
		updateQueue.pop();

		Octree* current = this;
		while (!BoundingVolume::AABBContains(current->bounds, object->GetBoundingVolumeWorldSpace()) && current->parent != nullptr)
		{
			current = current->parent;
		}

		auto position = std::find(objects.begin(), objects.end(), object);
		objects.erase(position);
		current->Insert(object);
	}

	for (unsigned short index = 0; index < 8; index++)
	{
		if (child[index] != nullptr && child[index]->currentLife == 0)
		{
			child[index] = nullptr;
			childCount--;
			//childInUseMask ^= static_cast<char>(1 << index); //remove the node from the active nodes flag list
		}
	}

}

