#include "Scene.h"

#include <omp.h>

using namespace std;

static const float PI = 3.141592653589f;
static const float EPS = 0.05f;
static const float SHADOW_FEELER_COUNT = 15;


Scene::Scene() {
	intersec = new Intersection();
	intersec->t = std::numeric_limits<float>::infinity();
	intersec->normal = glm::vec3(0);
	intersected_node = NULL;
	lightBlocked = false;
	MONTE_CARLO = 1;

	picture.ReadFromFile("uffizi_probe.bmp");

}

bool Scene::readFile(const char* filename) {
	   
	std::ifstream ifs(filename, std::ifstream::in);
	if (!ifs || ifs.fail()) {
        std::cerr << "[!] Error reading configuration file [!]" << std::endl;
        throw (errno);
		return false;
	}

	while (ifs.good()) {
		std::string st;
		std::string trash;
		ifs >> st;
		
		if (st.compare("CAMERA") == 0) {
			ifs >> trash >> width >> height;
			ifs >> trash >> pos[0] >> pos[1] >> pos[2];
			ifs >> trash >> viewDir[0] >> viewDir[1] >> viewDir[2];
			ifs >> trash >> viewUp[0] >> viewUp[1] >> viewUp[2];
			ifs >> trash >> fovy;
		}
		else if (st.compare("LIGHT") == 0) {
			ifs >> trash >> lightPos[0] >> lightPos[1] >> lightPos[2];
			ifs >> trash >> lightColor[0] >> lightColor[1] >> lightColor[2];
		}
		else if (st.compare("MONC") == 0) {
			ifs >> MONTE_CARLO;
		}
		else if (st.compare("MAT") == 0) {
			
			Material* mat = new Material();
			ifs >> mat->name;
			ifs >> trash >> mat->diff_color[0] >> mat->diff_color[1] >> mat->diff_color[2];
			ifs >> trash >> mat->refl_color[0] >> mat->refl_color[1] >> mat->refl_color[2];
			ifs >> trash >> mat->expo;
			ifs >> trash >> mat->ior;
			ifs >> trash >> mat->mirr;
			ifs >> trash >> mat->tran;
			ifs >> trash >> mat->emit_light;

			if (mat->emit_light) {
				lightColor = mat->diff_color;  
				lightPos = glm::vec3(0, 9, 0); // default for openGL preview 
			}

			materials.push_back(mat);
		}
		else if (st.compare("NODE") == 0) {
			
			Node* n = new Node();
			ifs >> n->name;

			float tx, ty, tz, rx, ry, rz, sx, sy, sz;
			ifs >> trash >> tx >> ty >> tz >> trash >> rx >> ry >> rz >> trash >> sx >> sy >> sz;
			n->translate(tx, ty, tz);
			n->rotate(rx, ry, rz);
			n->scale(sx, sy, sz);

			glm::vec3 cent;
			ifs >> trash >> cent[0] >> cent[1] >> cent[2];

			std::string parent_name;
			ifs >> trash >> parent_name;
			for (unsigned int i = 0; i < nodes.size(); i++) {
				if (nodes.at(i)->name.compare(parent_name) == 0) {
					n->parent = nodes.at(i);
				}
			}

			std::string shape;
			ifs >> trash >> shape;

			if (shape.compare("cube") == 0) {
				Cube* cube = new Cube();
				n->geo = cube;
			}
			else if (shape.compare("sphere") == 0) {
				Sphere* sphere = new Sphere();
				n->geo = sphere;
			}
			else if (shape.compare("cylinder") == 0) {
				Cylinder* cyl = new Cylinder();
				n->geo = cyl;
			}
			else if (shape.compare("mesh") == 0) {
				std::string temp_filename;
				ifs >> trash >> temp_filename;
				temp_filename = "obj/" + temp_filename;

				Mesh* mesh = new Mesh(temp_filename);
				n->geo = mesh;
			}
			else {
				n->geo = NULL;
			}

			std::string next;
			ifs >> next;
			if (next.compare("RGBA") == 0) {
				ifs >> n->color[0] >> n->color[1] >> n->color[2];
			}
			else {
				std::string mat_name;
				ifs >> mat_name;
				for (int i = 0; i < materials.size(); i++) {
					if (materials.at(i)->name.compare(mat_name) == 0) {
						n->mat = materials.at(i); 
						break;
					}
				}
			}
			nodes.push_back(n);
		}
	}

	// set nodes' children
	for (unsigned int i = 0; i < nodes.size(); i++) {	
		for (unsigned int j = 0; j < nodes.size(); j++) {
			if ((nodes.at(j)->parent != NULL) && (nodes.at(j)->parent == nodes.at(i))) {
				nodes.at(i)->children.push_back(nodes.at(j));
			}
		}
	}

	return true;
}


