#include <sb6.h>
#include <assert.h>

#include "NullModel.h"


NullModel::NullModel(const char * const modelFileName)
	: Model()
{
	modelFileName;
}

NullModel::~NullModel()
{
	// remove anything dynamic here
}

void NullModel::privCreateVAO(const char * const modelFileName)
{
	modelFileName;
}

