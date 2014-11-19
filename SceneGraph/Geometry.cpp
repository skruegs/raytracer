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
    // The input ray here is in WORLD-space. It may not be normalized!

    // TODO: normalize ray_world.
	//ray_world.orig = glm::normalize(ray_world.orig);
	ray_world.dir = glm::normalize(ray_world.dir);

    // Transform the ray into OBJECT-LOCAL-space, for intersection calculation.
    // TODO: COMPUTE THIS AS FOLLOWS:
		// Transform the ray by the inverse transformation to get ray_local.
		// (Remember that position = vec4(vec3, 1) while direction = vec4(vec3, 0).)
	Ray ray_local;
	ray_local.orig = glm::vec3(glm::inverse(T) * glm::vec4(ray_world.orig, 1));
	ray_local.dir = glm::vec3(glm::inverse(T) * glm::vec4(ray_world.dir, 0));

    // Compute the intersection in LOCAL-space.
    Intersection isx = intersectImpl(ray_local);

    if (isx.t != -1) {
        // Transform the local-space intersection BACK into world-space.
        //     (Note that, as long as you didn't re-normalize the ray direction
        //     earlier, `t` doesn't need to change.)
        const glm::vec3 normal_local = isx.normal;
        // TODO: COMPUTE THIS AS FOLLOWS:
			// Inverse-transpose-transform the normal to get it back from
			// local-space to world-space. (If you were transforming a position,
			// you would just use the unmodified transform T.)
			// http://www.arcsynthesis.org/gltut/Illumination/Tut09%20Normal%20Transformation.html
        glm::vec3 normal_world = glm::normalize(glm::vec3(glm::transpose(glm::inverse(T)) *  glm::vec4(normal_local, 0)));
		
        // TODO: You might want to do this here: make sure here that your
        // normal is pointing the right way (toward, not away from, the ray
        // origin). Instead of doing this inside intersectImpl, you can do so
        // here by just negating normal_world depending on the glm::sign of
        //     glm::dot(normal_world, ray_world.dir).


		if (glm::dot(normal_world, ray_world.dir) > 0) {
			normal_world = -normal_world;
		}

		isx.normal = normal_world;

    }

    // The final output intersection data is in WORLD-space.
    return isx;
}
