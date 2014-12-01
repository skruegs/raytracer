#include "Material.h"


Material::Material() {
	name = "no_name_set";
	diff_color = glm::vec3(0.4,0.4,0.4);
	refl_color = glm::vec3(0.0,0.0,0.0);
	expo = 0;		
	ior  = 0;		
	mirr = 0;		
	tran = 0;
}


Material::~Material() 
{
}


glm::vec3 Material::calculateColor(glm::vec3 eye_pos, glm::vec3 normal, glm::vec3 point, glm::vec3 lightPos, glm::vec3 lightColor) {
	
	glm::vec3 lightDir = glm::normalize(lightPos - point);
	float d = glm::length(lightDir);

	// calculate diffuse
	float diff_intensity = glm::clamp(glm::dot(normal, lightDir),-0.05f,1.0f);
	glm::vec3 diffuse = diff_intensity * diff_color / (d);

	// calculate specular
	glm::vec3 H = glm::normalize(lightDir + glm::normalize(eye_pos - point));
	float spec_intensity = pow(glm::dot(normal, H), expo);
	glm::vec3 specular;
	if (expo == 0) 
		specular = glm::vec3(0,0,0);
	else
		specular = spec_intensity * lightColor / (2*d);

	// define ambient
	glm::vec3 ambient = glm::vec3(0.1,0.1,0.1);
	
	// calculate diffuse + specular + ambient
	glm::vec3 color = diffuse + specular + ambient;

	// constrain 0 <= R, G, B <= 1
	color[0] = glm::clamp(color[0], 0.0f, 1.0f);
	color[1] = glm::clamp(color[1], 0.0f, 1.0f);
	color[2] = glm::clamp(color[2], 0.0f, 1.0f);

	return color;

}