#ifndef NODE_H
#define NODE_H

#include "Geometry.h"
#include "Cube.h"
#include "Cylinder.h"
#include "Sphere.h"
#include "Mesh.h"
#include "Material.h"

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
	Material* mat;
	glm::mat4x4 translate_matrix;
	glm::mat4x4 rotate_matrix;
	glm::mat4x4 scale_matrix;
	glm::mat4x4 transformation_matrix;

	// shows currently selected node
	bool highlight;

	// transformation functions
    void translate(float, float, float);
    void rotate(float, float, float);
    void scale(float, float, float);

    void del(Node*);
};

#endif 