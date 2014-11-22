#include "Sphere.h"

static const float PI = 3.141592653589f;

// Creates a unit sphere centered at (0, 0, 0)
Sphere::Sphere() :
    Geometry(SPHERE),
    center_(glm::vec3(0.f, 0.f, 0.f)),
    radius_(1.0f)
{
    buildGeomtery();
}

Sphere::~Sphere() {}

void Sphere::buildGeomtery()
{
    vertices_.clear();
    colors_.clear();
    normals_.clear();
    indices_.clear();

    // Find vertex positions for the sphere.
    unsigned int subdiv_axis = 16;      // vertical slices
    unsigned int subdiv_height = 16;        // horizontal slices
    float dphi = PI / subdiv_height;
    float dtheta = 2.0f * PI / subdiv_axis;
    float epsilon = 0.0001f;
    glm::vec3 color (0.6f, 0.6f, 0.6f);

    // North pole
    glm::vec3 point (0.0f, 1.0f, 0.0f);
    normals_.push_back(point);
    // scale by radius_ and translate by center_
    vertices_.push_back(center_ + radius_ * point);

    for (float phi = dphi; phi < PI; phi += dphi) {
        for (float theta = dtheta; theta <= 2.0f * PI + epsilon; theta += dtheta) {
            float sin_phi = sin(phi);

            point[0] = sin_phi * sin(theta);
            point[1] = cos(phi);
            point[2] = sin_phi * cos(theta);

            normals_.push_back(point);
            vertices_.push_back(center_ + radius_ * point);
        }
    }
    // South pole
    point = glm::vec3(0.0f, -1.0f, 0.0f);
    normals_.push_back(point);
    vertices_.push_back(center_ + radius_ * point);

    // fill in index array.
    // top cap
    for (unsigned int i = 0; i < subdiv_axis - 1; ++i) {
        indices_.push_back(0);
        indices_.push_back(i + 1);
        indices_.push_back(i + 2);
    }
    indices_.push_back(0);
    indices_.push_back(subdiv_axis);
    indices_.push_back(1);

    // middle subdivs
    unsigned int index = 1;
    for (unsigned int i = 0; i < subdiv_height - 2; i++) {
        for (unsigned int j = 0; j < subdiv_axis - 1; j++) {
            // first triangle
            indices_.push_back(index);
            indices_.push_back(index + subdiv_axis);
            indices_.push_back(index + subdiv_axis + 1);

            // second triangle
            indices_.push_back(index);
            indices_.push_back(index + subdiv_axis + 1);
            indices_.push_back(index + 1);

            index++;
        }
        // reuse vertices from start and end point of subdiv_axis slice
        indices_.push_back(index);
        indices_.push_back(index + subdiv_axis);
        indices_.push_back(index + 1);

        indices_.push_back(index);
        indices_.push_back(index + 1);
        indices_.push_back(index + 1 - subdiv_axis);

        index++;
    }

    // end cap
    unsigned int bottom = (subdiv_height - 1) * subdiv_axis + 1;
    unsigned int offset = bottom - subdiv_axis;
    for (unsigned int i = 0; i < subdiv_axis - 1 ; ++i) {
        indices_.push_back(bottom);
        indices_.push_back(i + offset);
        indices_.push_back(i + offset + 1);
    }
    indices_.push_back(bottom);
    indices_.push_back(bottom - 1);
    indices_.push_back(offset);

    // colors
    for (unsigned int i = 0; i < vertices_.size(); ++i) {
        colors_.push_back(glm::vec3(1,1,1));
    }
}


Intersection Sphere::intersectImpl(const Ray &ray) const {

	Intersection isx;
	isx.t = -1;

	float a = glm::dot(ray.dir, ray.dir);
	float b = glm::dot(ray.dir,  (2.0f * (ray.orig - center_)));
	float c = glm::dot(center_, center_) +  glm::dot(ray.orig, ray.orig) - 2.0f * glm::dot(ray.orig, center_) - radius_*radius_;
	float D = b*b + (-4.0f)*a*c;

	if (D < 0) {
        return isx;
	}

	float t0 = (-0.5f)*(b + sqrt(D))/a;
	float t1 = (-0.5f)*(b - sqrt(D))/a;
	float t = min(t0, t1);
	
	if (t > 0.0f) {
        glm::vec3 hitpoint = ray.orig + t*ray.dir;
        glm::vec3 normal = (hitpoint - center_) / radius_;
		isx.t = t;
		isx.normal = normal;
    }

	//cout << t <<" "<< isx.normal[0] << " " << isx.normal[1] << " " << isx.normal[2] << endl;
    return isx;
}

