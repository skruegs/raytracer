#ifndef CUBE_H
#define CUBE_H

#include "Geometry.h"

class Cube : public Geometry {

public:
    Cube();
    virtual ~Cube();

    virtual void buildGeomtery();

    glm::vec3 center_;
    float length_;

protected:     
	virtual Intersection intersectImpl(const Ray &ray) const; 

};

#endif