#include "AnimMan.h"
#include "FrameBucket.h"

extern AnimList *anim;

void AnimMan::Update()
{
	Keyboard *key = InputMan::GetKeyboard();
	AnimMan *pMan = AnimMan::privGetInstance();

	static Time tCurrent(TIME_ZERO);

	// Animate ME
	ProcessAnimation(tCurrent);
	SetAnimationPose(AnimMan::GetBone(), tCurrent);


	// Faster
	bool plusCurr = key->GetKeyState(AZUL_KEY::KEY_P);
	if (plusCurr == true && pMan->privPlus == false)
	{
		pMan->fastForward += 0.1f;
	}
	pMan->privPlus = plusCurr;

	// Slow to Reverse
	bool minusCurr = key->GetKeyState(AZUL_KEY::KEY_O);
	if (minusCurr == true && pMan->privMinus == false)
	{
		pMan->fastForward -= 0.1f;
	}
	pMan->privMinus = minusCurr;

	// Next Animation
	bool NextCurr = key->GetKeyState(AZUL_KEY::KEY_N);
	if (NextCurr == true && pMan->privNext == false)
	{
		if (pMan->GetHead() == anim[0].bucketList)
			pMan->SetHead(anim[1].bucketList);
		else
			pMan->SetHead(anim[0].bucketList);
	}
	pMan->privNext = NextCurr;

	// Play/Pause
	bool playCurr = key->GetKeyState(AZUL_KEY::KEY_SPACE);
	if (playCurr == true && pMan->privPlay == false)
	{
		pMan->isPlaying = !pMan->isPlaying;
	}
	pMan->privPlay = playCurr;



	if (pMan->isPlaying)
	{
		Time deltaTime = pMan->fastForward * Time(TIME_NTSC_30_FRAME);
		Time maxTime;

		tCurrent += deltaTime;

		FindMaxTime(maxTime);

		// protection for time values for looping
		if (tCurrent < Time(TIME_ZERO))
		{
			tCurrent = maxTime;
		}
		else if (tCurrent > maxTime)
		{
			tCurrent = Time(TIME_ZERO);
		}
		else
		{
			// do nothing
		}
	}
}

void AnimMan::SetHead(Frame_Bucket *inObj)
{
	AnimMan *pMan = AnimMan::privGetInstance();
	pMan->pFirstBone = inObj;
}

Frame_Bucket * AnimMan::GetHead()
{
	AnimMan *pMan = AnimMan::privGetInstance();
	return pMan->pFirstBone;
}

void AnimMan::SetBone(GameObject *inObj)
{
	AnimMan *pMan = AnimMan::privGetInstance();
	pMan->pFBone = inObj;
}

GameObject * AnimMan::GetBone()
{
	AnimMan *pMan = AnimMan::privGetInstance();
	return pMan->pFBone;
}

AnimMan::AnimMan()
{

}

AnimMan * AnimMan::privGetInstance()
{
	static AnimMan privInstance;
	return &privInstance;
}
