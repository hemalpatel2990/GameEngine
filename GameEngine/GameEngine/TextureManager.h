#ifndef TEXTURE_MANAGER_H
#define TEXTURE_MANAGER_H

#include "sb6.h"
#include "TextureNode.h" 

// Singleton
class TextureMan
{
public:
	static void addTexture( const char * const _assetName, const TextureName _name);
	static void deleteTextures();
	static GLuint find( const TextureName _name);

private:  // methods

	static TextureMan *privGetInstance();
	TextureMan();

	                      
	// helper private methods
	void privLoadTexture( const char * const _assetName, GLuint *&textureID );
	bool privLoadTGATexture(const char *szFileName, GLenum minFilter, GLenum magFilter, GLenum wrapMode);
	void privAddToFront(TextureNodeLink *node, TextureNodeLink *&head);
	GLbyte *gltReadTGABits(const char *szFileName, GLint *iWidth, GLint *iHeight, GLint *iComponents, GLenum *eFormat);

private:  // add

	TextureNodeLink *active;

};

#endif