/* raytrace using config file variables and the current state of the scene graph; outputs BMP image */
void Scene::traceImage(bool monte) {

	BMP output;
	output.SetSize((int)width, (int)height);
	output.SetBitDepth(24);

	glm::vec3 A = glm::cross(viewDir, viewUp);
	glm::vec3 B = glm::cross(A, viewDir);
	glm::vec3 M = pos + viewDir;	
	float magA = glm::length(A);
	float magB = glm::length(B);
	float magC = glm::length(viewDir);

	float fovh = atan((tan(glm::radians(fovy/2))) * (width / height));
    
	glm::vec3 H = (A * magC * tan(fovh)) / magA;
	glm::vec3 V = (B * magC * tan(glm::radians(fovy/2))) / magB;

	int w = width;
	int h = height;

	// -------- find root --------
	root = nodes.at(0);
	for (unsigned int i = 0; i < nodes.size(); i++) {
		if (nodes.at(i)->parent == NULL) {
			root = nodes.at(i);
			break;
		}
	}
	for (unsigned int i = 0; i < nodes.size(); i++) {
		if (nodes.at(i)->parent == NULL && nodes.at(i) != root) 
			nodes.at(i)->parent = root;
	} // -------------------------

//#pragma omp parallel for
	for (int y = 0; y < h; y++) {
		cout << "Raytracing: line " << y+1 << "/" << height << endl;
		for (int x = 0; x < w; x++) {

			if (monte) {
				// monte carlo
				float Sx = (float)(x) / (width - 1);	
				float Sy = (float)(y) / (height - 1);

				glm::vec3 Pw = M + (float)(2 * Sx - 1) * H - (float)(2 * Sy - 1) * V;
				Ray ray = Ray(pos, glm::normalize(Pw - pos));

				glm::vec3 pixel_color = glm::vec3(0);

				for (int j = 0; j < MONTE_CARLO; j++) {

						glm::vec3 color = traceRayMonteCarlo(root, ray, 0, glm::vec3(1.0));

						pixel_color[0] += color[0];
						pixel_color[1] += color[1];
						pixel_color[2] += color[2];
				}
				float r = glm::clamp(pixel_color[0] * 255 / (float)MONTE_CARLO, 0.0f, 255.0f);
				float g = glm::clamp(pixel_color[1] * 255 / (float)MONTE_CARLO, 0.0f, 255.0f);
				float b = glm::clamp(pixel_color[2] * 255 / (float)MONTE_CARLO, 0.0f, 255.0f);
				output(x,y)->Red   = r;
				output(x,y)->Green = g;
				output(x,y)->Blue  = b;
			}

			else {
				// antialising via supersampling (9 rays per pixel) 
				/*glm::vec3 pixel_color = glm::vec3(0);

				for (int dy = -1; dy <= 1; dy += 1) {
					for (int dx = -1; dx <= 1; dx += 1) {

						float Sx = (float)(x + (0.49*dx)) / (width - 1);	
						float Sy = (float)(y + (0.49*dx)) / (height - 1);

						glm::vec3 Pw = M + (float)(2 * Sx - 1) * H - (float)(2 * Sy - 1) * V;
						Ray ray = Ray(pos, glm::normalize(Pw - pos));

						glm::vec3 color = traceRay(root, ray, 0);

						pixel_color[0] += glm::clamp(color[0], 0.0f, 1.0f);
						pixel_color[1] += glm::clamp(color[1], 0.0f, 1.0f);
						pixel_color[2] += glm::clamp(color[2], 0.0f, 1.0f);

					}
				}
				output(x,y)->Red   = pixel_color[0] * 255 / 9;
				output(x,y)->Green = pixel_color[1] * 255 / 9;
				output(x,y)->Blue  = pixel_color[2] * 255 / 9;
				*/
				float Sx = (float)(x) / (width - 1);	
				float Sy = (float)(y) / (height - 1);

				glm::vec3 Pw = M + (float)(2 * Sx - 1) * H - (float)(2 * Sy - 1) * V;
				Ray ray = Ray(pos, glm::normalize(Pw - pos));

				glm::vec3 color = traceRay(root, ray, 0);

				color[0] = glm::clamp(color[0], 0.0f, 1.0f);
				color[1] = glm::clamp(color[1], 0.0f, 1.0f);
				color[2] = glm::clamp(color[2], 0.0f, 1.0f);

				output(x,y)->Red   = color[0] * 255;
				output(x,y)->Green = color[1] * 255;
				output(x,y)->Blue  = color[2] * 255;
			}

		}
	}
	cout << "Finished raycasting." << endl;
	output.WriteToFile("output.BMP");
}



