#include <assert.h>

#include "Model.h"
#include "GraphicsObject_WireframeConstantColor.h"
#include "ShaderObject.h"
#include "Camera.h"
#include "CameraMan.h"

GraphicsObject_WireframeConstantColor::GraphicsObject_WireframeConstantColor(Model *model, ShaderObject *pShaderObj, Vect &wireColor)
: GraphicsObject(model, pShaderObj)
{
	assert(model);
	assert(pShaderObj);
	this->pWireColor = new Vect(wireColor);
}


void GraphicsObject_WireframeConstantColor::SetState()
{	
    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CW);
}

void GraphicsObject_WireframeConstantColor::SetDataGPU()
{	     
	// Draw Me: -----------------------------------------

	// set the vao
	glBindVertexArray(this->getModel()->vao);
	
		// Set state
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glDisable(GL_CULL_FACE);

	// use this shader
	this->pShaderObj->SetActive();

	Camera *pCam = CameraMan::GetCurrent();

	Matrix world = this->getWorld();
	Matrix view = pCam->getViewMatrix();
	Matrix proj = pCam->getProjMatrix();

	glUniformMatrix4fv(this->pShaderObj->GetLocation("proj_matrix"), 1, GL_FALSE, (float *)&proj);
	glUniformMatrix4fv(this->pShaderObj->GetLocation("view_matrix"), 1, GL_FALSE, (float *)&view);
	glUniformMatrix4fv(this->pShaderObj->GetLocation("world_matrix"), 1, GL_FALSE, (float *)&world);

	glUniform4fv(this->pShaderObj->GetLocation("wire_color"), 1, (float *)this->pWireColor);
}

void GraphicsObject_WireframeConstantColor::Draw( )
{	     
	//The starting point of the IBO
	glDrawElements(GL_TRIANGLES, 3 * this->getModel()->numTris, GL_UNSIGNED_INT, 0);
}

void GraphicsObject_WireframeConstantColor::RestoreState()
{	     
	// Restore state
	glEnable(GL_CULL_FACE);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}
