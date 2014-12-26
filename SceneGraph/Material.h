#ifndef MATERIAL_H
#define MATERIAL_H

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <iostream>

class Material {

public:
	Material();
	virtual ~Material();

	// material attributes
	std::string name;
	glm::vec3 diff_color;
	glm::vec3 refl_color;
	float expo;				// specular exponent (shininess)
	float ior;				// index of refraction (transparency)
	int mirr;				// 1 = reflentance, 0 = specular
	int tran;				// 1 = refractive (transparent), 0 = opaque
	int emit_light;			// 0 = doesn't emit light, >0 = emittance value

	glm::vec3 calculateColor(glm::vec3, glm::vec3, glm::vec3, glm::vec3, glm::vec3);

};

#endif MATERIAL_H