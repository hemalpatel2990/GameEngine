#include <GL/glfw.h>
#include <assert.h>

#include "Mouse.h"

Mouse::Mouse()
{

}

// Use this to read mouse buttons
bool Mouse::GetKeyState(AZUL_MOUSE button)
{
	bool value;

	//if (glfwGetMouseButton(this->window, button) == GLFW_PRESS)
	if (glfwGetMouseButton( button) == GLFW_PRESS)
	{
		value = true;
	}
	else
	{
		value = false;
	}
	return value;
}

void Mouse::GetCursor(float &xPos, float &yPos)
{
	int xInt;
	int yInt;

	// get mouse position
	glfwGetMousePos(&xInt, &yInt);

	// why the hell would you use a double for a mouse cursor?
	// We went to the moon on 16 bits, why we need 64 for a mouse amazes me	
	xPos = (float)xInt;
	yPos = (float)yInt;

}