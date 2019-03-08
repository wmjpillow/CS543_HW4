
#include "Angel.h"
#include "Mesh.h"
#include "Spotlight.h"
#include "CTMStack.h"

#define BASE_ROTATE_SPEED 0.25f
#define CUTOFF_CHANGE_AMOUNT 50
#define FOG_CHANGE_UNIT 0.1
#define SHININESS_CHANGE_UNIT 0.1
#define REFRACTION_CHANGE_UNIT 0.1
#define ROOT_TWO_OVER_TWO sqrt(2.0f) / 2.0f

//remember to prototype
void setUpOpenGLBuffers();
void display();
void drawShadows(Mesh* m);
void resize(int newWidth, int newHeight);
void update();
void keyboard(unsigned char key, int x, int y);

using namespace std;

//Globals
GLuint program;
GLuint global_program;
int width  = 0;
int height = 0;
CTMStack ctmStack(0);
Mesh* floorMesh;
Mesh* wall1;
Mesh* wall2;
Mesh* root;
Spotlight* light;

bool shouldRefract = false;
bool shouldReflect = false;
bool shouldDrawShadows = true;
bool areWallsTextured = true;
bool shouldAddFog = true;
bool shouldChangeReflect = true;
bool shouldChangeRefract = true;
float currentRotationAmount;
float fogDensity = 0.5;
float ReflectShininess = 0.5;
float RefractShininess = 0.5;

//Set up openGL buffers
void setUpOpenGLBuffers() {	
    //Create a vertex array object
    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    //Create and initialize a buffer object
    GLuint buffer;
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);

	//Load shaders and use the resulting shader program
    program = InitShader("vshader1.glsl", "fshader1.glsl");
    glUseProgram(program);

	//Setting up ctm
	ctmStack = CTMStack(program);

    //set up vertex arrays
    GLuint vPosition = glGetAttribLocation( program, "vPosition" );
    glEnableVertexAttribArray(vPosition);
    glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 3 * 4 * sizeof(float), BUFFER_OFFSET(0));

	GLuint vNormal = glGetAttribLocation(program, "vNormal");
	glEnableVertexAttribArray(vNormal);
	glVertexAttribPointer(vNormal, 4, GL_FLOAT, GL_FALSE, 3 * 4 * sizeof(float), BUFFER_OFFSET(4 * sizeof(float)));

	GLuint vTexCoord = glGetAttribLocation(program, "vTexCoord");
	glEnableVertexAttribArray(vTexCoord);
	glVertexAttribPointer(vTexCoord, 4, GL_FLOAT, GL_FALSE, 3 * 4 * sizeof(float), BUFFER_OFFSET(2 * 4 * sizeof(float)));

	//Set up light
	GLuint ambient = glGetUniformLocationARB(program, "ambient");
	glUniform4f(ambient, 0, 0, 0, 1);

	GLuint specular = glGetUniformLocationARB(program, "specular");
	glUniform4f(specular, 1, 1, 1, 1);

	GLuint shininess = glGetUniformLocationARB(program, "shininess");
	glUniform1f(shininess, 100);

	GLuint lightPos = glGetUniformLocationARB(program, "lightPos");
	glUniform4f(lightPos, light->getPosition().x, light->getPosition().y, light->getPosition().z, 0);

	GLuint lightDir = glGetUniformLocationARB(program, "lightDir");
	glUniform4f(lightDir, light->getDirection().x, light->getDirection().y, light->getDirection().z, 0);

	//sets the default color to clear screen
    glClearColor(0.0, 0.0, 0.0, 1.0); // black background
}

