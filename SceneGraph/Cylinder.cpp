#include "Cylinder.h"

static const float PI = 3.141592653589f;

// Creates a unit cylinder centered at (0, 0, 0)
Cylinder::Cylinder() :
    Geometry(CYLINDER),
    center_(glm::vec3(0.f, 0.f, 0.f)),
    radius_(0.5f),
    height_(1.0f)
{
    buildGeomtery();
}

Cylinder::~Cylinder() {}

void Cylinder::buildGeomtery()
{
    vertices_.clear();
    colors_.clear();
    normals_.clear();
    indices_.clear();

    unsigned short subdiv = 20;
    float dtheta = 2 * PI / subdiv;

    glm::vec4 point_top(0.0f, 0.5f * height_, radius_, 1.0f),
        point_bottom (0.0f, -0.5f * height_, radius_, 1.0f);
    vector<glm::vec3> cap_top, cap_bottom;

    // top and bottom cap vertices
    for (int i = 0; i < subdiv + 1; ++i) {
        glm::mat4 rotate = glm::rotate(glm::mat4(1.0f), i * dtheta, glm::vec3(0.f, 1.f, 0.f));
        glm::mat4 translate = glm::translate(glm::mat4(1.0f), center_);

        cap_top.push_back(glm::vec3(translate * rotate * point_top));
        cap_bottom.push_back(glm::vec3(translate * rotate * point_bottom));
    }

    //Create top cap.
    for ( int i = 0; i < subdiv - 2; i++) {
        vertices_.push_back(cap_top[0]);
        vertices_.push_back(cap_top[i + 1]);
        vertices_.push_back(cap_top[i + 2]);
    }
    //Create bottom cap.
    for (int i = 0; i < subdiv - 2; i++) {
        vertices_.push_back(cap_bottom[0]);
        vertices_.push_back(cap_bottom[i + 1]);
        vertices_.push_back(cap_bottom[i + 2]);
    }
    //Create barrel
    for (int i = 0; i < subdiv; i++) {
        //Right-side up triangle
        vertices_.push_back(cap_top[i]);
        vertices_.push_back(cap_bottom[i + 1]);
        vertices_.push_back(cap_bottom[i]);
        //Upside-down triangle
        vertices_.push_back(cap_top[i]);
        vertices_.push_back(cap_top[i + 1]);
        vertices_.push_back(cap_bottom[i + 1]);
    }

    // create normals
    glm::vec3 top_centerpoint(0.0f , 0.5f * height_ , 0.0f),
        bottom_centerpoint(0.0f, -0.5f * height_, 0.0f);
    glm::vec3 normal(0, 1, 0);

    // Create top cap.
    for (int i = 0; i < subdiv - 2; i++) {
        normals_.push_back(normal);
        normals_.push_back(normal);
        normals_.push_back(normal);
    }
    // Create bottom cap.
    for (int i = 0; i < subdiv - 2; i++) {
        normals_.push_back(-normal);
        normals_.push_back(-normal);
        normals_.push_back(-normal);
    }

    // Create barrel
    for (int i = 0; i < subdiv; i++) {
        //Right-side up triangle
        normals_.push_back(glm::normalize(cap_top[i] - top_centerpoint));
        normals_.push_back(glm::normalize(cap_bottom[i + 1] - bottom_centerpoint));
        normals_.push_back(glm::normalize(cap_bottom[i] - bottom_centerpoint));
        //Upside-down triangle
        normals_.push_back(glm::normalize(cap_top[i] - top_centerpoint));
        normals_.push_back(glm::normalize(cap_top[i + 1] - top_centerpoint));
        normals_.push_back(glm::normalize(cap_bottom[i + 1] - bottom_centerpoint));
    }

    // indices and colors
    for (unsigned int i = 0; i < vertices_.size(); ++i) {
        colors_.push_back(glm::vec3(1,1,1));
    }

    for (unsigned int i = 0; i < vertices_.size(); ++i) {
        indices_.push_back(i);
    }
}


Intersection Cylinder::intersectImpl(const Ray &ray) const {
    Intersection isx;
    isx.t = -1;

	vector<float> t_values;
	vector<glm::vec3> normal_values;

	Ray newRay;
	newRay.orig = glm::vec3(ray.orig[0], 0.0, ray.orig[2]);
	newRay.dir = glm::vec3(ray.dir[0], 0.0, ray.dir[2]);

	glm::vec3 Va = glm::vec3(0,1,0);
	glm::vec3 Vb = glm::vec3(0,-1,0);
	glm::vec3 dP = ray.orig - glm::vec3(0,-0.5,0);
	glm::vec3 dP2 = ray.orig - glm::vec3(0,0.5,0);

	float a = glm::dot(newRay.dir, newRay.dir);
	float b = 2 * glm::dot(newRay.orig - center_, newRay.dir);
	float c = glm::dot(newRay.orig - center_, newRay.orig - center_) - radius_*radius_;

	float d = b*b - 4*a*c;
	float t0 = (-b + sqrt(d)) / (2*a);
	float t1 = (-b - sqrt(d)) / (2*a);

	// add t0 and t1 to possible t values if they are valid
	if (d < 0) {
		return isx;
	}
	else {
		if (t0 >= 0 && glm::dot(Va, (dP + ray.dir * t0)) > 0 && glm::dot(Va, (dP2 +  ray.dir * t0)) < 0) {
			t_values.push_back(t0);
			glm::vec3 r = ray.orig + t0 * ray.dir;
			normal_values.push_back(glm::normalize(glm::vec3(r[0], 0.0, r[2]) - glm::vec3(center_[0], 0.0, center_[2])));
		}
		if (t1 >= 0 && glm::dot(Va, (dP + ray.dir * t1)) > 0 && glm::dot(Va, (dP2 + ray.dir * t1)) < 0) {
			t_values.push_back(t1);
			glm::vec3 r = ray.orig + t1 * ray.dir;
			normal_values.push_back(glm::normalize(glm::vec3(r[0], 0.0, r[2]) - glm::vec3(center_[0], 0.0, center_[2])));
		}
	}

	// Find cap intersections -- t2 and t3 values
	float a0 = -(glm::dot(dP, Va));
    float b0 = glm::dot(Va, ray.dir);
	//float a0 = -glm::dot(Va, dP);
    //float b0 = glm::dot(Va, ray.dir);
	float t3 = a0 / b0;
	if (t3 > 0 && glm::length(dP + ray.dir * t3) <= radius_) {
		t_values.push_back(t3);
		normal_values.push_back(glm::vec3(0,1,0));
	}
	
	float a2 = -(glm::dot(dP2, Vb));
    float b2 = glm::dot(Vb, ray.dir);
	//float a2 = -glm::dot(Vb, dP2);
    //float b2 = glm::dot(Vb, ray.dir);
	float t4 = a2 / b2;
	if (t4 > 0 && glm::length((dP2 + ray.dir * t4)) <= radius_) {
		t_values.push_back(t4);
		normal_values.push_back(glm::vec3(0,-1,0));
	}
	

	// Find smallest t
	bool flag = false;
	float minT =  std::numeric_limits<float>::infinity();
	glm::vec3 norm;

	for(int i = 0; i < t_values.size(); i++) {
		if(t_values[i] < minT) {
			minT = t_values[i];
			norm = normal_values[i];
			flag = true;
		}
	}

	if(flag) {
		isx.t = minT;
		isx.normal = norm;
	}

	return isx;
}

