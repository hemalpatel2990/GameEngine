#include <assert.h>
#include "MathEngine.h"
#include "Model.h"
#include "GraphicsObject_ColorByPosition.h"
#include "Camera.h"
#include "CameraMan.h"

GraphicsObject_ColorByPosition::GraphicsObject_ColorByPosition(Model *model, ShaderObject *pShaderObj)
: GraphicsObject(model, pShaderObj)
{

}

void GraphicsObject_ColorByPosition::SetState()
{
    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CW);
}

void GraphicsObject_ColorByPosition::SetDataGPU()
{
        // Set program
	this->pShaderObj->SetActive();

	// set the vao
	glBindVertexArray(this->getModel()->vao);
	
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

void GraphicsObject_ColorByPosition::Draw()
{	     
	// draw
	//glDrawArrays(GL_TRIANGLES, 0, (3 * this->getModel()->numVerts));
	//The starting point of the IBO
	glDrawElements(GL_TRIANGLES, 3 * this->getModel()->numTris, GL_UNSIGNED_INT, 0);   


}

void GraphicsObject_ColorByPosition::RestoreState()
{	
    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CW);
}