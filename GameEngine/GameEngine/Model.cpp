
#include "Model.h"

Model::Model()
	: numVerts(0), numTris(0), vao(0)
{
	this->vbo[0] = 0;
	this->vbo[1] = 0;
}

Model::~Model()
{
}

