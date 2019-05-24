#ifndef TEXTURE_NODE_H
#define TEXTURE_NODE_H

#include "sb6.h"

#define TEXTURE_ASSET_NAME_SIZE 64

enum class TextureName
{
	STONES,
	RED_BRICK,
	DUCKWEED,
	ROCKS,
	NOT_INITIALIZED
};

class TextureNodeLink
{
public:
	TextureNodeLink()
	{
		this->next = 0;
		this->prev = 0;
	}

	TextureNodeLink *next;
	TextureNodeLink *prev;
};

class TextureNode: public TextureNodeLink
{
public:
	TextureNode();
	void set( const char * const _assetName, 
				TextureName _name, 
				GLuint _TextureID, 
				GLenum minFilter, 
				GLenum magFilter, 
				GLenum wrapMode);

private:
	char assetName[TEXTURE_ASSET_NAME_SIZE];

public:
	TextureName name;
	GLuint textureID;
	GLenum minFilter;
	GLenum magFilter;
	GLenum wrapMode;
};



#endif