//#define GLM_FORCE_RADIANS
#include "glm/glm.hpp"
#include <iostream>
#include <fstream>
#include <string>
#include <stdio.h> 

#include "Node.h"
#include "EasyBMP.h"
#include "Ray.h"
#include "Intersection.h"

#ifndef SCENE_H
#define SCENE_H

class Scene {

public:
	Scene();

	bool readFile(const char* filename);

	void Raycast();
	void traverse(Node* n, glm::mat4, Ray);
	Intersection* intersec;
	Node* root;

	std::vector<Node*> nodes;

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