#include "Cube.h"

static const float PI = 3.141592653589f;

// Creates a unit cube centered at (0, 0, 0)
Cube::Cube() :
    Geometry(CUBE),
    center_(glm::vec3(0.f, 0.f, 0.f)),
    length_(1.0f)
{
    buildGeomtery();
}

Cube::~Cube() {}

void Cube::buildGeomtery()
{
    vertices_.clear();
    colors_.clear();
    normals_.clear();
    indices_.clear();

	// vertices
	glm::vec3 v1 = glm::vec3(-0.5, -0.5, 0.5);
	glm::vec3 v2 = glm::vec3(0.5, -0.5, 0.5);
	glm::vec3 v3 = glm::vec3(0.5, 0.5, 0.5);
	glm::vec3 v4 = glm::vec3(-0.5, 0.5, 0.5);
	glm::vec3 v5 = glm::vec3(-0.5, -0.5, -0.5);
	glm::vec3 v6 = glm::vec3(0.5, -0.5, -0.5);
	glm::vec3 v7 = glm::vec3(0.5, 0.5, -0.5);
	glm::vec3 v8 = glm::vec3(-0.5, 0.5, -0.5);
	vertices_.push_back(v1);
	vertices_.push_back(v2);
	vertices_.push_back(v3);
	vertices_.push_back(v4);
	vertices_.push_back(v5);
	vertices_.push_back(v6);
	vertices_.push_back(v7);
	vertices_.push_back(v8);

	// normals
	normals_.push_back(v1);
	normals_.push_back(v2);
	normals_.push_back(v3);
	normals_.push_back(v4);
	normals_.push_back(v5);
	normals_.push_back(v6);
	normals_.push_back(v7);
	normals_.push_back(v8);


	// indices
	indices_.push_back(0);
	indices_.push_back(1);
	indices_.push_back(2);	
	indices_.push_back(2);
	indices_.push_back(3);
	indices_.push_back(0);
	indices_.push_back(3);
	indices_.push_back(2);
	indices_.push_back(6);
	indices_.push_back(6);
	indices_.push_back(7);
	indices_.push_back(3);
	indices_.push_back(7);
	indices_.push_back(6);
	indices_.push_back(5);
	indices_.push_back(5);
	indices_.push_back(4);
	indices_.push_back(7);
	indices_.push_back(4);
	indices_.push_back(0);
	indices_.push_back(3);	
	indices_.push_back(3);
	indices_.push_back(7);
	indices_.push_back(4);
	indices_.push_back(0);
	indices_.push_back(1);
	indices_.push_back(5);
	indices_.push_back(5);
	indices_.push_back(4);
	indices_.push_back(0);
	indices_.push_back(1);
	indices_.push_back(5);
	indices_.push_back(6);
	indices_.push_back(6);
	indices_.push_back(2);
	indices_.push_back(1);

	// colors
    for (unsigned int i = 0; i < vertices_.size(); ++i) {
        colors_.push_back(glm::vec3(1,1,1));
    }
}


Intersection Cube::intersectImpl(const Ray &ray) const {

	Intersection isx;
	isx.t = -1;

	float Tnear = -1.0 * std::numeric_limits<float>::infinity();
	float Tfar = std::numeric_limits<float>::infinity();

	for (int i = 0; i < 3; i++) {

		glm::vec3 Bl = glm::vec3(center_[i] - length_/2.0f);
		glm::vec3 Bh = glm::vec3(center_[i] + length_/2.0f);

		if (ray.dir[i] == 0 && (ray.orig[i] < Bl[i] || ray.orig[i] > Bh[i])) {
			return isx;
		}
		else {
			float T1 = (Bl[i] - ray.orig[i]) / ray.dir[i];
			float T2 = (Bh[i] - ray.orig[i]) / ray.dir[i];
			if (T1 > T2) {
				T1 -= T2;  
				T2 += T1;  
				T1 = (T2 - T1);  
			}
			if (T1 > Tnear) {
				Tnear = T1;
			}
			if (T2 < Tfar) {
				Tfar = T2;
			}
			if (Tnear > Tfar) {
				return isx;
			}
			if (Tfar < 0) {
				return isx;
			}
		}
	}

	isx.t = Tnear;
	glm::vec3 hitpoint = ray.orig + isx.t*ray.dir;
    
	if (hitpoint[0] < 0.5 + 1e-3 && hitpoint[0] > 0.5 - 1e-3)
		isx.normal = glm::vec3(1,0,0);
	else if (hitpoint[0] < -0.5 + 1e-3 && hitpoint[0] > -0.5 - 1e-3)
		isx.normal = glm::vec3(-1,0,0);
	else if (hitpoint[1] < 0.5 + 1e-3 && hitpoint[1] > 0.5 - 1e-3)
		isx.normal = glm::vec3(0,1,0);
	else if (hitpoint[1] < -0.5 + 1e-3 && hitpoint[1] > -0.5 - 1e-3)
		isx.normal = glm::vec3(0,-1,0);
	else if (hitpoint[2] < 0.5 + 1e-3 && hitpoint[2] > 0.5 - 1e-3)
		isx.normal = glm::vec3(0,0,1);
	else if (hitpoint[2] < -0.5 + 1e-3 && hitpoint[2] > -0.5 - 1e-3)
		isx.normal = glm::vec3(0,0,-1);

	//cout << isx.t << " " << isx.normal[0] << " " << isx.normal[1] << " " << isx.normal[2] << endl;
	return isx;

}

