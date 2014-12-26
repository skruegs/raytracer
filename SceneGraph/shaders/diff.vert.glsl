#version 130
// ^ Change this to version 130 if you have compatibility issues

uniform mat4 u_Model;
uniform mat4 u_ModelInvTr;
uniform mat4 u_ViewProj;
uniform vec4 u_LightPos;
uniform vec4 u_LightColor;

in vec3 vs_Position;
in vec3 vs_Normal;
in vec3 vs_Color;

out vec4 fs_Normal;
out vec4 fs_LightVector;
out vec4 fs_Color;
out vec3 vertPos;
out vec4 lightColor;


void main()
{
	gl_Position = u_ViewProj * u_Model * vec4(vs_Position, 1.0);

    vec4 vertPos4 = u_Model * vec4(vs_Position, 1.0);
    vertPos = vec3(vertPos4) / vertPos4.w;

    fs_Normal = vec4(u_ModelInvTr * vec4(vs_Normal, 0.0));
	fs_Color = vec4(vs_Color, 1.0);
	fs_LightVector = u_LightPos - vertPos4;

	lightColor = u_LightColor;
}