glm::vec3 Scene::traceRayMonteCarlo(Node* root, Ray ray, int depth, glm::vec3 transmittance) {
	
	// reset global variables
	intersec->t = std::numeric_limits<float>::infinity();
	intersec->normal = glm::vec3(0);
	intersected_node = NULL;
	lightBlocked = false;

	// find intersection and intersected node
	intersect(root, glm::mat4(1.0), ray);

	// point of intersection
	glm::vec3 point = ray.orig + intersec->t * ray.dir;
			
	// sphere mapping, or return black 
	if (intersected_node == NULL) {
		
		/*int picWidth = picture.TellWidth();
        int picHeight = picture.TellHeight(); 
		glm::vec3 rd = glm::normalize(ray.dir);

		return glm::vec3(picture((rd[0]+1)*picWidth/2, (rd[1]+1)*picHeight/2)->Red   / 255, 
						 picture((rd[0]+1)*picWidth/2, (rd[1]+1)*picHeight/2)->Green / 255, 
						 picture((rd[0]+1)*picWidth/2, (rd[1]+1)*picHeight/2)->Blue  / 255);*/
		
		return glm::vec3(0.0f);
	}

	// limit recursion depth
	if (depth > 5) {
		return glm::vec3(0);
	}

	if (!intersected_node->mat->mirr && !intersected_node->mat->tran && intersected_node->mat->emit_light == 0) {
				
		float r = ((float) rand() / (RAND_MAX));
		float maxRG = max(intersected_node->mat->diff_color[0], intersected_node->mat->diff_color[1]);
		float absorbance = 1 - max(maxRG, intersected_node->mat->diff_color[2]);

		if (r < absorbance) 
			return glm::vec3(0);

		glm::vec3 cw = glm::normalize(getCosineWeightedDirection(glm::normalize(intersec->normal)));
		Ray cw_ray = Ray(point + cw*EPS, cw);

		transmittance = transmittance * intersected_node->mat->diff_color / (1-absorbance);

		return traceRayMonteCarlo(root, cw_ray, depth + 1, transmittance);
	}

	// reflection
	else if (intersected_node->mat->mirr) {

		glm::vec3 refl = glm::normalize(glm::reflect(ray.dir, intersec->normal));
		Ray reflected_ray = Ray(point + refl*EPS, refl);

		return traceRayMonteCarlo(root, reflected_ray, depth + 1, transmittance * intersected_node->mat->refl_color);
	}

	// refraction
	else if (intersected_node->mat->tran) {
		float n_i;
		float n_t;
		if (intersected_node->geo->isAway()) {
			n_i = intersected_node->mat->ior;
			n_t = 1.0;
		}
		else {
			n_t = intersected_node->mat->ior;
			n_i = 1.0;
		}
		glm::vec3 refr = glm::normalize(glm::refract(ray.dir, intersec->normal, n_i/n_t));
		Ray refracted_ray = Ray(point + refr*EPS, refr);

		return traceRayMonteCarlo(root, refracted_ray, depth + 1, transmittance * glm::vec3(1.0f));
	}

	// hits a light
	else if (intersected_node->mat->emit_light != 0) {
		return transmittance * lightColor * (float)(intersected_node->mat->emit_light); // emittance **READ IN CONFIG FILE**
	}
	
}



