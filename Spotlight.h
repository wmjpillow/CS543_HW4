
#pragma once
#include "Angel.h"

class Spotlight {
public:
	Spotlight(vec3 pos, vec3 dir, float cut);
	vec4 getPosition();
	vec4 getDirection();
	float getCutoff();
	void setCutoff(float newCutoff);

private:
	vec4 position;
	vec3 direction;
	float cutoff;
};