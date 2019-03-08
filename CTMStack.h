
#pragma once
#include "Angel.h"

struct CTMNode {
	CTMNode* prev;
	mat4 matrix;
};

class CTMStack {
public:
	CTMStack(int intialProgram);
	void pushMatrix(mat4 matrix);
	mat4 popMatrix();
	mat4 peekMatrix();
	int getSize();
	void setUpCTM(mat4 ctm);
	void clear();
	void printContents();

private:
	int size;
	int program;
	CTMNode* head;
};