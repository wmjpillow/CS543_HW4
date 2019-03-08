
#include "Spotlight.h"

Spotlight::Spotlight(vec3 pos, vec3 dir, float cut) {
	position = vec4(pos, 1);
	direction = dir;
	cutoff = cut;
}

vec4 Spotlight::getPosition() {
	return position;
}

vec4 Spotlight::getDirection() {
	return vec4(direction, 1);
}

float Spotlight::getCutoff() {
	return cutoff;
}

void Spotlight::setCutoff(float newCutoff) {
	cutoff = newCutoff;
}
