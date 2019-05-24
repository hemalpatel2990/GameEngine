#include <assert.h>

#include "GraphicsObject_FlatTexture.h"
#include "TextureManager.h"
#include "Model.h"
#include "Camera.h"
#include "CameraMan.h"


GraphicsObject_FlatTexture::GraphicsObject_FlatTexture(Model *model, ShaderObject *pShaderObj, TextureName name)
	: GraphicsObject(model, pShaderObj), textName(name)
{

}

void GraphicsObject_FlatTexture::SetState()
{
	glActiveTexture(GL_TEXTURE0);

	// Bind the texture
	GLuint textureID = TextureMan::find(this->textName);
	glBindTexture(GL_TEXTURE_2D, textureID);

	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CW);
}

void GraphicsObject_FlatTexture::SetDataGPU()
{
	// use this shader
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
	glUniformMatrix4fv(this->pShaderObj->GetLocation("world_matrix"), 1, GL_FALSE, (float *)&world);;
}

void GraphicsObject_FlatTexture::Draw()
{
	// draw
	//glDrawArrays(GL_TRIANGLES, 0, (3 * this->getModel()->numTris));
	//The starting point of the IBO
	glDrawElements(GL_TRIANGLES, 3 * this->getModel()->numTris, GL_UNSIGNED_INT, 0);
}

void GraphicsObject_FlatTexture::RestoreState()
{
	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CW);
}
