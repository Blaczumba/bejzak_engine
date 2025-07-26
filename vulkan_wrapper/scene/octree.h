#pragma once

#include "vulkan_wrapper/object/object.h"
#include "vulkan_wrapper/primitives/geometry.h"

#include <array>
#include <memory>
#include <vector>

constexpr size_t NUM_OCTREE_NODE_CHILDREN = 8;

class OctreeNode {
	AABB _volume;
	std::array<std::unique_ptr<OctreeNode>, NUM_OCTREE_NODE_CHILDREN> _children;
	std::vector<const Object*> _objects;

public:
    enum class Subvolume : size_t {
        LOWER_LEFT_FRONT = 0,
        LOWER_RIGHT_FRONT,
        UPPER_LEFT_FRONT,
        UPPER_RIGHT_FRONT,
        LOWER_LEFT_BACK,
        LOWER_RIGHT_BACK,
        UPPER_LEFT_BACK,
        UPPER_RIGHT_BACK
    };

	OctreeNode(const AABB& volume);
	void addObject(const Object* object, const AABB& volume);
    const OctreeNode* getChild(Subvolume subvolume) const;
    const AABB& getVolume() const;

    const std::vector<const Object*>& getObjects() const;

    friend class Octree;
};

class Octree {
    std::unique_ptr<OctreeNode> _root;

public:
    Octree(const AABB& volume);

    bool addObject(const Object* object, const AABB& volume);
    OctreeNode* getRoot();
};