#include <assert.h>

#include "Model.h"
#include "GraphicsObject_Wireframe.h"
#include "ShaderObject.h"
#include "Camera.h"
#include "CameraMan.h"

GraphicsObject_Wireframe::GraphicsObject_Wireframe(Model *model, ShaderObject *pShaderObj)
: GraphicsObject(model, pShaderObj)
{
	assert(model);
	assert(pShaderObj);
}

void GraphicsObject_Wireframe::SetState()
{	
    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CW);
}

void GraphicsObject_Wireframe::SetDataGPU()
{	     
	// Draw Me: -----------------------------------------

        // Set program
	this->pShaderObj->SetActive();
	
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

}

void GraphicsObject_Wireframe::Draw( )
{	     
	// draw
	glDrawElements(GL_TRIANGLES, 3 * this->getModel()->numTris, GL_UNSIGNED_INT, 0);
}

void GraphicsObject_Wireframe::RestoreState()
{	     
	// Restore state
	glEnable(GL_CULL_FACE);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}
