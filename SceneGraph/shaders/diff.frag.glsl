#version 130

in vec4 fs_Normal;
in vec4 fs_LightVector;
in vec4 fs_Color;
in vec3 vertPos;
in vec4 lightColor;


out vec4 out_Color;

void main()
{
	vec4 diffuseColor = fs_Color;
	vec4 specColor = lightColor;

	vec3 normal = normalize(vec3(fs_Normal));
	vec3 lightDir = normalize(vec3(fs_LightVector) - vertPos);
	float lambertian = max(dot(lightDir,normal), 0.0);
	float specular = 0.0;

	if(lambertian > 0.0) {
		vec3 viewDir = normalize(-vertPos);
		vec3 reflectDir = reflect(-(lightDir), (normal));
		float specAngle = max(dot(reflectDir, viewDir), 0.0);
		specular = pow(specAngle, 16.0);
	}

	float lightIntensity = lambertian + 0.2 + specular;
	out_Color = vec4((diffuseColor.rgb + specColor.rgb) * lightIntensity, diffuseColor.a);
}




