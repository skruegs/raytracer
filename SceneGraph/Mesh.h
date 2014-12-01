#ifndef MESH_H
#define MESH_H
#include <iostream>
#include <fstream>
#include <string>
#include <stdio.h> 

#include "Geometry.h"

class Mesh : public Geometry {

public:
    Mesh();
	Mesh(std::string);
    virtual ~Mesh();

    virtual void buildGeomtery();

	std::string filename;
	Intersection intersectTri(const glm::mat4 &T, const Ray &ray, glm::vec3,  glm::vec3,  glm::vec3) const; 
	
protected:     
	virtual Intersection intersectImpl(const Ray &ray) const; 

};

#endif