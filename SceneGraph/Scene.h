#define GLM_FORCE_RADIANS
#include "glm/glm.hpp"
#include <iostream>
#include <fstream>
#include <string>
#include <stdio.h> 
#include <iomanip>

#include "Node.h"
#include "Material.h"
#include "EasyBMP.h"
#include "Ray.h"
#include "Intersection.h"

#ifndef SCENE_H
#define SCENE_H

class Scene {

public:
	Scene();

	bool readFile(const char* filename);

	// raytrace functions
	void traceImage();
	glm::vec3 traceRay(Node* root, Ray ray, int depth);
	// helper functions
	void intersect(Node* n, glm::mat4, Ray);
	void pointToLightIntersect(Node* n, glm::mat4 t, Ray ray); 

	// global vars
	Node* root;
	Intersection* intersec;
	Node* intersected_node;
	bool lightBlocked;

	// lists
	std::vector<Node*> nodes;
	std::vector<Material*> materials;

	// config file vars
	float width;
	float height;
	glm::vec3 pos;
	glm::vec3 viewDir;
	glm::vec3 viewUp;
	float fovy;
	glm::vec3 lightPos;
	glm::vec3 lightColor;

};


#endif