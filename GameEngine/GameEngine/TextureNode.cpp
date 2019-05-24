#include <string.h>
#include <sb6.h>


#include "TextureNode.h"

TextureNode::TextureNode()
: name(TextureName::NOT_INITIALIZED),
textureID(0),
minFilter(GL_LINEAR),
magFilter(GL_LINEAR),
wrapMode(GL_CLAMP_TO_EDGE)
{
	memset( this->assetName, 0, TEXTURE_ASSET_NAME_SIZE );
}


void TextureNode::set( const char * const _assetName, 
						TextureName _name, 
						GLuint _TextureID, 
						GLenum _minFilter, 
						GLenum _magFilter, 
						GLenum _wrapMode)
{
	memcpy( this->assetName, _assetName, TEXTURE_ASSET_NAME_SIZE-1);
	this->name = _name;
	this->magFilter = _magFilter;
	this->minFilter = _minFilter;
	this->wrapMode = _wrapMode;
	this->textureID = _TextureID;
}
