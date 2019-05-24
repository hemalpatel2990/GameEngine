#include <assert.h>

#include "Game.h"

int CALLBACK WinMain(HINSTANCE , HINSTANCE ,  LPSTR , int)                  
{                                                   
	Game *pGame = new Game("Demo", 800,600);
	pGame->run();                                  
                                   
	return 0;                                       
}

