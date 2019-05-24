#include <assert.h>
#include <GL/glfw.h>

#include "Keyboard.h"

Keyboard::Keyboard()
{

}

// Use this to read keyboard
bool Keyboard::GetKeyState( AZUL_KEY key)
{
	bool value;

	if( glfwGetKey(key) == GLFW_PRESS )
	{
		value = true;
	}
	else
	{
		value = false;
	}
	return value;
}
