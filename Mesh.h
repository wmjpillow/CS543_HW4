
#pragma once
#include <functional>
#include "Angel.h"
#include "Spotlight.h"

typedef Angel::vec4  color4;

struct point4 {
	vec4* point;
	vec4* normal;
	vec4* texCoord;
};
typedef struct point4 point4;

struct face {
	point4* p1;
	point4* p2;
	point4* p3;
};
typedef struct face Face;

class Mesh {
public:
	//Constructor, destructor
	Mesh(int polyCount, int vertCount);
	~Mesh();

	//Methods for building meshes
	void addVertex(float x, float y, float z);
	void addVertex(float x, float y, float z, float u, float v, float w);
	void addPoly(int vertIndex1, int vertIndex2, int vertIndex3);
	void Mesh::buildNormals();
	void normalize(); //Move to be centered on the origin, make the mesh's largest dimension 1
	void prepForDrawing();
	void setColor(vec4 newColor);
	void setTexture(const char* path);
	void shouldDrawWithTexture(bool shouldUseTexture);
	void setEnvironmentMap(const char* posX, const char* posY, const char* posZ, const char* negX, const char* negY, const char* negZ);
	void setShouldRefract(bool should);
	void setShouldReflect(bool should);
	void setShouldAddFog(bool should);
	void setFogChange(float density);
	void setReflectChange(float reflect);
	void setShouldChangeReflect(bool should);
	void setShouldChangeRefract(bool should);
	void setRefractChange(float refract);

	//Methods for getting information about meshes
	float getWidth(mat4 CTM = Angel::identity());
	float getHeight(mat4 CTM = Angel::identity());
	float getDepth(mat4 CTM = Angel::identity());
	vec3 getCenterPosition(mat4 CTM = Angel::identity());

	//Methods for drawing
	void drawMesh(int program, Spotlight* light);
	void drawShadows(int program, Spotlight* light, float dist, vec3 correction, vec3 planeRotation, mat4 modelView);

	//Methods for transforming meshes
	void moveTo(float x, float y, float z);
	void moveBy(float x, float y, float z);
	void rotateTo(float xRollAmount, float yRollAmount, float zRollAmount);
	void rotateBy(float xRollAmount, float yRollAmount, float zRollAmount);
	void scaleTo(float x, float y, float z);
	void scaleBy(float x);
	mat4 getModelMatrix();

private:
	//Model matrix stuff
	vec3 position;
	vec3 rotationAboutPosition;
	vec3 scale;
	vec4 color;

	//Polygon vars
	int numPolys;
	int polyListSize;
	Face** polys;

	//Vertex vars
	int numVerts;
	int vertsListSize;
	point4** verts;

	//Texture stuff
	GLuint texture;
	bool hasTexture;

	GLuint cubeMap;
	bool hasReflectionMap;
	bool shouldReflect;
	bool shouldRefract;
	bool shouldAddFog;
	float fogDensity;
	float ReflectShininess;
	bool shouldChangeReflect;
	bool shouldChangeRefract;
	float RefractShininess;

	//Drawing stuff
	vec4* meshGLPoints;

	//Convenience methods for mins and maxes
	vec4 calcNormal(Face* face);
	float Mesh::getMin(std::function<float(point4*)> func);
	float Mesh::getMax(std::function<float(point4*)> func);
};

Mesh* loadMeshFromPLY(char* filename);

