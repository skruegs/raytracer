#include "Geometry.h"

Geometry::Geometry(geometryType geomType) :
    type_(geomType)
{
}

Geometry::~Geometry()
{
    vertices_.clear();
    normals_.clear();
    colors_.clear();
    indices_.clear();
}


Intersection Geometry::intersect(const glm::mat4 &T, Ray ray_world) const
{

	// TODO: normalize ray_world.
	ray_world.dir = glm::normalize(ray_world.dir);

        // Transform the ray into OBJECT-LOCAL-space, for intersection calculation.
	Ray ray_local;
	ray_local.orig = glm::vec3(glm::inverse(T) * glm::vec4(ray_world.orig, 1));
	ray_local.dir = glm::vec3(glm::inverse(T) * glm::vec4(ray_world.dir, 0));

    	// Compute the intersection in LOCAL-space.
    	Intersection isx = intersectImpl(ray_local);

    	if (isx.t != -1) {
        	// Transform the local-space intersection BACK into world-space.
        	const glm::vec3 normal_local = isx.normal;
		// Inverse-transpose-transform the normal to get it back from local-space to world-space. 
        	glm::vec3 normal_world = glm::normalize(glm::vec3(glm::transpose(glm::inverse(T)) *  glm::vec4(normal_local, 0)));
	
        	// Make sure that normal is pointing the right way (toward the ray origin)
		if (glm::dot(normal_world, ray_world.dir) > 0) {
			normal_world = -normal_world;
		}

		isx.normal = normal_world;

    }

    // output intersection data in WORLD-space
    return isx;
}
