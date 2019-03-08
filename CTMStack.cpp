#include "CTMStack.h"

CTMStack::CTMStack(int intialProgram) {
	program = intialProgram;
	head = nullptr;
	size = 0;
}

void CTMStack::pushMatrix(mat4 matrix) {
	CTMNode* newNode = new CTMNode;
	newNode->matrix = (head != nullptr ? head->matrix : Angel::identity()) * matrix;
	newNode->prev = head;

	head = newNode;
	size++;
}

//Returns the itentity matrix if the stack is empty
mat4 CTMStack::popMatrix() {
	mat4 result = Angel::identity();
	if (size != 0) {
		result = head->matrix;

		//Remove from stack
		CTMNode* newPrev = head->prev;
		delete head;
		head = newPrev;
		size--;
	}
	setUpCTM(result);

	return result;
}

mat4 CTMStack::peekMatrix() {
	mat4 result = Angel::identity();
	if (size != 0) {
		result = head->matrix;
	}
	setUpCTM(result);

	return result;
}

void CTMStack::setUpCTM(mat4 ctm) {
	//Set up ctm
	float CTMf[16];
	CTMf[0] = ctm[0][0]; CTMf[4] = ctm[0][1];
	CTMf[1] = ctm[1][0]; CTMf[5] = ctm[1][1];
	CTMf[2] = ctm[2][0]; CTMf[6] = ctm[2][1];
	CTMf[3] = ctm[3][0]; CTMf[7] = ctm[3][1];
	CTMf[8] = ctm[0][2]; CTMf[12] = ctm[0][3];
	CTMf[9] = ctm[1][2]; CTMf[13] = ctm[1][3];
	CTMf[10] = ctm[2][2]; CTMf[14] = ctm[2][3];
	CTMf[11] = ctm[3][2]; CTMf[15] = ctm[3][3];
	GLuint ctmLocation = glGetUniformLocationARB(program, "modelMatrix");
	glUniformMatrix4fv(ctmLocation, 1, GL_FALSE, CTMf);
}

void CTMStack::clear() {
	while (head != nullptr) {
		CTMNode* temp = head->prev;
		delete head;
		head = temp;
	}
	size = 0;
}

void CTMStack::printContents() {
	CTMNode* curr = head;
	printf("=======  SIZE %d  ======\n", size);
	while (curr != nullptr) {
		mat4 mat = curr->matrix;
		printf("| %04f %04f %04f %04f |\n", mat[0][0], mat[0][1], mat[0][2], mat[0][3]);
		printf("| %04f %04f %04f %04f |\n", mat[1][0], mat[1][1], mat[1][2], mat[1][3]);
		printf("| %04f %04f %04f %04f |\n", mat[2][0], mat[2][1], mat[2][2], mat[2][3]);
		printf("| %04f %04f %04f %04f |\n", mat[3][0], mat[3][1], mat[3][2], mat[3][3]);
		printf("\n");
		curr = curr->prev;
	}
	printf("=======================\n");
}


int CTMStack::getSize() {
	return size;
}
