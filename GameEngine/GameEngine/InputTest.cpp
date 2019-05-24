#include <string>

#include "Trace.h"
#include "InputMan.h"
#include "Keyboard.h"


void KeyboardTest()
{
	Keyboard *key = InputMan::GetKeyboard();

	// Quick and dirty test, if these work the rest do.
	// ---> try a,s,d,<SPACE> keys
	std::string a;
	std::string b;
	std::string c;
	std::string d;

	if (key->GetKeyState(AZUL_KEY::KEY_A))
	{
		a = " A";
	}

	if (key->GetKeyState(AZUL_KEY::KEY_S))
	{
		b = " S";
	}

	if (key->GetKeyState(AZUL_KEY::KEY_D))
	{
		c = " D";
	}

	if (key->GetKeyState(AZUL_KEY::KEY_SPACE))
	{
		d = " <SPACE>";
	}

	std::string total = a + b + c + d;
	Trace::out("Key:%s \n", total.c_str());

	
	
}
void MouseTest()
{
	Mouse *mouse = InputMan::GetMouse();

	// Quick and dirty test, if these work the rest do.
	// --> try move the mouse inside the window, click right, click left
	std::string a;
	std::string b;

	float xPos;
	float yPos;

	// get mouse position
	mouse->GetCursor(xPos, yPos);

	// read mouse buttons
	if (mouse->GetKeyState(AZUL_MOUSE::BUTTON_RIGHT))
	{
		a = " <right>";
	}

	if (mouse->GetKeyState(AZUL_MOUSE::BUTTON_LEFT))
	{
		b = " <left>";
	}

	std::string total = a + b;


	Trace::out("mouse:(%d,%d)%s \n", (int)xPos, (int)yPos, total.c_str());
}

