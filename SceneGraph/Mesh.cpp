#include "Mesh.h"

using namespace std;

Mesh::Mesh() : Geometry(MESH) {
	filename = "";
    buildGeomtery();
}

Mesh::Mesh(std::string fn) : Geometry(MESH) {
    filename = fn;
	buildGeomtery();
}

Mesh::~Mesh() {}


void Mesh::buildGeomtery()
{
    vertices_.clear();
    colors_.clear();
    normals_.clear();
    indices_.clear();
	 
	/*  
	**  Build geometry by reading
	**  in data from .OBJ file.
	*/
	if (filename.empty()) return;
	cout << "Loading: " << filename << endl;
	const char * c = filename.c_str();

	FILE * file = fopen(c, "r");
	if( file == NULL ){
		std::cerr << "[!] Error reading .OBJ file \"" << filename << "\" [!]" << std::endl;
	}

	std::vector<glm::vec3> temp_normals;

	while (1) {
		
		char lineHeader[128];
		// read the first word of the line
		int res = fscanf(file, "%s", lineHeader);
		if (res == EOF) break; // EOF = End Of File. Quit the loop.

		// else : parse lineHeader
		if (strcmp(lineHeader, "v") == 0){
			glm::vec3 vertex;
			fscanf(file, "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z );
			vertices_.push_back(vertex);
		}
		else if (strcmp(lineHeader, "vt") == 0 ){
			// texture data
		}
		else if (strcmp(lineHeader, "vn") == 0){
			glm::vec3 normal;
			fscanf(file, "%f %f %f\n", &normal.x, &normal.y, &normal.z );
			temp_normals.push_back(normal);
		}
		else if (strcmp(lineHeader, "f") == 0){
			std::string vertex1, vertex2, vertex3;
			unsigned int vertexIndex[3], textureIndex[3], normalIndex[3];
			
			if (temp_normals.empty()) {
				int matches = fscanf(file, "%d/%d %d/%d %d/%d\n", &vertexIndex[0], &textureIndex[0], &vertexIndex[1], &textureIndex[1], &vertexIndex[2], &textureIndex[2] );
				if (matches != 6) printf("File can't be read by the parser...\n");
			}
			else {
				int matches = fscanf(file, "%d/%d/%d %d/%d/%d %d/%d/%d\n", &vertexIndex[0], &textureIndex[0], &normalIndex[0], &vertexIndex[1], &textureIndex[1],  &normalIndex[1], &vertexIndex[2], &textureIndex[2],  &normalIndex[2]);
				if (matches != 9) printf("File can't be read by the parser...\n");
				normals_.push_back(temp_normals[normalIndex[0] - 1]);
				normals_.push_back(temp_normals[normalIndex[1] - 1]);
				normals_.push_back(temp_normals[normalIndex[2] - 1]);
			}
			indices_.push_back(vertexIndex[0] - 1);
			indices_.push_back(vertexIndex[1] - 1);
			indices_.push_back(vertexIndex[2] - 1);
		}
	}

	// if normals are not given:
	if (normals_.empty()) {

		// calculate face normals
		std::vector<glm::vec3> face_normals;
		for (int i = 0; i < indices_.size(); i+=3) {
			glm::vec3 p1 = vertices_[indices_[i]];
			glm::vec3 p2 = vertices_[indices_[i+1]];
			glm::vec3 p3 = vertices_[indices_[i+2]];

			glm::vec3 U = p2 - p1;
			glm::vec3 V = p3 - p1;
			glm::vec3 normal = glm::vec3(U[1]*V[2] - U[2]*V[1], U[2]*V[0] - U[0]*V[2], U[0]*V[1] - U[1]*V[0]);

			face_normals.push_back(normal);
		}

		for (int i = 0; i < vertices_.size(); i++) {
			int count = 0;
			glm::vec3 normal;

			for (int j = 0; j < indices_.size(); j+=3) {
				if (i == indices_[j] || i == indices_[j+1] || i == indices_[j+2]) {
					count++;
					normal += face_normals[j/3];
				}
			}
			float c = (float)count;
			normals_.push_back(normal / c);
		}

	}

	// default colors
    for (unsigned int i = 0; i < vertices_.size(); i++) {
        colors_.push_back(glm::vec3(1,1,1));
    }
}




Intersection Mesh::intersectImpl(const Ray &ray) const
{    
	Intersection ret;
	ret.t = -1;
	vector<float> t_values;
	vector<glm::vec3> normal_values;

	for (int i = 0; i < indices_.size(); i+=3) {
		glm::vec3 p1 = vertices_.at(indices_.at(i));
		glm::vec3 p2 = vertices_.at(indices_.at(i+1));
		glm::vec3 p3 = vertices_.at(indices_.at(i+2));
		Intersection inter = intersectTri(glm::mat4(1.0f), ray, p1, p2, p3);
		if (inter.t != -1) {
			t_values.push_back(inter.t);
			normal_values.push_back(inter.normal);
		}
	}

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
		ret.t = minT;
		ret.normal = norm;
	}

    return ret;
}


Intersection Mesh::intersectTri(const glm::mat4 &T, const Ray &ray, glm::vec3 p1,  glm::vec3 p2,  glm::vec3 p3) const {
	
	Intersection isx;
	isx.t = -1;

    // get triangle edge vectors and plane normal
    glm::vec3 u = p2 - p1;
    glm::vec3 v = p3 - p1;
    glm::vec3 n = glm::cross(u, v);              
    if (n == glm::vec3(0))          // triangle is degenerate
        return isx;                  

    float a = -glm::dot(n, ray.orig - p1);
    float b = glm::dot(n, ray.dir);
    if (abs(b) < 0.00000001) {     // ray is  parallel to triangle plane
        return isx;
    }

    // get intersect point of ray with triangle plane
    float r = a / b;
    if (r < 0.0)
        return isx;      
    glm::vec3 I = ray.orig + r * ray.dir;   

    // is I inside T?
    float uu, uv, vv, wu, wv, D;
    uu = glm::dot(u,u);
    uv = glm::dot(u,v);
    vv = glm::dot(v,v);
    glm::vec3 w = I - p1;
    wu = glm::dot(w,u);
    wv = glm::dot(w,v);
    D = uv * uv - uu * vv;

    // get and test parametric coords
    float s, t;
    s = (uv * wv - vv * wu) / D;
    if (s < 0.0 || s > 1.0)         // I is outside T
        return isx;
    t = (uv * wu - uu * wv) / D;
    if (t < 0.0 || (s + t) > 1.0)   // I is outside T
        return isx;

	// define isx to return after transforming normal 
    isx.t = r;
	isx.normal = n;

	return isx;

}