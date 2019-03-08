
#include <limits.h>
#include "Mesh.h"
#include "textfile.h"
#include "bmpread.h"

//Constructor, destructor
Mesh::Mesh(int polyCount, int vertCount) {
	position = vec3(0, 0, 0);
	rotationAboutPosition = vec3(0, 0, 0);
	scale = vec3(1, 1, 1);
	color = vec4(1, 1, 1, 1);

	polyListSize = polyCount;
	numPolys = 0;
	polys = new Face*[polyCount];

	vertsListSize = vertCount;
	numVerts = 0;
	verts = new point4*[vertCount];

	hasTexture = false;
	shouldReflect = false;
	shouldRefract = false;
	shouldAddFog = false;
	fogDensity = 0.5;
	ReflectShininess = 0.5;
	shouldChangeReflect = false;
	RefractShininess = 0.5;
	shouldChangeRefract = false;
}
Mesh::~Mesh() {
	for (int i = 0; i < numPolys; i++) { delete polys[i]; }
	for (int i = 0; i < numVerts; i++) { 
		delete verts[i]->point;
		delete verts[i]->normal;
		delete verts[i]; 
	}
	delete polys;
	delete verts;
}

//Methods for building meshes
void Mesh::addVertex(float x, float y, float z) {
	addVertex(x, y, z, 0, 0, 0);
}

void Mesh::addVertex(float x, float y, float z, float u, float v, float w) {
	if (numVerts >= vertsListSize) {
		printf("Failed to add point (%f, %f, %f) - verts list is full!\n", x, y, z);
		return;
	}
	point4* toAdd = new point4();
	toAdd->point = new vec4(x, y, z, 1);
	toAdd->normal = new vec4(0, 0, 0, 1);
	toAdd->texCoord = new vec4(u, v, w, 1);

	verts[numVerts] = toAdd;
	numVerts++;
}

void Mesh::addPoly(int vertIndex1, int vertIndex2, int vertIndex3) {
	if (numPolys >= polyListSize) {
		printf("Failed to add poly with verts %d, %d, %d - poly list is full!\n", vertIndex1, vertIndex2, vertIndex3);
		return;
	}

	Face* toAdd = new Face();
	toAdd->p1 = verts[vertIndex1];
	toAdd->p2 = verts[vertIndex2];
	toAdd->p3 = verts[vertIndex3];
	polys[numPolys] = toAdd;
	numPolys++;
}

void Mesh::buildNormals() {
	for (int i = 0; i < numVerts; i++) {
		vec4 pointNormal = vec4(0, 0, 0, 1);
		for (int j = 0; j < numPolys; j++) {
			if (polys[j]->p1 == verts[i] || polys[j]->p2 == verts[i] || polys[j]->p3 == verts[i]) {
				pointNormal += calcNormal(polys[j]);
			}
		}

		vec4 normalized = Angel::normalize(pointNormal);
		normalized.w = 0;
		verts[i]->normal = new vec4(normalized);
	}
}

void Mesh::normalize() {
	//Center and scale the mesh
	float meshWidth = getWidth();
	float meshHeight = getHeight();
	float meshDepth = getDepth();
	float biggestDimension = fmaxf(meshWidth, fmaxf(meshHeight, meshDepth));
	float scaleFactor = 1.0f / biggestDimension;

	mat4 correctionTransform = Angel::identity();
	mat4 moveToOrigin = Angel::Translate(getCenterPosition() * -1);
	mat4 normalizeScale = Angel::Scale(scaleFactor, scaleFactor, scaleFactor);
	correctionTransform = correctionTransform * normalizeScale;
	for (int i = 0; i < numVerts; i++) {
		vec4 temp = correctionTransform * *(verts[i]->point);
		verts[i]->point->x = temp.x;
		verts[i]->point->y = temp.y;
		verts[i]->point->z = temp.z;
	}
}

void Mesh::prepForDrawing() {
	buildNormals();

	int bufferSize = 3 * 3 * numPolys;
	meshGLPoints = new vec4[bufferSize];
	int pointsIndex = 0;
	for (int i = 0; i < numPolys; i++) {
		meshGLPoints[pointsIndex] = *(polys[i]->p1->point);  pointsIndex++;
		meshGLPoints[pointsIndex] = *(polys[i]->p1->normal); pointsIndex++;
		meshGLPoints[pointsIndex] = *(polys[i]->p1->texCoord); pointsIndex++;

		meshGLPoints[pointsIndex] = *(polys[i]->p2->point);  pointsIndex++;
		meshGLPoints[pointsIndex] = *(polys[i]->p2->normal); pointsIndex++;
		meshGLPoints[pointsIndex] = *(polys[i]->p2->texCoord); pointsIndex++;

		meshGLPoints[pointsIndex] = *(polys[i]->p3->point);  pointsIndex++;
		meshGLPoints[pointsIndex] = *(polys[i]->p3->normal); pointsIndex++;
		meshGLPoints[pointsIndex] = *(polys[i]->p3->texCoord); pointsIndex++;
	}
}

