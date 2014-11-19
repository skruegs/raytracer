#ifndef INTERSECTION_H
#define INTERSECTION_H

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

struct Intersection {
    // The parameter `t` along the ray which was used. (A negative value indicates no intersection.)
    float t;
    // The surface normal at the point of intersection. (Ignored if t < 0.)
    glm::vec3 normal;
};

#endif
