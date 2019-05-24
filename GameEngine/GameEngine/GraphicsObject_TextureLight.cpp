#include <assert.h>

#include "GraphicsObject_TextureLight.h"
#include "Model.h"
#include "TextureManager.h"
#include "Camera.h"
#include "CameraMan.h"

GraphicsObject_TextureLight::GraphicsObject_TextureLight(Model *model, ShaderObject *pShaderObj, TextureName name, Vect &LightColor, Vect &LightPos)
	: GraphicsObject(model, pShaderObj),
	textName(name)
{
	this->pLightColor = new Vect(LightColor);
	this->pLightPos = new Vect(LightPos);
}

GraphicsObject_TextureLight::~GraphicsObject_TextureLight()
{
	delete this->pLightColor;
	delete this->pLightPos;
}

void GraphicsObject_TextureLight::SetState()
{
	glActiveTexture(GL_TEXTURE0);

	// Bind the texture
	GLuint textureID = TextureMan::find(this->textName);
	glBindTexture(GL_TEXTURE_2D, textureID);

	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CW);
}

void GraphicsObject_TextureLight::SetDataGPU()
{
	// use this shader
	this->pShaderObj->SetActive();

	// set the vao
	glBindVertexArray(this->getModel()->vao);

	Camera *pCam = CameraMan::GetCurrent();

	Matrix world = this->getWorld();
	Matrix view = pCam->getViewMatrix();
	Matrix proj = pCam->getProjMatrix();

	glUniformMatrix4fv(this->pShaderObj->GetLocation("proj_matrix"), 1, GL_FALSE, (float *)&proj);
	glUniformMatrix4fv(this->pShaderObj->GetLocation("view_matrix"), 1, GL_FALSE, (float *)&view);
	glUniformMatrix4fv(this->pShaderObj->GetLocation("world_matrix"), 1, GL_FALSE, (float *)&world);

	Vect light = *this->pLightPos;
	Vect color = *this->pLightColor;
	glUniform3fv(this->pShaderObj->GetLocation("vLightPos"), 1, (float *)&light);
	glUniform4fv(this->pShaderObj->GetLocation("vColor"), 1, (float *)&color);
}


void GraphicsObject_TextureLight::Draw()
{
	//The draw triangles 
	//glDrawArrays(GL_TRIANGLES, 0, (3 * this->getModel()->numTris));
	//The starting point of the IBO
	glDrawElements(GL_TRIANGLES, 3 * this->getModel()->numTris, GL_UNSIGNED_INT, 0);
}

void GraphicsObject_TextureLight::RestoreState()
{
	// undo the states you corrupted
	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CW);
}