//Draw the current model with the appropriate model and view matrices
void display() {
	//Clear the window and depth buffer
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	//Build the view matrix
	Angel::mat4 perspectiveMat = Angel::Perspective((GLfloat)60.0, (GLfloat)width/(GLfloat)height, (GLfloat)0.2, (GLfloat) 120.0);
	float viewMatrixf[16];
	viewMatrixf[0]  = perspectiveMat[0][0];viewMatrixf[4]  = perspectiveMat[0][1];
	viewMatrixf[1]  = perspectiveMat[1][0];viewMatrixf[5]  = perspectiveMat[1][1];
	viewMatrixf[2]  = perspectiveMat[2][0];viewMatrixf[6]  = perspectiveMat[2][1];
	viewMatrixf[3]  = perspectiveMat[3][0];viewMatrixf[7]  = perspectiveMat[3][1];
	viewMatrixf[8]  = perspectiveMat[0][2];viewMatrixf[12] = perspectiveMat[0][3];
	viewMatrixf[9]  = perspectiveMat[1][2];viewMatrixf[13] = perspectiveMat[1][3];
	viewMatrixf[10] = perspectiveMat[2][2];viewMatrixf[14] = perspectiveMat[2][3];
	viewMatrixf[11] = perspectiveMat[3][2];viewMatrixf[15] = perspectiveMat[3][3];

	//Set up projection matricies
	GLuint viewMatrix = glGetUniformLocationARB(program, "projectionMatrix");
	glUniformMatrix4fv(viewMatrix, 1, GL_FALSE, viewMatrixf);

	//Draw the floors/walls
	glEnable(GL_STENCIL_TEST); //Stencil test prevents semi-tranparent shadows from drawing twice
	glStencilFunc(GL_ALWAYS, 111, ~0);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

	ctmStack.pushMatrix(floorMesh->getModelMatrix());
	ctmStack.popMatrix();
	floorMesh->drawMesh(program, light);
	ctmStack.pushMatrix(Angel::identity());

	ctmStack.pushMatrix(wall1->getModelMatrix());
	ctmStack.popMatrix();
	wall1->drawMesh(program, light);
	ctmStack.pushMatrix(Angel::identity());

	ctmStack.pushMatrix(wall2->getModelMatrix());
	ctmStack.popMatrix();
	wall2->drawMesh(program, light);
	ctmStack.pushMatrix(Angel::identity());

	glDisable(GL_STENCIL_TEST);

	//Draw the sculpture 
	ctmStack.pushMatrix(root->getModelMatrix());
	ctmStack.peekMatrix();
	root->drawMesh(program, light);
	drawShadows(root);

	//Clean up the matrix stack
	ctmStack.clear();

	//Flush and show
    glFlush();
	glutSwapBuffers();
}

void drawShadows(Mesh* m) {
	if (shouldDrawShadows) {
		m->drawShadows(program, light, 0.5f, vec3( 0,    0, 0), vec3(0, 0, 0), ctmStack.peekMatrix()); //Floor
		m->drawShadows(program, light, 1.0f, vec3( 0, -0.5, 0), vec3(90, 45, 0), ctmStack.peekMatrix()); //Wall
	}
}


//Update the size of the viewport, report the new width and height, redraw the scene
void resize(int newWidth, int newHeight) {
	width = newWidth;
	height = newHeight;
	glViewport(0, 0, width, height);
	display();
}

//Update rotation
void update() {
	root->rotateBy  (0, 0, 0);
	//root->rotateBy(0, -BASE_ROTATE_SPEED, 0);
	display();

	currentRotationAmount += BASE_ROTATE_SPEED;
}

