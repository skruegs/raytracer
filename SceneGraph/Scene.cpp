#include "Scene.h"

using namespace std;
static const float PI = 3.141592653589f;
static const float epsilon = 0.05f;

Scene::Scene() {
	intersec = new Intersection();
	intersec->t = std::numeric_limits<float>::infinity();
	intersec->normal = glm::vec3(0);
	intersected_node = NULL;
	lightBlocked = false;
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
		else if (st.compare("MAT") == 0) {
			
			Material* mat = new Material();
			ifs >> mat->name;
			ifs >> trash >> mat->diff_color[0] >> mat->diff_color[1] >> mat->diff_color[2];
			ifs >> trash >> mat->refl_color[0] >> mat->refl_color[1] >> mat->refl_color[2];
			ifs >> trash >> mat->expo;
			ifs >> trash >> mat->ior;
			ifs >> trash >> mat->mirr;
			ifs >> trash >> mat->tran;

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


/* raytrace using config file variables and the current state
 * of the scene graph; output BMP image */
void Scene::traceImage() {

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

	for (int y = 0; y < height; y++) {
		cout << "Raytracing: line " << y+1 << "/" << height << endl;
		for (int x = 0; x < width; x++) {

			float Sx = (float)x/(width - 1);	
			float Sy = (float)y/(height - 1);

			glm::vec3 Pw = M + (float)(2 * Sx - 1) * H - (float)(2 * Sy - 1) * V;
			Ray ray = Ray(pos, glm::normalize(Pw - pos));

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
			}
			// ---------------------------

			glm::vec3 color = traceRay(root, ray, 0);

			color[0] = glm::clamp(color[0], 0.0f, 1.0f);
			color[1] = glm::clamp(color[1], 0.0f, 1.0f);
			color[2] = glm::clamp(color[2], 0.0f, 1.0f);

			output(x,y)->Red	= color[0] * 255;
			output(x,y)->Green	= color[1] * 255;
			output(x,y)->Blue	= color[2] * 255;
		}
	}
	cout << "Finished raycasting." << endl;
	output.WriteToFile("output.BMP");
}



glm::vec3 Scene::traceRay(Node* root, Ray ray, int depth) {
	
	// reset global variables
	intersec->t = std::numeric_limits<float>::infinity();
	intersec->normal = glm::vec3(0);
	intersected_node = NULL;
	lightBlocked = false;

	// find intersection and intersected node
	intersect(root, glm::mat4(1.0), ray);

	// define color to be returned
	glm::vec3 color = glm::vec3(0);
	
	// calculate pixel color
	if (intersected_node != NULL) {

		// point of intersection
		glm::vec3 point = ray.orig + intersec->t * ray.dir;

		// shadows
		glm::vec3 lightDir = glm::normalize(lightPos - point);
		pointToLightIntersect(root, glm::mat4(1.0f), Ray(point + lightDir*epsilon, lightDir));
		color = glm::vec3(0.1,0.1,0.1) * intersected_node->mat->diff_color;
		if (!lightBlocked) 
			color = intersected_node->mat->calculateColor(pos, intersec->normal, point, lightPos, lightColor);

		// limit recursion to a depth of 5
		if (depth < 5) {

			// reflection
			if (intersected_node->mat->mirr) {
				glm::vec3 temp_color = intersected_node->mat->refl_color;

				glm::vec3 refl = glm::normalize(glm::reflect(ray.dir, intersec->normal));
				Ray reflected_ray = Ray(point + refl*epsilon, refl);

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
				Ray refracted_ray = Ray(point + refr*epsilon, refr);

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

/* sets global variable lightBlocked to true if there is 
 * an object occluding the light source */
void Scene::pointToLightIntersect(Node* n, glm::mat4 t, Ray ray) {
	t = t * n->transformation_matrix;
	
	if (n->geo != NULL && n != intersected_node) {
		Intersection inter = n->geo->intersect(t, ray);

		if (inter.t != -1) { 
			glm::vec3 pt = ray.orig + inter.t * ray.dir;

			float distance = glm::length(pt - ray.orig);
			float distance_to_light = glm::length(lightPos - ray.orig);

			if (distance <= distance_to_light) {
				lightBlocked = true;
				return;
			}	
		}
	}
	for (unsigned int i = 0; i < n->children.size(); i++) {
        pointToLightIntersect(n->children.at(i), t, ray);
    }
}