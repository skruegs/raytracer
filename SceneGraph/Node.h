#ifndef NODE_H
#define NODE_H

#include "Geometry.h"
#include "Cube.h"
#include "Cylinder.h"
#include "Sphere.h"
#include "Mesh.h"

class Node {

public:
	Node();
	virtual ~Node();

	// node attributes
	std::string name;
	Geometry *geo;
	std::vector<Node*> children;
	Node* parent;
	glm::vec3 color;
	glm::mat4x4 translate_matrix;
	glm::mat4x4 rotate_matrix;
	glm::mat4x4 scale_matrix;
	glm::mat4x4 transformation_matrix;

	bool highlight;

	// functions
    void translate(float, float, float);
    void rotate(float, float, float);
    void scale(float, float, float);

    void del(Node*);
};

#endif 