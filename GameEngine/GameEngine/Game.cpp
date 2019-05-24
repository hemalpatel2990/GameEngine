#include <assert.h>
#include <sb6.h>
#include <math.h>

#include "Trace.h"
#include "GameObject.h"
#include "Game.h"
#include "ShaderObject.h"
#include "TextureManager.h"
#include "InputMan.h"
#include "CameraMan.h"
#include "GameObjectMan.h"
#include "Timer.h"
#include "Anim.h"
#include "AnimMan.h"

extern GameObject *pFirstBone;



Game::Game(const char* windowName, const int Width, const int Height)
	:Engine(windowName, Width, Height)
{
	this->globalTimer.tic();
	this->intervalTimer.tic();
}

void Game::Initialize()
{
	InputMan::Create();
}


void Game::LoadContent()
{
	// Camera setup

		// Camera_0		
		Camera *pCam0 = new Camera( );
		pCam0->setViewport(0, 0, this->info.windowWidth, this->info.windowHeight);
	
		pCam0->setPerspective(35.0f, float(pCam0->getScreenWidth()) / float(pCam0->getScreenHeight()), 1.0f, 500.0f);
		pCam0->setOrientAndPosition(Vect(0.0f, 0.0f, 1.0f), Vect(0.0f, 0.0f, 10.0f), Vect(180.0f, 0.0f, 90.0f));

		pCam0->updateCamera();
		CameraMan::Add(Camera::Name::CAMERA_0, pCam0);
		CameraMan::SetCurrent(Camera::Name::CAMERA_0);

	// Textures
		TextureMan::addTexture("Rocks.tga", TextureName::ROCKS);
		TextureMan::addTexture("Stone.tga", TextureName::STONES);
		TextureMan::addTexture("RedBrick.tga", TextureName::RED_BRICK);
		TextureMan::addTexture("Duckweed.tga", TextureName::DUCKWEED);

		// setup relationship and hierarchy
		SetAnimationHierarchy();

//		SetAnimationNew();
	//	SetAnimationData();

		ProcessAnimation(Time(TIME_ZERO));
}

void Game::Update(float)
{
	// Mark our end time.
	this->intervalTimer.toc();
	this->intervalTimer.tic();

	// Update camera - make sure everything is consistent
	Camera *pCam = CameraMan::GetCurrent();
	pCam->updateCamera();

	// Game objects
	GameObjectMan::Update(this->globalTimer.toc());

	AnimMan::Update();
	

}

//-----------------------------------------------------------------------------
// Game::Draw()
//		This function is called once per frame
//	    Use this for draw graphics to the screen.
//      Only do rendering here
//-----------------------------------------------------------------------------
void Game::Draw()
{
	// draw all objects
	GameObjectMan::Draw();
}

//-----------------------------------------------------------------------------
// Game::UnLoadContent()
//       unload content (resources loaded above)
//       unload all content that was loaded before the Engine Loop started
//-----------------------------------------------------------------------------
void Game::UnLoadContent()
{
	// todo fix this.
}


//------------------------------------------------------------------
// Game::ClearBufferFunc()
// Allows user to change the way the clear buffer function works
//------------------------------------------------------------------
void Game::ClearBufferFunc()
{
	const GLfloat blue[] = { 0.8f, 0.8f, 1.2f, 1.0f };
	const GLfloat one = 1.0f;

	glViewport(0, 0, info.windowWidth, info.windowHeight);
	glClearBufferfv(GL_COLOR, 0, blue);
	glClearBufferfv(GL_DEPTH, 0, &one);
}
