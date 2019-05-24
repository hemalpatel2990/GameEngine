#include "Anim.h"
#include "Constants.h"
#include "VectApp.h"
#include "QuatApp.h"
#include "FrameBucket.h"
#include "Trace.h"
#include "AnimMan.h"

Frame_Bucket *pHead2;

void FindMaxTime(Time &tMax)
{
	pHead2 = AnimMan::GetHead();

	Frame_Bucket *pTmp = pHead2->nextBucket;

	while (pTmp->nextBucket != 0)
	{
		pTmp = pTmp->nextBucket;
	}

	tMax = pTmp->KeyTime;
}

void ProcessAnimation(Time tCurr)
{
	//Trace::out("time:%f\n", tCurr / Time(TIME_ONE_SECOND));
	pHead2 = AnimMan::GetHead();

	Frame_Bucket *pTmp = pHead2->nextBucket;

	// Get the result bone array
	// Remember the firs Frame is the result
	Bone *bResult = pHead2->pBone;

	// First one is the result, skip it
	pTmp = pHead2->nextBucket;

	// Find which key frames
	while (tCurr >= pTmp->KeyTime  && pTmp->nextBucket != 0)
	{
		pTmp = pTmp->nextBucket;
	}

	// pTmp is the "B" key frame
	// pTmp->prev is the "A" key frame
	Frame_Bucket *pA = pTmp->prevBucket;
	Frame_Bucket *pB = pTmp;

	// find the "S" of the time
	float tS = (tCurr - pA->KeyTime) / (pB->KeyTime - pA->KeyTime);

	// interpolate to 
	Bone *bA = pA->pBone;
	Bone *bB = pB->pBone;

	// Interpolate to tS time, for all bones
	for (int i = 0; i < numBones; i++)
	{
		// interpolate ahoy!
		VectApp::Lerp(bResult->T, bA->T, bB->T, tS);
		QuatApp::Slerp(bResult->Q, bA->Q, bB->Q, tS);
		VectApp::Lerp(bResult->S, bA->S, bB->S, tS);

		// advance the pointer
		bA++;
		bB++;
		bResult++;
	}

}

