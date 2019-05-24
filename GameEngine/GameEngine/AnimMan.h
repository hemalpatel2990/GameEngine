#ifndef ANIM_MAN_H
#define ANIM_MAN_H

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
#include "FrameBucket.h"

class AnimMan
{
public:
	static void Update();
	static void SetHead(Frame_Bucket *);
	static Frame_Bucket * GetHead();
	static void SetBone(GameObject *);
	static GameObject * GetBone();

private:
	AnimMan();
	static AnimMan *privGetInstance();
	Frame_Bucket *pFirstBone;
	GameObject *pFBone;

	bool privPlus = false;
	bool privMinus = false;
	bool privNext = false;
	bool privPlay = false;
	bool isPlaying = true;
	float fastForward = 0.1f;
};

#endif