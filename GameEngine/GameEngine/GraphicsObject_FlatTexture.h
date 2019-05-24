#ifndef GRAPHICS_OBJECT_FLAT_TEXTURE_H
#define GRAPHICS_OBJECT_FLAT_TEXTURE_H

#include "GraphicsObject.h"
#include "TextureManager.h"

class GraphicsObject_FlatTexture :public GraphicsObject
{
public:
	GraphicsObject_FlatTexture(Model *model, ShaderObject *pShaderObj, TextureName name);

	// Rendermaterial contract
	virtual void SetState() override;
	virtual void SetDataGPU() override;
	virtual void Draw() override;
	virtual void RestoreState() override;

	// data:  place uniform instancing here

	TextureName textName;

};
#endif