void Mesh::setColor(vec4 newColor) {
	color = newColor;
}

void Mesh::setTexture(const char* path) {
	bmpread_t bitmap;
	if (!bmpread(path, 0, &bitmap)) {
		fprintf(stderr, "%s:error loading bitmap file\n", path);
		exit(1);
	}

	glActiveTexture(GL_TEXTURE0);
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, bitmap.width, bitmap.height, 0, GL_RGB, GL_UNSIGNED_BYTE, bitmap.rgb_data);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	bmpread_free(&bitmap);
	hasTexture = true;
}

void Mesh::shouldDrawWithTexture(bool shouldUseTexture) {
	hasTexture = shouldUseTexture;
}

void Mesh::setEnvironmentMap(const char* posX, const char* posY, const char* posZ, const char* negX, const char* negY, const char* negZ) {
	bmpread_t bitmapPosX;
	bmpread_t bitmapPosY;
	bmpread_t bitmapPosZ;
	bmpread_t bitmapNegX;
	bmpread_t bitmapNegY;
	bmpread_t bitmapNegZ;

	bool success = true;
	success = success && bmpread(posX, 0, &bitmapPosX);
	success = success && bmpread(posY, 0, &bitmapPosY);
	success = success && bmpread(posZ, 0, &bitmapPosZ);
	success = success && bmpread(negX, 0, &bitmapNegX);
	success = success && bmpread(negY, 0, &bitmapNegY);
	success = success && bmpread(negZ, 0, &bitmapNegZ);

	if (!success) {
		fprintf(stderr, "%s:error loading bitmap file\n");
		exit(1);
	}

	glActiveTexture(GL_TEXTURE1);
	glGenTextures(1, &cubeMap);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMap);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_RGB, bitmapPosX.width, bitmapPosX.height, 0, GL_RGB, GL_UNSIGNED_BYTE, bitmapPosX.rgb_data);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_RGB, bitmapPosY.width, bitmapPosY.height, 0, GL_RGB, GL_UNSIGNED_BYTE, bitmapPosY.rgb_data);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_RGB, bitmapPosZ.width, bitmapPosZ.height, 0, GL_RGB, GL_UNSIGNED_BYTE, bitmapPosZ.rgb_data);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_RGB, bitmapNegX.width, bitmapNegX.height, 0, GL_RGB, GL_UNSIGNED_BYTE, bitmapNegX.rgb_data);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_RGB, bitmapNegY.width, bitmapNegY.height, 0, GL_RGB, GL_UNSIGNED_BYTE, bitmapNegY.rgb_data);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_RGB, bitmapNegZ.width, bitmapNegZ.height, 0, GL_RGB, GL_UNSIGNED_BYTE, bitmapNegZ.rgb_data);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	bmpread_free(&bitmapPosX);
	bmpread_free(&bitmapPosY);
	bmpread_free(&bitmapPosZ);
	bmpread_free(&bitmapNegX);
	bmpread_free(&bitmapNegY);
	bmpread_free(&bitmapNegZ);
	hasReflectionMap = true;
}

void Mesh::setShouldRefract(bool should) {
	shouldRefract = should;
	shouldReflect = false;
}

void Mesh::setShouldReflect(bool should) {
	shouldReflect = should;
	shouldRefract = false;
}

void Mesh::setShouldAddFog(bool should) {
	shouldAddFog = should;
}

void Mesh::setFogChange(float density) {
	fogDensity = density;
}

void Mesh::setReflectChange(float reflect) {
	ReflectShininess = reflect;
}

void Mesh::setShouldChangeReflect(bool should) {
	shouldChangeReflect = should;
}

void Mesh::setShouldChangeRefract(bool should) {
	shouldChangeRefract = should;
}

void Mesh::setRefractChange(float refract) {
	RefractShininess = refract;
}


