#include "Node.h"
#define GLM_FORCE_RADIANS
static const float PI = 3.141592653589f;

Node::Node() {
	name = "no_name_set";
    geo = NULL;
    std::vector<Node*> children;
    parent = NULL;
	color = glm::vec3(0.5,0.5,0.5);
    mat = NULL;

	highlight = false;

	translate_matrix = glm::mat4x4(1);
    rotate_matrix = glm::mat4x4(1);
    scale_matrix = glm::mat4x4(1);
	transformation_matrix = translate_matrix * (rotate_matrix * (scale_matrix));
}

Node::~Node() {}

void Node::translate(float x, float y, float z) {
	translate_matrix[3][0] = x;
	translate_matrix[3][1] = y;
	translate_matrix[3][2] = z;
	transformation_matrix = translate_matrix * (rotate_matrix * (scale_matrix));
}
    
void Node::rotate(float a, float b, float c) {
	float rad_a = a * (PI/180);
	float rad_b = b * (PI/180);
	float rad_c = c * (PI/180);

	glm::mat4 rx = glm::mat4(1.0f);
	rx[1][1] = cos(rad_a);
	rx[2][1] = -sin(rad_a);
	rx[1][2] = sin(rad_a);
	rx[2][2] = cos(rad_a);

	glm::mat4 ry = glm::mat4(1.0f);
	ry[0][0] = cos(rad_b );
	ry[2][0] = sin(rad_b );
	ry[0][2] = -sin(rad_b );
	ry[2][2] = cos(rad_b );

	glm::mat4 rz = glm::mat4(1.0f);
	rz[0][0] = cos(rad_c);
	rz[1][0] = -sin(rad_c);
	rz[0][1] = sin(rad_c);
	rz[1][1] = cos(rad_c);

	rotate_matrix = rx*ry*rz;
	transformation_matrix = translate_matrix * (rotate_matrix * (scale_matrix));
}
    
void Node::scale(float x, float y, float z) {
	scale_matrix[0][0] = x;
	scale_matrix[1][1] = y;
	scale_matrix[2][2] = z;
	if (scale_matrix[0][0] < 0.1) scale_matrix[0][0] = 0.1;

	transformation_matrix = translate_matrix * (rotate_matrix * (scale_matrix));
}



void Node::del(Node* n) {
	 for (unsigned int i = 0; i < children.size(); i++) {
       if(children.at(i) == n)
           children.erase(children.begin() + i);
    }
}

