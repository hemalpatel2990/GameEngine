#ifndef CUBE_MODEL_H
#define CUBE_MODEL_H

#include "Model.h"

class CubeModel : public Model
{
public:
	CubeModel( const char * const modelFileName );
	CubeModel() = delete;
	CubeModel(const CubeModel &) = delete;
	CubeModel & operator = (CubeModel &copyModel) = delete;
	~CubeModel();

private:
	virtual void privCreateVAO( const char * const modelFileName  ) override;

// Data

};

#endif