glm::vec3 Scene::traceRay(Node* root, Ray ray, int depth) {
	
	// reset global variables
	intersec->t = std::numeric_limits<float>::infinity();
	intersec->normal = glm::vec3(0);
	intersected_node = NULL;
	lightBlocked = false;

	// find intersection and intersected node
	intersect(root, glm::mat4(1.0), ray);

	// calculate pixel color
	glm::vec3 color = glm::vec3(0);

	if (intersected_node != NULL) {
		// point of intersection
		glm::vec3 point = ray.orig + intersec->t * ray.dir;
			
		// soft shadows from area light
		Node* light_node = nodes.at(0);
		for (unsigned int i = 0; i < nodes.size(); i++) {
			if (nodes.at(i)->mat != NULL && nodes.at(i)->mat->emit_light) {
				light_node = nodes.at(i);
				break;
			}
		}						
		for (int i = 0; i < SHADOW_FEELER_COUNT; i++) {
			lightBlocked = false;
			glm::vec3 random_light_pt;
			switch (light_node->geo->getGeometryType()) {
				case Geometry::SPHERE:
					random_light_pt = getRandomPointOnSphere(light_node);
					break;
				case Geometry::CUBE:
					random_light_pt = getRandomPointOnCube(light_node);
					break;
			}
			glm::vec3 lightDir = glm::normalize(random_light_pt - point);
			pointToLight(root, glm::mat4(1.0f), Ray(point + lightDir*EPS, lightDir), random_light_pt);
			if (lightBlocked) {
				color += glm::vec3(0.1,0.1,0.1) * intersected_node->mat->diff_color;
			}
			else {
				if (intersected_node->mat->emit_light) color += intersected_node->mat->diff_color;	// light source
				else color += intersected_node->mat->calculateColor(pos, intersec->normal, point, random_light_pt, lightColor);
			}		
		}
		color /= SHADOW_FEELER_COUNT;

		// limit recursion to a depth of 5
		if (depth < 5) {

			// reflection
			if (intersected_node->mat->mirr) {
				glm::vec3 temp_color = intersected_node->mat->refl_color;

				glm::vec3 refl = glm::normalize(glm::reflect(ray.dir, intersec->normal));
				Ray reflected_ray = Ray(point + refl*EPS, refl);

				color += temp_color * traceRay(root, reflected_ray, depth + 1);
			}

			// refraction
			else if (intersected_node->mat->tran) {
				float n_i;
				float n_t;
				if (intersected_node->geo->isAway()) {
					n_i = intersected_node->mat->ior;
					n_t = 1.0;
				}
				else {
					n_t = intersected_node->mat->ior;
					n_i = 1.0;
				}
				glm::vec3 refr = glm::normalize(glm::refract(ray.dir, intersec->normal, n_i/n_t));
				Ray refracted_ray = Ray(point + refr*EPS, refr);

				color +=  traceRay(root, refracted_ray, depth + 1);
			}
		}
	}

	return color;
}



/* sets global intersec and intersected_node varibales */
void Scene::intersect(Node* n, glm::mat4 t, Ray ray) {
	t = t * n->transformation_matrix;

	if (n->geo != NULL) {
		Intersection inter = n->geo->intersect(t, ray);
		if (inter.t < intersec->t && inter.t != -1) {
			intersec->t = inter.t;
			intersec->normal = inter.normal;
			intersected_node = n;
		}
	}
    for (unsigned int i = 0; i < n->children.size(); i++) {
        intersect(n->children.at(i), t, ray);
    }
}

/* sets lightBlocked to true if there is an object occluding the light source */
void Scene::pointToLight(Node* n, glm::mat4 t, Ray ray, glm::vec3 light_pt) {
	t = t * n->transformation_matrix;

	Node* light_node = nodes.at(0);
	for (unsigned int i = 0; i < nodes.size(); i++) {
		if (nodes.at(i)->mat != NULL && nodes.at(i)->mat->emit_light) {
			light_node = nodes.at(i);
			break;
		}
	}
	if (n->geo != NULL && n != intersected_node && n != light_node) {
		Intersection inter = n->geo->intersect(t, ray);

		if (inter.t != -1) { 
			glm::vec3 pt = ray.orig + inter.t * ray.dir;

			float distance = glm::length(pt - ray.orig);
			float distance_to_light = glm::length(light_pt - ray.orig);

			if (distance <= distance_to_light) {
				lightBlocked = true;
			}
		}
	}
	for (unsigned int i = 0; i < n->children.size(); i++) {
		pointToLight(n->children.at(i), t, ray, light_pt);
    }
}



