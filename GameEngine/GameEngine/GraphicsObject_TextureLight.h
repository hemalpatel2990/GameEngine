#ifndef GRAPHICS_OBJECT_TEXTURE_LIGHT_H
#define GRAPHICS_OBJECT_TEXTURE_LIGHT_H

#include "GraphicsObject.h"
#include "TextureManager.h"

class GraphicsObject_TextureLight :public GraphicsObject
{
public:
	GraphicsObject_TextureLight(Model *model, ShaderObject *pShaderObj, TextureName  name, Vect &LightColor, Vect &LightPos);
	GraphicsObject_TextureLight() = delete;
	~GraphicsObject_TextureLight();

	// Rendermaterial contract
	virtual void SetState() override;
	virtual void SetDataGPU() override;
	virtual void Draw() override;
	virtual void RestoreState() override;

	// data:  place uniform instancing here

	TextureName textName;
	Vect *pLightColor;
	Vect *pLightPos;

};
#endif