//Methods for getting information about meshes
float Mesh::getWidth(mat4 CTM) {
	auto getXLambda = [mat = CTM](point4* point) -> float { return (mat * *(point->point)).x; };
	return getMax(getXLambda) - getMin(getXLambda);
}
float Mesh::getHeight(mat4 CTM) {
	auto getYLambda = [mat = CTM](point4* point) -> float { return (mat * *(point->point)).y; };
	return getMax(getYLambda) - getMin(getYLambda);
}
float Mesh::getDepth(mat4 CTM) {
	auto getZLambda = [mat = CTM](point4* point) -> float { return (mat * *(point->point)).z; };
	return getMax(getZLambda) - getMin(getZLambda);
}
vec3 Mesh::getCenterPosition(mat4 CTM) {
	auto getXLambda = [](point4* point) -> float { return point->point->x; };
	auto getYLambda = [](point4* point) -> float { return point->point->y; };
	auto getZLambda = [](point4* point) -> float { return point->point->z; };

	float xCenter = (getMin(getXLambda) + getMax(getXLambda)) / 2.0f;
	float yCenter = (getMin(getYLambda) + getMax(getYLambda)) / 2.0f;
	float zCenter = (getMin(getZLambda) + getMax(getZLambda)) / 2.0f;

	vec4 transformedPos = CTM * vec3(xCenter, yCenter, zCenter);
	return vec3(transformedPos.x, transformedPos.y, transformedPos.z);
}