//keyboard handler
void keyboard(unsigned char key, int x, int y)
{
	switch (key) {
	//Exit
	case 033:
		exit(EXIT_SUCCESS);
		break;
	case 'P':
		light->setCutoff(light->getCutoff() + CUTOFF_CHANGE_AMOUNT);
		break;
	case 'p':
		light->setCutoff(light->getCutoff() - CUTOFF_CHANGE_AMOUNT);
		break;
	case 'a':
	case 'A':
		shouldDrawShadows = !shouldDrawShadows; 
		break;
	case 'b':
	case 'B':
		areWallsTextured = !areWallsTextured;
		floorMesh->shouldDrawWithTexture(areWallsTextured);
		wall1->shouldDrawWithTexture(areWallsTextured);
		wall2->shouldDrawWithTexture(areWallsTextured);
		break;
	case 'c':
	case 'C':
		shouldReflect = !shouldReflect;
		shouldRefract = false;
		root->setShouldReflect(shouldReflect);
		break;
	case 'd':
	case 'D':
		shouldRefract = !shouldRefract;
		shouldReflect = false;
		root->setShouldRefract(shouldRefract);
		break;
	case 'S':
		root->setShouldChangeReflect(shouldChangeReflect);
		ReflectShininess += SHININESS_CHANGE_UNIT;
		root->setReflectChange(ReflectShininess);// printf("%.1f\n", ReflectShininess);
		break;
	case 's':
		root->setShouldChangeReflect(shouldChangeReflect);
		ReflectShininess -= SHININESS_CHANGE_UNIT;
		root->setReflectChange(ReflectShininess);// printf("%.1f\n", ReflectShininess);
		break;
	case 'f':  
		root->setShouldAddFog(shouldAddFog);
		fogDensity += FOG_CHANGE_UNIT;
		root->setFogChange(fogDensity); //printf("%.2lf\n", fogDensity);
		break;
	case 'F': 
		root->setShouldAddFog(shouldAddFog);
		fogDensity -= FOG_CHANGE_UNIT; 
		root->setFogChange(fogDensity); //printf("%.2lf\n",fogDensity);
		break;
	case 'i':
		root->setShouldRefract(true);
		root->setShouldChangeRefract(shouldChangeRefract);
		RefractShininess -= REFRACTION_CHANGE_UNIT;
		root->setRefractChange(RefractShininess); printf("%.1f\n", RefractShininess);
		break;
	case 'I':
		root->setShouldRefract(true);
		root->setShouldChangeRefract(shouldChangeRefract);
		RefractShininess += REFRACTION_CHANGE_UNIT;
		root->setRefractChange(RefractShininess); printf("%.1f\n", RefractShininess);

		break;
	}
}

//Make a flat square mesh
Mesh* makeWall() {
	Mesh* m = new Mesh(2, 4);
	m->addVertex(-1, -1, 0, 0, 0, 0);
	m->addVertex(-1,  1, 0, 0, 3, 0);
	m->addVertex( 1,  1, 0, 3, 3, 0);
	m->addVertex( 1, -1, 0, 3, 0, 0);
	m->addPoly(0, 2, 1);
	m->addPoly(0, 3, 2);

	m->prepForDrawing();

	return m;
}

//entry point
int main( int argc, char **argv ) {
	//Set up light
	light = new Spotlight(vec3(0.75f, 0.6f, 0.0f), vec3(1.0f, 0.75f, 1.0f), 360);

	//init glut
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
	glutInitWindowSize(800, 600);
	width = 800;
	height = 600;

	//create window
	glutCreateWindow("hw4");

	//init glew
	glewInit();

	//Set up buffers
	setUpOpenGLBuffers();

	//assign handlers
	glutDisplayFunc(display);
	glutReshapeFunc(resize);
	glutIdleFunc(update);
	glutKeyboardFunc(keyboard);



	//Setting up the walls and floor
	floorMesh = makeWall();
	floorMesh->setTexture("Resources/grass.bmp");
	floorMesh->setColor(vec4(0.5, 0.5, 0.5, 1));
	floorMesh->rotateBy(270, 0, 45);
	floorMesh->moveBy(0, -0.5, 0);

	wall1 = makeWall();
	wall1->setTexture("Resources/stones.bmp");
	wall1->setColor(vec4(0, 0, 1, 1));
	wall1->rotateBy(0, 45, 0);
	wall1->moveBy(-ROOT_TWO_OVER_TWO, 0, -ROOT_TWO_OVER_TWO);

	wall2 = makeWall();
	wall2->setTexture("Resources/stones.bmp");
	wall2->setColor(vec4(0, 0, 1, 1));
	wall2->rotateBy(0, -45, 0);
	wall2->moveBy(ROOT_TWO_OVER_TWO, 0, -ROOT_TWO_OVER_TWO);

	root = loadMeshFromPLY("Resources/happy.ply");
	root->setEnvironmentMap("Resources/nvposx.bmp", "Resources/nvposy.bmp", "Resources/nvposz.bmp", "Resources/nvnegx.bmp", "Resources/nvnegy.bmp", "Resources/nvnegz.bmp");
	root->setColor(vec4(1.0f, 0.0f, 0.0f, 1.0f));
	root->scaleBy(0.7f);
	root->moveBy(0,-0.02f, -0.8f);

	//enter the drawing loop
    glutMainLoop();
    return 0;
}
