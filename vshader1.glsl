#version 330 core

in  vec4 vPosition;
in  vec4 vNormal;
in  vec4 vTexCoord;
out vec2 texCoord;
flat out vec4 flatColor;
out vec3 pos;
out vec3 fL;
out vec3 fN;
out vec3 reflectVec;
out vec3 refractVec;


uniform mat4 projectionMatrix;
uniform mat4 modelMatrix;
uniform vec4 ambient, diffuse, specular;
uniform vec4 lightPos;
uniform vec4 lightDir;
uniform float lightFalloff;
uniform float lightCutoff;
uniform float shininess;
void main()
{
	//Calculate needed vectors (Need for both flat and smooth shading)
	pos = (modelMatrix * vPosition).xyz;
	fL = normalize(lightPos.xyz - pos);
	vec3 fE = normalize(-pos);
	vec3 fH = normalize(fL + fE);
	fN = normalize((modelMatrix * vNormal).xyz);

	//Calculate reflection and refraction vectors
	vec3 eyePos = vPosition.xyz;
	reflectVec = reflect(eyePos, fN);
	refractVec = refract(eyePos, fN, 0.75);

	texCoord = vTexCoord.st;
	gl_Position = projectionMatrix * modelMatrix * vPosition;
}
