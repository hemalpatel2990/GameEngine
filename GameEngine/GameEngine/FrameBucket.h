#ifndef FRAME_BUCKET
#define FRAME_BUCKET

#include "Timer.h"
#include "Vect.h"
#include "Quat.h"

struct BoneHeader
{
	int numBones;
	int fps;
};

struct myBone
{
	char bName[64];
	int parentIndex;
	void *pPtr;

	myBone()
	{
		memset(this, 0, sizeof(myBone));
	}
};

struct Bone : public Align16
{
	Vect  T;
	Quat  Q;
	Vect  S;
};

struct Frame_Bucket
{
	Frame_Bucket *nextBucket;
	Frame_Bucket *prevBucket;
	Time		  KeyTime;
	Bone		  *pBone;
};

struct AnimList
{
	Frame_Bucket *bucketList;
	int keyFrames;
	AnimList *next;
};

struct AnimationHeader
{
	int numAnims;
};


#endif