#ifndef INPUT_MAN_H
#define INPUT_MAN_H

#include "Keyboard.h"
#include "Mouse.h"

class InputMan
{
public:
	static void Create();
	static Keyboard *GetKeyboard();
	static Mouse *GetMouse();
	~InputMan();

private:
	static InputMan *privInstance();
	static void privCreate();
	InputMan();
	
	// Data
	static InputMan *pInputMan;
	Keyboard		*pKeyboard;
	Mouse			*pMouse;

};
#endif