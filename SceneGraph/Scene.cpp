#include "Scene.h"

using namespace std;
static const float PI = 3.141592653589f;
#define GLM_FORCE_RADIANS

Scene::Scene() {
	intersec = new Intersection();
	intersec->t = std::numeric_limits<float>::infinity();
	intersec->normal = glm::vec3(0);
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
			//n->translate(cent[0], cent[1], cent[2]);

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

			ifs >> trash >> n->color[0] >> n->color[1] >> n->color[2];
		
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



void Scene::Raycast() {

	BMP output;
	output.SetSize((int)width, (int)height);
	output.SetBitDepth(24);

	glm::vec3 A = glm::cross(viewDir, viewUp);
	glm::vec3 B = glm::cross(A, viewDir);
	glm::vec3 M = pos + viewDir;	
	float magA = glm::length(A);
	float magB = glm::length(B);
	float magC = glm::length(viewDir);
	

	fovy /= 2;
	float fovh = atan((tan(fovy)) * (width / height));

	glm::vec3 H = (A * magC * tan(fovh)) / magA;
	glm::vec3 V = (B * magC * tan(fovy)) / magB;

	float Sx, Sy;
	glm::vec3 Pw;
	for (int y = 0; y < height; y++) {
		cout << "Raycasting: line " << y+1 << "/480" << endl;

		for (int x = 0; x < width; x++) {
	
			Sx = (float)x/(width - 1);	
			Sy = (float)y/(height - 1);

			Pw = M + (float)(2 * Sx - 1) * H - (float)(2 * Sy - 1) * V;	
			Ray ray = Ray(pos, glm::normalize(Pw - pos));

			// --- find root ---
			root = nodes.at(0);
			for (unsigned int i = 0; i < nodes.size(); i++) {
				if (nodes.at(i)->parent == NULL) {
					root = nodes.at(i);
					break;
				}
			}
			//if there are other nodes with no parent, set their parent to root
			for (unsigned int i = 0; i < nodes.size(); i++) {
				if (nodes.at(i)->parent == NULL && nodes.at(i) != root) 
					nodes.at(i)->parent = root;
			}
			// -----------------
			

			traverse(root, glm::mat4(1.0f), ray);

			float n0 = 0.0f, n1 = 0.0f, n2 = 0.0f;
			if (intersec->t != -1) {
				if (intersec->normal[0] < 0) intersec->normal[0] = 0;
				if (intersec->normal[1] < 0) intersec->normal[1] = 0;
				if (intersec->normal[2] < 0) intersec->normal[2] = 0;
				n0 = intersec->normal[0];
				n1 = intersec->normal[1];
				n2 = intersec->normal[2];
			}


			output(x, y)->Red =   n0 * 255;
			output(x, y)->Green = n1 * 255;
			output(x, y)->Blue =  n2 * 255;
			//output(width - x - 1, height - y - 1)->Red =   n0 * 255;
			//output(width - x - 1, height - y - 1)->Green = n1 * 255;
			//output(width - x - 1, height - y - 1)->Blue =  n2 * 255;
			
			intersec->t = std::numeric_limits<float>::infinity();
			intersec->normal = glm::vec3(0);
		}
	}
	cout << "Finished raycasting." << endl;
	output.WriteToFile("output.BMP");
}



void Scene::traverse(Node* n, glm::mat4 t, Ray ray) {
	t = t * n->transformation_matrix;

	if (n->geo != NULL) {
		Intersection inter = n->geo->intersect(t, ray);
		if (inter.t < intersec->t && inter.t != -1) {
			intersec->t = inter.t;
			intersec->normal = inter.normal;
		}
	}

    for (unsigned int i = 0; i < n->children.size(); i++) {
        traverse(n->children.at(i), t, ray);
    }
}
