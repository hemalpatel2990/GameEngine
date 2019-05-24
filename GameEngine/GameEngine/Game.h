#ifndef GAME_H
#define GAME_H
#include "Engine.h"
#include "Time.h"
#include "Timer.h"

#include <sb6.h>

#include <math.h>

#include <vmath.h>
// ToDo get rid of the projection, move to the camera

#include "MathEngine.h"

class Game: public Engine
{
public:
	// constructor
	Game( const char* windowName, const int Width,const int Height);

    virtual void Initialize();
    virtual void LoadContent();
	virtual void Update(float currentTime);
    virtual void Draw();
    virtual void UnLoadContent();

	virtual void ClearBufferFunc();
		
   // virtual void onResize(int w, int h);

private:	
	// prevent them from calling wrong call
	Game();

	Timer			intervalTimer;
	Timer			globalTimer;

};

#endif