//Methods for drawing
void Mesh::drawMesh(int program, Spotlight* light) {
	//Send the data to GPU
	int bufferSize = 3 * 3 * numPolys;
	glBufferData(GL_ARRAY_BUFFER, sizeof(vec4) * bufferSize, meshGLPoints, GL_STATIC_DRAW);

	//Clear global color
	GLuint vColor = glGetUniformLocationARB(program, "overrideColor");
	glUniform4f(vColor, 0, 0, 0, 0);

	//Set up color
	GLuint diffuse = glGetUniformLocationARB(program, "diffuse");
	glUniform4f(diffuse, color.x, color.y, color.z, color.w);

	//Set up lighting
	GLuint lightFalloff = glGetUniformLocationARB(program, "lightFalloff");
	glUniform1f(lightFalloff, 20.0f / (light->getCutoff() / 45.0f));

	GLuint lightCutoff = glGetUniformLocationARB(program, "lightCutoff");
	glUniform1f(lightCutoff, light->getCutoff() * Angel::DegreesToRadians);

	// Set up textures
	if (hasTexture && !(shouldRefract || shouldReflect)) {
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture);
		GLuint texture = glGetUniformLocation(program, "texture");
		glUniform1i(texture, 0);
	}
	GLuint useTexture = glGetUniformLocationARB(program, "useTexture");
	glUniform1i(useTexture, hasTexture ? GL_TRUE : GL_FALSE);

	//Set up environment map
	if (hasReflectionMap && (shouldRefract || shouldReflect)) {
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMap);
		GLuint envMap = glGetUniformLocation(program, "envMap");
		glUniform1i(envMap, 1);
	}
	GLuint reflect = glGetUniformLocation(program, "shouldReflect");
	glUniform1i(reflect, shouldReflect ? GL_TRUE : GL_FALSE);
	GLuint refract = glGetUniformLocation(program, "shouldRefract");
	glUniform1i(refract, shouldRefract ? GL_TRUE : GL_FALSE);
	GLuint addfog = glGetUniformLocation(program, "shouldAddFog");
	glUniform1i(addfog, shouldAddFog ? GL_TRUE : GL_FALSE);
	GLuint fogdensity = glGetUniformLocation(program, "fogDensity");
	glUniform1f(fogdensity, fogDensity);
	GLuint reflectshininess = glGetUniformLocation(program, "ReflectShininess");
	glUniform1f(reflectshininess, ReflectShininess);
	GLuint changereflect = glGetUniformLocation(program, "shouldChangeReflect");
	glUniform1i(changereflect, shouldChangeReflect ? GL_TRUE : GL_FALSE);
	GLuint refractshininess = glGetUniformLocation(program, "RefractShininess");
	glUniform1f(refractshininess, RefractShininess);
	GLuint changerefract = glGetUniformLocation(program, "shouldChangeRefract");
	glUniform1i(changerefract, shouldChangeRefract ? GL_TRUE : GL_FALSE);

	//Draw the PLY model
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glEnable(GL_DEPTH_TEST);
	glDrawArrays(GL_TRIANGLES, 0, 3 * numPolys);
	glDisable(GL_DEPTH_TEST);
	
	glUniform1i(useTexture, GL_FALSE);
	glUniform1i(reflect, GL_FALSE);
	glUniform1i(refract, GL_FALSE);
}
void Mesh::drawShadows(int program, Spotlight* light, float dist, vec3 correction, vec3 planeRotation, mat4 modelView) {
	//Shadow matrix
	vec4 offsetPos = vec4(light->getPosition().x,
						  light->getPosition().y + dist - 0.0001, //Move the shadow slightly closer to avoid z-fighting
						  light->getPosition().z,
						  1);

	mat4 shadowMat = Angel::identity();
	shadowMat[3][1] = -1.0f / offsetPos.y;
	shadowMat[3][3] = 0;

	mat4 rotate = Angel::RotateX(-planeRotation.x) * Angel::RotateY(-planeRotation.y) * Angel::RotateZ(-planeRotation.z);
	mat4 rotateBack = Angel::RotateX(planeRotation.x) * Angel::RotateY(planeRotation.y) * Angel::RotateZ(planeRotation.z);
	mat4 rotateToPlane = Angel::Translate(correction) *Angel::RotateY(planeRotation.y) * Angel::RotateZ(-planeRotation.y);
	mat4 shadowProjMat = rotateToPlane * rotateBack * Angel::Translate(light->getPosition()) * shadowMat * Angel::Translate(-light->getPosition()) * rotate * modelView;

	float mm[16];
	mm[0] =  shadowProjMat[0][0]; mm[4] =  shadowProjMat[0][1];
	mm[1] =  shadowProjMat[1][0]; mm[5] =  shadowProjMat[1][1];
	mm[2] =  shadowProjMat[2][0]; mm[6] =  shadowProjMat[2][1];
	mm[3] =  shadowProjMat[3][0]; mm[7] =  shadowProjMat[3][1];
	mm[8] =  shadowProjMat[0][2]; mm[12] = shadowProjMat[0][3];
	mm[9] =  shadowProjMat[1][2]; mm[13] = shadowProjMat[1][3];
	mm[10] = shadowProjMat[2][2]; mm[14] = shadowProjMat[2][3];
	mm[11] = shadowProjMat[3][2]; mm[15] = shadowProjMat[3][3];
	GLuint ctmLocation = glGetUniformLocationARB(program, "modelMatrix");
	glUniformMatrix4fv(ctmLocation, 1, GL_FALSE, mm);

	//Send the data to GPU
	int bufferSize = 3 * 3 * numPolys;
	glBufferData(GL_ARRAY_BUFFER, sizeof(vec4) * bufferSize, meshGLPoints, GL_STATIC_DRAW);

	//Clear global color
	GLuint vColor = glGetUniformLocationARB(program, "overrideColor");
	glUniform4f(vColor, 0, 0, 0, 0.5f);

	//Draw the shadow polygons
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	glStencilFunc(GL_EQUAL, 111, ~0);
	glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);

	glEnable(GL_STENCIL_TEST);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDrawArrays(GL_TRIANGLES, 0, 3 * numPolys);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_STENCIL_TEST);

	//Clean 
	mm[0] =  modelView[0][0]; mm[4] =  modelView[0][1];
	mm[1] =  modelView[1][0]; mm[5] =  modelView[1][1];
	mm[2] =  modelView[2][0]; mm[6] =  modelView[2][1];
	mm[3] =  modelView[3][0]; mm[7] =  modelView[3][1];
	mm[8] =  modelView[0][2]; mm[12] = modelView[0][3];
	mm[9] =  modelView[1][2]; mm[13] = modelView[1][3];
	mm[10] = modelView[2][2]; mm[14] = modelView[2][3];
	mm[11] = modelView[3][2]; mm[15] = modelView[3][3];
	glUniformMatrix4fv(ctmLocation, 1, GL_FALSE, mm);
}

