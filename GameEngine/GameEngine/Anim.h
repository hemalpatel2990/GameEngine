#ifndef ANIM_H
#define ANIM_H

#include "Timer.h"
#include "Vect.h"
#include "Quat.h"
#include "GameObject.h"


#define NUM_BONES 20

extern int numBones;
void SetAnimationHierarchy();
void SetAnimationData();
//void SetAnimationPose(GameObject *node);
void SetAnimationPose(GameObject *, Time);
void SetAnimationNew();

void ProcessAnimation(Time tCurr);
void FindMaxTime(Time &tMax);
#endif