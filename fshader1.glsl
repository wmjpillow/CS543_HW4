#version 330 core

flat in vec4 flatColor;

in vec3 pos;
in vec3 fL;
in vec3 fE;
in vec3 fN;
in vec2 texCoord;
in vec3 reflectVec;
in vec3 refractVec;
out vec4 fColor;

uniform bool useTexture;
uniform vec4 overrideColor;
uniform sampler2D tex;

uniform bool shouldReflect;
uniform bool shouldRefract;
uniform samplerCube envMap;

uniform vec4 ambient, diffuse, specular;
uniform vec4 lightPos;
uniform vec4 lightDir;
uniform float lightFalloff;
uniform float lightCutoff;
uniform float shininess;
uniform mat4 modelMatrix;

// For fog use
uniform bool shouldAddFog;
uniform float fogDensity;
const vec3 fogColor  = vec3(0.1, 0.3, 0.7);

// change shininess
uniform float ReflectShininess;
uniform bool shouldChangeReflect;

// change refraction
uniform float RefractShininess;
uniform bool shouldChangeRefract;

void main()
{
	vec4 diffuseToUse;
	// check use texture
	if(useTexture) {
		diffuseToUse = texture(tex, texCoord);
	} else {
		diffuseToUse = diffuse;
	}

	// get 
	vec3 N = normalize(fN);
	vec3 E = normalize(-pos);
	vec3 L = normalize(fL);
	vec3 H = normalize(L + E);

	//Find intensity
	float lightPosAngle = acos(dot(L, normalize(lightDir.xyz)));
	float intensity = 0.0;
	if(lightPosAngle < lightCutoff) {
		intensity = pow(cos(lightPosAngle), lightFalloff) * 0.5;
	}

	//Calculate phong shaded color terms
	vec4 sAmbient = ambient * intensity;

	float cosTheta = max(dot(L, N), 0.0);
	vec4 sDiffuse = cosTheta * diffuseToUse * intensity * 2.0;

	float cosPhi = pow(max(dot(N, H), 0.0), shininess);
	vec4 sSpecular = cosPhi * specular * intensity;
	if(dot(L, N) < 0.0) {
		sSpecular = vec4(0.0, 0.0, 0.0, 0.0);
	}
	// shininess
	if(shouldChangeReflect) {
		sSpecular =  mix( sSpecular, vec4(1.0, 1.0, 1.0, 1.0), ReflectShininess );
	}
	//Build phong shaded color
	fColor = sAmbient + sDiffuse + sSpecular;
	fColor.a = 1.0;

	//Override color if set
	if(length(overrideColor) > 0.0) {
		fColor = overrideColor;
	}

	//Override with reflect/refract if set
	if(shouldReflect) {
		fColor =  texture(envMap, reflectVec);
	}
	if(shouldRefract) {
		fColor = mix(texture(envMap, refractVec), vec4(1.0, 1.0, 1.0, 1.0), 0.3);
	}

	// Add fog
	if (shouldAddFog){
        float fogFactor = 1 * fogDensity;
        fogFactor = clamp( fogFactor, 0.0, 1.0 );
        fColor.xyz = mix( fogColor, fColor.xyz, fogFactor );
	}

	// refract
	if (shouldChangeRefract && shouldRefract) { 
		vec3 changeRefractVec = mix(refractVec, vec3(1.0, 1.0, 1.0), RefractShininess);
		fColor = mix(texture(envMap, changeRefractVec), vec4(1.0, 1.0, 1.0, 1.0), 0.3);
	}
}
