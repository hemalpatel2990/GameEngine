#ifndef SHADER_OBJECT_H
#define SHADER_OBJECT_H

#include "sb6.h"

enum class ShaderName
{
	NULL_SHADER,
	COLOR_LIGHT,
	COLOR_SINGLE,
	TEXTURE_SIMPLE,
	TEXTURE_POINT_LIGHT,
	CONST_COLOR,
};


class ShaderObject
{
public:
	ShaderObject( ShaderName shaderName, const char * const shaderBaseFileName );
	ShaderObject() = delete;
	ShaderObject(const ShaderObject &) = delete;
	ShaderObject &operator = (ShaderObject &) = delete;
	~ShaderObject();

	void SetActive();
	GLuint GetLocation( const char * uniformName);

private:
	bool privCreateShader(GLuint &programObject, const char * const shaderBaseFileName  );
	bool privLoadNCompile(GLuint &shaderObject, const char * const shaderFileName, GLenum shader_type );

public:
// data
	ShaderName  name;
	GLuint		programObject;

};

#endif