#ifndef GRAPHICS_OBJECT_CONST_COLOR_H
#define GRAPHICS_OBJECT_CONST_COLOR_H

#include "GraphicsObject.h"

class GraphicsObject_ConstColor :public GraphicsObject
{
public:
	GraphicsObject_ConstColor(Model *model, ShaderObject *pShaderObj, Vect &LightColor, Vect &LightPos);

	// Rendermaterial contract
	virtual void SetState() override;
	virtual void SetDataGPU() override;
	virtual void Draw() override;
	virtual void RestoreState() override;

	// data:  place uniform instancing here
	Vect *pLightColor;
	Vect *pLightPos;
};
#endif