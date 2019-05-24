#ifndef GRAPHICS_OBJECT_COLOR_BY_POSITION_H
#define GRAPHICS_OBJECT_COLOR_BY_POSITION_H

#include "GraphicsObject.h"

class GraphicsObject_ColorByPosition :public GraphicsObject
{
public:
	GraphicsObject_ColorByPosition(Model *model, ShaderObject *pShaderObj);

	// Rendermaterial contract
	virtual void SetState() override;
	virtual void SetDataGPU() override;	
	virtual void Draw() override;
	virtual void RestoreState() override;
	
// data:  place uniform instancing here

};
#endif