/* returns a random point on a sphere */
glm::vec3 Scene::getRandomPointOnSphere(Node* n) {
	// generate u, v, in the range (0, 1)
	float u = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
	float v = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);

	float theta = 2.0f * PI * u;
	float phi = acos(2.0f * v - 1.0f);

	// find x, y, z coordinates assuming unit sphere in object space
	glm::vec3 point;
	point[0] = sin(phi) * cos(theta);
	point[1] = sin(phi) * sin(theta);
	point[2] = cos(phi);

	// transform point to world space
	point = glm::vec3((n->transformation_matrix) *  glm::vec4(point, 1));
	return point;
}

/* returns a random point on a cube */
glm::vec3 Scene::getRandomPointOnCube(Node* n) {
	// get the dimensions of the transformed cube in world space
	glm::vec3 dim = glm::vec3(n->transformation_matrix[0][0], n->transformation_matrix[1][1], n->transformation_matrix[2][2]);

	// Get surface area of the cube
	float side1 = dim[0] * dim[1];		// x-y
	float side2 = dim[1] * dim[2];		// y-z
	float side3 = dim[0] * dim[2];		// x-z
	float totalArea = 2.0f * (side1 + side2 + side3);	

	// pick random face weighted by surface area
	float r = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
	// pick 2 random components for the point in the range (-0.5, 0.5)
	float c1 = static_cast <float> (rand()) / static_cast <float> (RAND_MAX) - 0.5f;
	float c2 = static_cast <float> (rand()) / static_cast <float> (RAND_MAX) - 0.5f;

	glm::vec3 point;
	if (r < side1 / totalArea) 									// x-y front		
		point = glm::vec3(c1, c2, 0.5f);
	else if (r < (side1 * 2) / totalArea)						// x-y back
		point = glm::vec3(c1, c2, -0.5f);
	else if (r < (side1 * 2 + side2) / totalArea)				// y-z front
		point = glm::vec3(0.5f, c1, c2);
	else if (r < (side1 * 2 + side2 * 2) / totalArea) 			// y-z back
		point = glm::vec3(-0.5f, c1, c2);
	else if (r < (side1 * 2 + side2 * 2 + side3) / totalArea)	// x-z front 
		point = glm::vec3(c1, 0.5f, c2);
	else														// x-z back
		point = glm::vec3(c1, -0.5f, c2);

	// transform point to world space
	point = glm::vec3((n->transformation_matrix) *  glm::vec4(point, 1));
	return point;
}


/* given a normal vector, finds a cosine weighted random direction in a hemisphere */
glm::vec3 Scene::getCosineWeightedDirection(const glm::vec3& normal) {

	// Pick 2 random numbers in the range (0, 1)
	float xi1 = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
	float xi2 = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);

    float up = sqrt(xi1); 			// cos(theta)
    float over = sqrt(1 - up * up); // sin(theta)
    float around = xi2 * 2.0f * PI;
    
    // Find a direction that is not the normal based off of whether or not the normal's components 
    // are all equal to sqrt(1/3) or whether or not at least one component is less than sqrt(1/3).
    const float SQRT_OF_ONE_THIRD = sqrt(1.0f/3.0f);
    glm::vec3 directionNotNormal;
    if (abs(normal.x) < SQRT_OF_ONE_THIRD) {
      directionNotNormal = glm::vec3(1.f, 0.f, 0.f);
    } else if (abs(normal.y) < SQRT_OF_ONE_THIRD) {
      directionNotNormal = glm::vec3(0.f, 1.f, 0.f);
    } else {
      directionNotNormal = glm::vec3(0.f, 0.f, 1.f);
    }
    
    //Use not-normal direction to generate two perpendicular directions
    glm::vec3 perpendicularDirection1 = glm::normalize(glm::cross(normal, directionNotNormal));
    glm::vec3 perpendicularDirection2 = glm::normalize(glm::cross(normal, perpendicularDirection1));
    
    return (up * normal) + (cos(around) * over * perpendicularDirection1) + (sin(around) * over * perpendicularDirection2);
}