//Methods for transforming meshes
void Mesh::moveTo(float x, float y, float z) {
	position = vec3(x, y, z);
}
void Mesh::moveBy(float x, float y, float z) {
	position += vec3(x, y, z);
}
void Mesh::rotateTo(float xRollAmount, float yRollAmount, float zRollAmount) {
	rotationAboutPosition = vec3(xRollAmount, yRollAmount, zRollAmount);
}
void Mesh::rotateBy(float xRollAmount, float yRollAmount, float zRollAmount) {
	rotationAboutPosition += vec3(xRollAmount, yRollAmount, zRollAmount);
}
void Mesh::scaleTo(float x, float y, float z) {
	scale = vec3(x, y, z);
}
void Mesh::scaleBy(float x) {
	scale *= x;
}
mat4 Mesh::getModelMatrix() {
	mat4 ModelMatrix = Angel::identity();
	mat4 scaleMatrix = Angel::Scale(scale);
	mat4 rotateMatrix = Angel::RotateX(rotationAboutPosition.x) * Angel::RotateY(rotationAboutPosition.y) * Angel::RotateZ(rotationAboutPosition.z);
	mat4 translateMatrix = Angel::Translate(position);

	ModelMatrix = ModelMatrix * translateMatrix * rotateMatrix * scaleMatrix;
	return ModelMatrix;
}

//Convenience stuff
vec4 Mesh::calcNormal(Face* face) {
	vec4 vectors[4];
	vectors[0] = *face->p1->point;
	vectors[1] = *face->p2->point;
	vectors[2] = *face->p3->point;
	vectors[3] = *face->p1->point;

	float mx = 0;
	float my = 0;
	float mz = 0;

	for (int i = 0; i < 3; i++) {
		mx += (vectors[i].y - vectors[i + 1].y) * (vectors[i].z + vectors[i + 1].z);
		my += (vectors[i].z - vectors[i + 1].z) * (vectors[i].x + vectors[i + 1].x);
		mz += (vectors[i].x - vectors[i + 1].x) * (vectors[i].y + vectors[i + 1].y);
	}

	vec4 normalized = vec4(Angel::normalize(vec3(mx, my, mz)), 0);
	return normalized;
}
float Mesh::getMin(std::function<float(point4*)> func) {
	float min = INFINITY;
	for (int i = 0; i < numVerts; i++) {
		if (func(verts[i]) < min) { min = func(verts[i]); }
	}
	return min;
}
float Mesh::getMax(std::function<float(point4*)> func) {
	float max = -INFINITY;
	for (int i = 0; i < numVerts; i++) {
		if (func(verts[i]) > max) { max = func(verts[i]); }
	}
	return max;
}

//Loading meshes
Mesh* loadMeshFromPLY(char* filename) {
	int success;
	char* fileContents = textFileRead(filename);

	//Tokenize on newlines
	char* token = strtok(fileContents, "\n");

	//Bail if not PLY
	if (!strcmp(token, "ply\n")) {
		printf("File \"%s\" is not a PLY file!\n", filename);
		return new Mesh(0, 0);
	}
	token = strtok(nullptr, "\n");
	token = strtok(nullptr, "\n"); //Skip ASCII line

	//Read in verts
	int numVerts;
	success = sscanf(token, "element vertex %d", &numVerts);
	if (success != 1) {
		printf("could not read number of verts from file \"%s\"\n", filename);
		return new Mesh(0, 0);
	}
	token = strtok(nullptr, "\n");
	token = strtok(nullptr, "\n"); //Skip 1st property line
	token = strtok(nullptr, "\n"); //Skip 2nd property line
	token = strtok(nullptr, "\n"); //Skip 3rd property line

	//Read in polys
	int numPolys;
	success = sscanf(token, "element face %d", &numPolys);
	if (success != 1) {
		printf("could not read number of polys from file \"%s\"\n", filename);
		return new Mesh(0, 0);
	}
	token = strtok(nullptr, "\n");
	token = strtok(nullptr, "\n"); //Skip 4th property line
	token = strtok(nullptr, "\n"); //Skip "end_header" line

	//Build the vertex list
	Mesh* mesh = new Mesh(numPolys, numVerts);

	//Read in verts
	for (int i = 0; i < numVerts; i++) {
		float x, y, z;
		success = sscanf(token, "%f %f %f", &x, &y, &z);
		if (success != 3) {
			printf("could not read vert #%d from file \"%s\"\n", i, filename);
			return new Mesh(0, 0);
		}
		mesh->addVertex(x, y, z);
		token = strtok(nullptr, "\n");
	}

	//Read in polys
	for (int i = 0; i < numPolys; i++) {
		int p1, p2, p3;
		success = sscanf(token, "3 %d %d %d", &p1, &p2, &p3);
		if (success != 3) {
			printf("could not read poly #%d from file \"%s\"\n", i, filename);
			return new Mesh(0, 0);
		}
		mesh->addPoly(p1, p2, p3);
		token = strtok(nullptr, "\n");
	}

	mesh->normalize();
	mesh->prepForDrawing();

	return mesh;
}
