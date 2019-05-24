#ifndef MODEL_H
#define MODEL_H

#include "MathEngine.h"
#include "sb6.h"

class Model
{
public:
	Model();
	Model(Model &copyModel) = delete;
	Model & operator = (Model &copyModel) = delete;
	virtual ~Model();
	
// Data
	int numVerts;
	int numTris;

	GLuint vao;
	GLuint vbo[2];

protected:
	// load VAO
	virtual void privCreateVAO( const char * const modelFileName ) = 0;

private:


};

#endif