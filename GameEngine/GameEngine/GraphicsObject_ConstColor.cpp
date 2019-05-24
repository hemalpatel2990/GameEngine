#include <assert.h>
#include "MathEngine.h"
#include "Model.h"
#include "GraphicsObject_ConstColor.h"
#include "Camera.h"
#include "CameraMan.h"



GraphicsObject_ConstColor::GraphicsObject_ConstColor(Model *model, ShaderObject *pShaderObj,Vect &LightColor, Vect &LightPos)
	: GraphicsObject(model, pShaderObj)
{
	this->pLightColor = new Vect(LightColor);
	this->pLightPos = new Vect(LightPos);
}

void GraphicsObject_ConstColor::SetState()
{
	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CW);
}

void GraphicsObject_ConstColor::SetDataGPU()
{
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

	Vect light = *this->pLightPos;
	Vect color = *this->pLightColor;
	glUniform3fv(this->pShaderObj->GetLocation("vLightPos"), 1, (float *)&light);
	glUniform4fv(this->pShaderObj->GetLocation("vColor"), 1, (float *)&color);



}

void GraphicsObject_ConstColor::Draw()
{
	// draw
	glDrawArrays(GL_TRIANGLES, 0, (3 * this->getModel()->numVerts));
}

void GraphicsObject_ConstColor::RestoreState()
{
	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CW);
}