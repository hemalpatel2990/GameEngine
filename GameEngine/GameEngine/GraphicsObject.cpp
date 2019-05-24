#include <assert.h>

#include "GraphicsObject.h"
#include "MathEngine.h"
#include "Model.h"


GraphicsObject::GraphicsObject(Model *model, ShaderObject *_pShaderObj)
	: pModel(model), pShaderObj(_pShaderObj)
{
	this->pWorld = new Matrix(MatrixSpecialType::IDENTITY);
	assert(model);
	assert(_pShaderObj);
}

GraphicsObject::~GraphicsObject()
{
	delete this->pWorld;
}

void GraphicsObject::Render(   )
{
	this->SetState();
	this->SetDataGPU();
	this->Draw();
	this->RestoreState();
}

Model *GraphicsObject::getModel() const
{
	return this->pModel;
}

Matrix &GraphicsObject::getWorld() 
{
	return *this->pWorld;
}

void GraphicsObject::SetWorld( Matrix &_world)
{
	*this->pWorld = _world;
}
