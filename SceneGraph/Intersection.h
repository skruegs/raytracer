#ifndef INTERSECTION_H
#define INTERSECTION_H

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

struct Intersection {
    // the parameter `t` along the ray which was used 
	// (negative value indicates no intersection)
    float t;

    // surface normal at the point of intersection
    glm::vec3 normal;
};

#endif
