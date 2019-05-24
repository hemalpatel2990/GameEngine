#include <assert.h>

#include "PyramidModel.h"
#include "GraphicsObject_TextureLight.h"
#include "GameObjectMan.h"
#include "Anim.h"
#include "FrameBucket.h"
#include "PCSTreeForwardIterator.h"
#include "File.h"
#include "AnimMan.h"

#define BONE_WIDTH 3.0f
void setBonePose(GameObject *node);

Frame_Bucket *pHead = 0;

AnimList *anim;

GameObject *pFirstBone;

PCSNode *root = 0;
int numBones;

void SetAnimationHierarchy()
{
	FileError ferror;
	FileHandle fh;

	ferror = File::open(fh, "human.anim", FileMode::READ);
	assert(ferror == FileError::SUCCESS);

	BoneHeader bHeader;

	ferror = File::seek(fh, FileSeek::BEGIN, 0);
	assert(ferror == FileError::SUCCESS);

	ferror = File::read(fh, &bHeader, sizeof(BoneHeader));
	assert(ferror == FileError::SUCCESS);

	myBone *boneList = new myBone[bHeader.numBones];

	ferror = File::read(fh, boneList, sizeof(myBone) * bHeader.numBones);
	assert(ferror == FileError::SUCCESS);

	AnimationHeader aheader;

	ferror = File::read(fh, &aheader, sizeof(AnimationHeader));
	assert(ferror == FileError::SUCCESS);

	AnimList *animList = new AnimList[aheader.numAnims];

	for (int i = 0; i < aheader.numAnims; ++i)
	{
		ferror = File::read(fh, &animList[i], sizeof(AnimList));
		assert(ferror == FileError::SUCCESS);

		animList[i].bucketList = new Frame_Bucket[animList[i].keyFrames];

		animList[i].next = 0;
		if (i == 0)
		{
			pHead = animList[i].bucketList;
			AnimMan::SetHead(pHead);
		}
		if (i > 0)
		{
			animList[i - 1].next = &animList[i];
		}

		for (int j = 0; j < animList[i].keyFrames; ++j)
		{
			ferror = File::read(fh, &animList[i].bucketList[j], sizeof(Frame_Bucket));
			assert(ferror == FileError::SUCCESS);

			animList[i].bucketList[j].KeyTime = j * Time(TIME_NTSC_30_FRAME);
			animList[i].bucketList[j].pBone = new Bone[bHeader.numBones - 1];

			animList[i].bucketList[j].nextBucket = 0;
			animList[i].bucketList[j].prevBucket = 0;

			if (j > 0)
			{
				animList[i].bucketList[j].prevBucket = &animList[i].bucketList[j - 1];
				animList[i].bucketList[j - 1].nextBucket = &animList[i].bucketList[j];
			}


			for (int k = 0; k < bHeader.numBones - 1; ++k)
			{
				/*if (k == 0)
				{
					ferror = File::seek(fh, FileSeek::CURRENT, sizeof(Bone));
					assert(ferror == FileError::SUCCESS);
				}*/
				
				{
					ferror = File::read(fh, &animList[i].bucketList[j].pBone[k], sizeof(Bone));
					assert(ferror == FileError::SUCCESS);
				}
			}
		}

	}

	File::close(fh);

	anim = new AnimList[2];
	numBones = bHeader.numBones - 1;

	// Load the model
	PyramidModel *pPyramidModel = new PyramidModel("pyramidModel.azul");
	assert(pPyramidModel);

	// Create/Load Shader 
	ShaderObject *pShaderObject_textureLight = new ShaderObject(ShaderName::TEXTURE_POINT_LIGHT, "texturePointLightDiff");
	assert(pShaderObject_textureLight);

	// GraphicsObject for a specific instance
	GraphicsObject_TextureLight *pGraphics_TextureLight;

	// Create GameObject
	Vect color;
	Vect pos(1, 1, 1);

	PCSTree *pTree = GameObjectMan::GetPCSTree();
	root = pTree->getRoot();

	for (int i = 1; i < bHeader.numBones; i++)
	{
		pGraphics_TextureLight = new GraphicsObject_TextureLight(pPyramidModel, pShaderObject_textureLight, TextureName::DUCKWEED, color, pos);
		GameObject *pBip01 = new GameObject(pGraphics_TextureLight);

		pBip01->setIndex(i - 1);


		boneList[i].pPtr = pBip01;
		int parentIndex = boneList[i].parentIndex;

		pBip01->setName(boneList[i].bName);

		if (i > 1)
			GameObjectMan::Add(pBip01, (GameObject *)boneList[parentIndex].pPtr);
		else
			GameObjectMan::Add(pBip01, GameObjectMan::GetRoot());
	}

	pFirstBone = (GameObject *)boneList[1].pPtr;

	AnimMan::SetBone(pFirstBone);

	pTree->printTree();
}


void SetAnimationPose(GameObject *pRoot, Time tCurr)
{
	// First thing, get the first frame of animation
	ProcessAnimation(tCurr);

	// Calls update on every object
	GameObjectMan::Update(tCurr);

	PCSTreeForwardIterator pIter(pRoot);
	PCSNode *pNode = pIter.First();
	GameObject *pGameObj = 0;

	// walks the anim node does the pose for everything that
	while (pNode != 0)
	{
		assert(pNode);
		// Update the game object
		pGameObj = (GameObject *)pNode;

		setBonePose(pGameObj);

		pNode = pIter.Next();
	}
}

void setBonePose(GameObject *node)
{
	// Now get the world matrices
	// getting the parent from current node
	GameObject *childNode = (GameObject *)node;
	GameObject *parentNode = (GameObject *)node->getParent();

	if (parentNode == root)
		return;

	if (parentNode != 0 && childNode != 0)
	{
		// starting point
		Vect start(0.0f, 0.0f, 0.0f);

		//  At this point, Find the two bones initial positions in world space
		//  Now get the length and directions

		Vect ptA = start * *parentNode->getWorld();
		Vect ptB = start * *childNode->getWorld();

		// direction between the anchor points of the respective bones
		Vect dir = (ptB - ptA);

		// length of the bone 0
		float mag = dir.mag();

		Matrix S(SCALE, BONE_WIDTH, BONE_WIDTH, mag);
		Quat Q(ROT_ORIENT, dir.getNorm(), Vect(0.0f, 1.0f, 0.0f));
		Matrix T(TRANS, ptA);

		Matrix BoneOrient = S * Q * T;

		childNode->setBoneOrientation(BoneOrient);
	}
}

void SetAnimationNew()
{
	anim[0].bucketList = new Frame_Bucket();
	anim[0].bucketList->prevBucket = 0;
	anim[0].bucketList->nextBucket = 0;
	anim[0].bucketList->KeyTime = Time(TIME_ZERO);
	anim[0].bucketList->pBone = new Bone[NUM_BONES];

	Frame_Bucket *pTmp = new Frame_Bucket();
	pTmp->prevBucket = anim[1].bucketList;
	pTmp->nextBucket = 0;
	pTmp->KeyTime = 0 * Time(TIME_NTSC_30_FRAME);
	pTmp->pBone = new Bone[NUM_BONES];
	anim[0].bucketList->nextBucket = pTmp;

	pHead = anim[1].bucketList;
	AnimMan::SetHead(pHead);

	//     Node Name: RootNode
	//     Node Name: Bip01
	pTmp->pBone[0].T = Vect(0.970236f, -2.711161f, 1.899864f);
	pTmp->pBone[0].Q = Quat(ROT_XYZ, 1.624573f, -1.264373f, 3.121366f);
	pTmp->pBone[0].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Pelvis
	pTmp->pBone[1].T = Vect(0.0f, 0.0f, 0.0f);
	pTmp->pBone[1].Q = Quat(ROT_XYZ, 0.0f, 0.0f, 0.0f);
	pTmp->pBone[1].S = Vect(1.0f, 1.0f, 1.0f);

	//     Node Name: Bip01 Spine
	pTmp->pBone[2].T = Vect(6.016934f, -0.014115f, -0.000028f);
	pTmp->pBone[2].Q = Quat(ROT_XYZ, -0.047765f, 0.020883f, 0.010792f);
	pTmp->pBone[2].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Thigh
	pTmp->pBone[3].T = Vect(-6.300546f, -0.567644f, 13.531080f);
	pTmp->pBone[3].Q = Quat(ROT_XYZ, 3.060995f, 0.095672f, -3.015697f);
	pTmp->pBone[3].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Calf
	pTmp->pBone[4].T = Vect(16.961559f, -0.000002f, -0.000004f);
	pTmp->pBone[4].Q = Quat(ROT_XYZ, -0.348718f, -0.021303f, -0.650505f);
	pTmp->pBone[4].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Foot
	pTmp->pBone[5].T = Vect(13.263550f, 0.000003f, 0.000002f);
	pTmp->pBone[5].Q = Quat(ROT_XYZ, -0.103780f, -0.201808f, 0.767066f);
	pTmp->pBone[5].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Thigh
	pTmp->pBone[6].T = Vect(-5.729567f, 0.737561f, -13.774520f);
	pTmp->pBone[6].Q = Quat(ROT_XYZ, 3.118021f, 0.054987f, 2.213239f);
	pTmp->pBone[6].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Calf
	pTmp->pBone[7].T = Vect(16.961571f, 0.000003f, -0.000003f);
	pTmp->pBone[7].Q = Quat(ROT_XYZ, -0.023962f, 0.004592f, -0.865707f);
	pTmp->pBone[7].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Foot
	pTmp->pBone[8].T = Vect(13.263530f, -0.000000f, 0.000002f);
	pTmp->pBone[8].Q = Quat(ROT_XYZ, -0.001414f, 0.038321f, 0.268111f);
	pTmp->pBone[8].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Spine1
	pTmp->pBone[9].T = Vect(17.730431f, -0.017629f, 0.000002f);
	pTmp->pBone[9].Q = Quat(ROT_XYZ, -0.095342f, 0.042221f, 0.018989f);
	pTmp->pBone[9].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Neck
	pTmp->pBone[10].T = Vect(22.140881f, -0.008821f, 0.000003f);
	pTmp->pBone[10].Q = Quat(ROT_XYZ, 0.204811f, 0.010239f, -0.060075f);
	pTmp->pBone[10].S = Vect(1.000000f, 1.000000f, 1.000001f);

	//     Node Name: Bip01 L Clavicle
	pTmp->pBone[11].T = Vect(-0.056449f, 1.119313f, 5.345238f);
	pTmp->pBone[11].Q = Quat(ROT_XYZ, 1.012052f, -1.036376f, 2.183302f);
	pTmp->pBone[11].S = Vect(0.999999f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L UpperArm
	pTmp->pBone[12].T = Vect(13.067860f, 0.000004f, -0.000004f);
	pTmp->pBone[12].Q = Quat(ROT_XYZ, -0.141163f, 0.660794f, 0.057318f);
	pTmp->pBone[12].S = Vect(1.000000f, 1.000001f, 1.000001f);

	//     Node Name: Bip01 L Forearm
	pTmp->pBone[13].T = Vect(15.099380f, -0.012400f, 0.002757f);
	pTmp->pBone[13].Q = Quat(ROT_XYZ, 0.645437f, 0.044597f, -0.613745f);
	pTmp->pBone[13].S = Vect(1.000000f, 1.000001f, 1.000001f);

	//     Node Name: Bip01 L Hand
	pTmp->pBone[14].T = Vect(15.099410f, 0.000082f, 0.000072f);
	pTmp->pBone[14].Q = Quat(ROT_XYZ, -1.583676f, 0.166629f, -0.123964f);
	pTmp->pBone[14].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Clavicle
	pTmp->pBone[15].T = Vect(0.055393f, -1.102072f, -5.348833f);
	pTmp->pBone[15].Q = Quat(ROT_XYZ, -0.402011f, 1.128657f, 2.673232f);
	pTmp->pBone[15].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R UpperArm
	pTmp->pBone[16].T = Vect(13.067840f, -0.000001f, 0.000001f);
	pTmp->pBone[16].Q = Quat(ROT_XYZ, -0.103288f, -0.809167f, 0.449669f);
	pTmp->pBone[16].S = Vect(1.000000f, 1.000000f, 1.000001f);

	//     Node Name: Bip01 R Forearm
	pTmp->pBone[17].T = Vect(15.099410f, -0.000577f, -0.000088f);
	pTmp->pBone[17].Q = Quat(ROT_XYZ, -0.818463f, -0.029294f, -0.231554f);
	pTmp->pBone[17].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Hand
	pTmp->pBone[18].T = Vect(15.099400f, 0.000180f, -0.000104f);
	pTmp->pBone[18].Q = Quat(ROT_XYZ, 1.566849f, -0.070789f, -0.100040f);
	pTmp->pBone[18].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Head
	pTmp->pBone[19].T = Vect(11.079190f, 0.000001f, 0.000001f);
	pTmp->pBone[19].Q = Quat(ROT_XYZ, 0.191348f, -0.036722f, 0.254348f);
	pTmp->pBone[19].S = Vect(1.000000f, 1.000000f, 1.000000f);



	//------- Key Time: 1  ms: 33 ------------------ 

	Frame_Bucket *pTmp1 = new Frame_Bucket();
	pTmp1->prevBucket = pTmp;
	pTmp1->nextBucket = 0;
	pTmp1->KeyTime = 1 * Time(TIME_NTSC_30_FRAME);
	pTmp1->pBone = new Bone[NUM_BONES];
	pTmp->nextBucket = pTmp1;

	//     Node Name: RootNode
	//     Node Name: Bip01
	pTmp1->pBone[0].T = Vect(1.849216f, -3.682207f, 1.335635f);
	pTmp1->pBone[0].Q = Quat(ROT_XYZ, 1.624670f, -1.290109f, 3.130007f);
	pTmp1->pBone[0].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Pelvis
	pTmp1->pBone[1].T = Vect(0.0f, 0.0f, 0.0f);
	pTmp1->pBone[1].Q = Quat(ROT_XYZ, 0.0f, 0.0f, 0.0f);
	pTmp1->pBone[1].S = Vect(1.0f, 1.0f, 1.0f);

	//     Node Name: Bip01 Spine
	pTmp1->pBone[2].T = Vect(6.016934f, -0.014115f, -0.000028f);
	pTmp1->pBone[2].Q = Quat(ROT_XYZ, -0.053292f, 0.025800f, 0.010692f);
	pTmp1->pBone[2].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Thigh
	pTmp1->pBone[3].T = Vect(-6.367048f, -0.641366f, 13.496590f);
	pTmp1->pBone[3].Q = Quat(ROT_XYZ, 3.068367f, 0.120414f, -2.965829f);
	pTmp1->pBone[3].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Calf
	pTmp1->pBone[4].T = Vect(16.961559f, -0.000002f, -0.000004f);
	pTmp1->pBone[4].Q = Quat(ROT_XYZ, -0.324466f, -0.019808f, -0.627606f);
	pTmp1->pBone[4].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Foot
	pTmp1->pBone[5].T = Vect(13.263550f, 0.000003f, 0.000002f);
	pTmp1->pBone[5].Q = Quat(ROT_XYZ, -0.102957f, -0.186260f, 0.802738f);
	pTmp1->pBone[5].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Thigh
	pTmp1->pBone[6].T = Vect(-5.661640f, 0.814576f, -13.798210f);
	pTmp1->pBone[6].Q = Quat(ROT_XYZ, 3.111218f, 0.039966f, 2.187612f);
	pTmp1->pBone[6].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Calf
	pTmp1->pBone[7].T = Vect(16.961571f, 0.000003f, -0.000003f);
	pTmp1->pBone[7].Q = Quat(ROT_XYZ, -0.030066f, 0.004231f, -0.864004f);
	pTmp1->pBone[7].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Foot
	pTmp1->pBone[8].T = Vect(13.263530f, -0.000000f, 0.000002f);
	pTmp1->pBone[8].Q = Quat(ROT_XYZ, -0.002249f, -0.042377f, 0.186320f);
	pTmp1->pBone[8].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Spine1
	pTmp1->pBone[9].T = Vect(17.730431f, -0.017629f, 0.000002f);
	pTmp1->pBone[9].Q = Quat(ROT_XYZ, -0.106372f, 0.052092f, 0.018413f);
	pTmp1->pBone[9].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Neck
	pTmp1->pBone[10].T = Vect(22.140881f, -0.008821f, 0.000003f);
	pTmp1->pBone[10].Q = Quat(ROT_XYZ, 0.204465f, 0.001706f, -0.073168f);
	pTmp1->pBone[10].S = Vect(1.000000f, 1.000000f, 1.000001f);

	//     Node Name: Bip01 L Clavicle
	pTmp1->pBone[11].T = Vect(-0.009947f, 1.117512f, 5.345905f);
	pTmp1->pBone[11].Q = Quat(ROT_XYZ, 1.011819f, -1.042900f, 2.197518f);
	pTmp1->pBone[11].S = Vect(0.999999f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L UpperArm
	pTmp1->pBone[12].T = Vect(13.067860f, 0.000004f, -0.000004f);
	pTmp1->pBone[12].Q = Quat(ROT_XYZ, -0.138620f, 0.663902f, 0.057334f);
	pTmp1->pBone[12].S = Vect(1.000000f, 1.000001f, 1.000001f);

	//     Node Name: Bip01 L Forearm
	pTmp1->pBone[13].T = Vect(15.099380f, -0.000623f, 0.000198f);
	pTmp1->pBone[13].Q = Quat(ROT_XYZ, 0.659133f, 0.048044f, -0.653231f);
	pTmp1->pBone[13].S = Vect(1.000000f, 1.000001f, 1.000001f);

	//     Node Name: Bip01 L Hand
	pTmp1->pBone[14].T = Vect(15.099410f, 0.000082f, 0.000072f);
	pTmp1->pBone[14].Q = Quat(ROT_XYZ, -1.584586f, 0.175301f, -0.121555f);
	pTmp1->pBone[14].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Clavicle
	pTmp1->pBone[15].T = Vect(0.008690f, -1.100281f, -5.349482f);
	pTmp1->pBone[15].Q = Quat(ROT_XYZ, -0.393472f, 1.139374f, 2.700089f);
	pTmp1->pBone[15].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R UpperArm
	pTmp1->pBone[16].T = Vect(13.067840f, -0.000001f, 0.000001f);
	pTmp1->pBone[16].Q = Quat(ROT_XYZ, -0.100755f, -0.812855f, 0.449885f);
	pTmp1->pBone[16].S = Vect(1.000000f, 1.000000f, 1.000001f);

	//     Node Name: Bip01 R Forearm
	pTmp1->pBone[17].T = Vect(15.099410f, 0.000176f, 0.000153f);
	pTmp1->pBone[17].Q = Quat(ROT_XYZ, -0.813545f, -0.028175f, -0.211810f);
	pTmp1->pBone[17].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Hand
	pTmp1->pBone[18].T = Vect(15.099400f, 0.000180f, -0.000104f);
	pTmp1->pBone[18].Q = Quat(ROT_XYZ, 1.567071f, -0.073978f, -0.097455f);
	pTmp1->pBone[18].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Head
	pTmp1->pBone[19].T = Vect(11.079190f, 0.000001f, 0.000001f);
	pTmp1->pBone[19].Q = Quat(ROT_XYZ, 0.188528f, -0.043045f, 0.251530f);
	pTmp1->pBone[19].S = Vect(1.000000f, 1.000000f, 1.000000f);



	//------- Key Time: 2  ms: 66 ------------------ 

	Frame_Bucket *pTmp2 = new Frame_Bucket();
	pTmp2->prevBucket = pTmp1;
	pTmp2->nextBucket = 0;
	pTmp2->KeyTime = 2 * Time(TIME_NTSC_30_FRAME);
	pTmp2->pBone = new Bone[NUM_BONES];
	pTmp1->nextBucket = pTmp2;

	//     Node Name: RootNode
	//     Node Name: Bip01
	pTmp2->pBone[0].T = Vect(2.559613f, -4.820625f, 0.807654f);
	pTmp2->pBone[0].Q = Quat(ROT_XYZ, 1.651774f, -1.328626f, 3.109358f);
	pTmp2->pBone[0].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Pelvis
	pTmp2->pBone[1].T = Vect(0.0f, 0.0f, 0.0f);
	pTmp2->pBone[1].Q = Quat(ROT_XYZ, 0.0f, 0.0f, 0.0f);
	pTmp2->pBone[1].S = Vect(1.0f, 1.0f, 1.0f);

	//     Node Name: Bip01 Spine
	pTmp2->pBone[2].T = Vect(6.016934f, -0.014115f, -0.000028f);
	pTmp2->pBone[2].Q = Quat(ROT_XYZ, -0.060823f, 0.030485f, 0.012140f);
	pTmp2->pBone[2].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Thigh
	pTmp2->pBone[3].T = Vect(-6.430149f, -0.732482f, 13.461990f);
	pTmp2->pBone[3].Q = Quat(ROT_XYZ, 3.070328f, 0.127781f, -2.917873f);
	pTmp2->pBone[3].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Calf
	pTmp2->pBone[4].T = Vect(16.961559f, -0.000002f, -0.000004f);
	pTmp2->pBone[4].Q = Quat(ROT_XYZ, -0.283292f, -0.019720f, -0.624020f);
	pTmp2->pBone[4].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Foot
	pTmp2->pBone[5].T = Vect(13.263550f, 0.000003f, 0.000002f);
	pTmp2->pBone[5].Q = Quat(ROT_XYZ, -0.106415f, -0.173676f, 0.847185f);
	pTmp2->pBone[5].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Thigh
	pTmp2->pBone[6].T = Vect(-5.596685f, 0.928764f, -13.817480f);
	pTmp2->pBone[6].Q = Quat(ROT_XYZ, 3.100217f, 0.031348f, 2.203141f);
	pTmp2->pBone[6].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Calf
	pTmp2->pBone[7].T = Vect(16.961571f, 0.000003f, -0.000003f);
	pTmp2->pBone[7].Q = Quat(ROT_XYZ, -0.087422f, 0.001052f, -0.831321f);
	pTmp2->pBone[7].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Foot
	pTmp2->pBone[8].T = Vect(13.263530f, -0.000000f, 0.000002f);
	pTmp2->pBone[8].Q = Quat(ROT_XYZ, 0.006155f, -0.135256f, 0.087510f);
	pTmp2->pBone[8].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Spine1
	pTmp2->pBone[9].T = Vect(17.730431f, -0.017629f, 0.000002f);
	pTmp2->pBone[9].Q = Quat(ROT_XYZ, -0.121376f, 0.061604f, 0.020826f);
	pTmp2->pBone[9].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Neck
	pTmp2->pBone[10].T = Vect(22.140881f, -0.008821f, 0.000003f);
	pTmp2->pBone[10].Q = Quat(ROT_XYZ, 0.202676f, -0.010824f, -0.084292f);
	pTmp2->pBone[10].S = Vect(1.000000f, 1.000000f, 1.000001f);

	//     Node Name: Bip01 L Clavicle
	pTmp2->pBone[11].T = Vect(0.058379f, 1.107880f, 5.347602f);
	pTmp2->pBone[11].Q = Quat(ROT_XYZ, 1.018151f, -1.051632f, 2.202647f);
	pTmp2->pBone[11].S = Vect(0.999999f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L UpperArm
	pTmp2->pBone[12].T = Vect(13.067860f, 0.000004f, -0.000004f);
	pTmp2->pBone[12].Q = Quat(ROT_XYZ, -0.131469f, 0.673224f, 0.058001f);
	pTmp2->pBone[12].S = Vect(1.000000f, 1.000001f, 1.000001f);

	//     Node Name: Bip01 L Forearm
	pTmp2->pBone[13].T = Vect(15.099380f, -0.000638f, 0.000191f);
	pTmp2->pBone[13].Q = Quat(ROT_XYZ, 0.672984f, 0.051312f, -0.687508f);
	pTmp2->pBone[13].S = Vect(1.000000f, 1.000001f, 1.000001f);

	//     Node Name: Bip01 L Hand
	pTmp2->pBone[14].T = Vect(15.099410f, 0.000082f, 0.000072f);
	pTmp2->pBone[14].Q = Quat(ROT_XYZ, -1.586217f, 0.188735f, -0.122430f);
	pTmp2->pBone[14].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Clavicle
	pTmp2->pBone[15].T = Vect(-0.059850f, -1.090654f, -5.351128f);
	pTmp2->pBone[15].Q = Quat(ROT_XYZ, -0.388875f, 1.144479f, 2.719413f);
	pTmp2->pBone[15].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R UpperArm
	pTmp2->pBone[16].T = Vect(13.067840f, -0.000001f, 0.000001f);
	pTmp2->pBone[16].Q = Quat(ROT_XYZ, -0.095281f, -0.821228f, 0.445243f);
	pTmp2->pBone[16].S = Vect(1.000000f, 1.000000f, 1.000001f);

	//     Node Name: Bip01 R Forearm
	pTmp2->pBone[17].T = Vect(15.099410f, -0.000636f, -0.000110f);
	pTmp2->pBone[17].Q = Quat(ROT_XYZ, -0.807391f, -0.027032f, -0.183771f);
	pTmp2->pBone[17].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Hand
	pTmp2->pBone[18].T = Vect(15.099400f, 0.000180f, -0.000104f);
	pTmp2->pBone[18].Q = Quat(ROT_XYZ, 1.565816f, -0.064428f, -0.093637f);
	pTmp2->pBone[18].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Head
	pTmp2->pBone[19].T = Vect(11.079190f, 0.000001f, 0.000001f);
	pTmp2->pBone[19].Q = Quat(ROT_XYZ, 0.181552f, -0.046188f, 0.249624f);
	pTmp2->pBone[19].S = Vect(1.000000f, 1.000000f, 1.000000f);



	//------- Key Time: 3  ms: 100 ------------------ 

	Frame_Bucket *pTmp3 = new Frame_Bucket();
	pTmp3->prevBucket = pTmp2;
	pTmp3->nextBucket = 0;
	pTmp3->KeyTime = 3 * Time(TIME_NTSC_30_FRAME);
	pTmp3->pBone = new Bone[NUM_BONES];
	pTmp2->nextBucket = pTmp3;

	//     Node Name: RootNode
	//     Node Name: Bip01
	pTmp3->pBone[0].T = Vect(3.080650f, -5.935921f, 0.180111f);
	pTmp3->pBone[0].Q = Quat(ROT_XYZ, 1.702839f, -1.378956f, 3.065481f);
	pTmp3->pBone[0].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Pelvis
	pTmp3->pBone[1].T = Vect(0.0f, 0.0f, 0.0f);
	pTmp3->pBone[1].Q = Quat(ROT_XYZ, 0.0f, 0.0f, 0.0f);
	pTmp3->pBone[1].S = Vect(1.0f, 1.0f, 1.0f);

	//     Node Name: Bip01 Spine
	pTmp3->pBone[2].T = Vect(6.016934f, -0.014115f, -0.000028f);
	pTmp3->pBone[2].Q = Quat(ROT_XYZ, -0.068606f, 0.033833f, 0.012427f);
	pTmp3->pBone[2].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Thigh
	pTmp3->pBone[3].T = Vect(-6.475285f, -0.834019f, 13.434450f);
	pTmp3->pBone[3].Q = Quat(ROT_XYZ, 3.065515f, 0.107671f, -2.885550f);
	pTmp3->pBone[3].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Calf
	pTmp3->pBone[4].T = Vect(16.961559f, -0.000002f, -0.000004f);
	pTmp3->pBone[4].Q = Quat(ROT_XYZ, -0.200336f, -0.020123f, -0.659933f);
	pTmp3->pBone[4].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Foot
	pTmp3->pBone[5].T = Vect(13.263550f, 0.000003f, 0.000002f);
	pTmp3->pBone[5].Q = Quat(ROT_XYZ, -0.107715f, -0.156771f, 0.873369f);
	pTmp3->pBone[5].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Thigh
	pTmp3->pBone[6].T = Vect(-5.550339f, 1.039276f, -13.828350f);
	pTmp3->pBone[6].Q = Quat(ROT_XYZ, 3.073914f, 0.062401f, 2.204732f);
	pTmp3->pBone[6].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Calf
	pTmp3->pBone[7].T = Vect(16.961571f, 0.000003f, -0.000003f);
	pTmp3->pBone[7].Q = Quat(ROT_XYZ, -0.149287f, -0.004462f, -0.839591f);
	pTmp3->pBone[7].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Foot
	pTmp3->pBone[8].T = Vect(13.263530f, -0.000000f, 0.000002f);
	pTmp3->pBone[8].Q = Quat(ROT_XYZ, 0.011289f, 0.017234f, -0.026588f);
	pTmp3->pBone[8].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Spine1
	pTmp3->pBone[9].T = Vect(17.730431f, -0.017629f, 0.000002f);
	pTmp3->pBone[9].Q = Quat(ROT_XYZ, -0.136926f, 0.068386f, 0.020930f);
	pTmp3->pBone[9].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Neck
	pTmp3->pBone[10].T = Vect(22.140881f, -0.008821f, 0.000003f);
	pTmp3->pBone[10].Q = Quat(ROT_XYZ, 0.200706f, -0.015178f, -0.097583f);
	pTmp3->pBone[10].S = Vect(1.000000f, 1.000000f, 1.000001f);

	//     Node Name: Bip01 L Clavicle
	pTmp3->pBone[11].T = Vect(0.082038f, 1.097273f, 5.349479f);
	pTmp3->pBone[11].Q = Quat(ROT_XYZ, 1.013787f, -1.059423f, 2.220898f);
	pTmp3->pBone[11].S = Vect(0.999999f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L UpperArm
	pTmp3->pBone[12].T = Vect(13.067860f, 0.000004f, -0.000004f);
	pTmp3->pBone[12].Q = Quat(ROT_XYZ, -0.124427f, 0.695644f, 0.047606f);
	pTmp3->pBone[12].S = Vect(1.000000f, 1.000001f, 1.000001f);

	//     Node Name: Bip01 L Forearm
	pTmp3->pBone[13].T = Vect(15.099380f, -0.002248f, 0.000674f);
	pTmp3->pBone[13].Q = Quat(ROT_XYZ, 0.686527f, 0.053035f, -0.696319f);
	pTmp3->pBone[13].S = Vect(1.000000f, 1.000001f, 1.000001f);

	//     Node Name: Bip01 L Hand
	pTmp3->pBone[14].T = Vect(15.099410f, 0.000082f, 0.000072f);
	pTmp3->pBone[14].Q = Quat(ROT_XYZ, -1.588506f, 0.197356f, -0.141726f);
	pTmp3->pBone[14].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Clavicle
	pTmp3->pBone[15].T = Vect(-0.083741f, -1.080058f, -5.352953f);
	pTmp3->pBone[15].Q = Quat(ROT_XYZ, -0.363947f, 1.149070f, 2.761519f);
	pTmp3->pBone[15].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R UpperArm
	pTmp3->pBone[16].T = Vect(13.067840f, -0.000001f, 0.000001f);
	pTmp3->pBone[16].Q = Quat(ROT_XYZ, -0.077571f, -0.824910f, 0.421713f);
	pTmp3->pBone[16].S = Vect(1.000000f, 1.000000f, 1.000001f);

	//     Node Name: Bip01 R Forearm
	pTmp3->pBone[17].T = Vect(15.099410f, -0.000194f, 0.000024f);
	pTmp3->pBone[17].Q = Quat(ROT_XYZ, -0.807806f, -0.025578f, -0.135432f);
	pTmp3->pBone[17].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Hand
	pTmp3->pBone[18].T = Vect(15.099400f, 0.000180f, -0.000104f);
	pTmp3->pBone[18].Q = Quat(ROT_XYZ, 1.566043f, -0.064301f, -0.102912f);
	pTmp3->pBone[18].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Head
	pTmp3->pBone[19].T = Vect(11.079190f, 0.000001f, 0.000001f);
	pTmp3->pBone[19].Q = Quat(ROT_XYZ, 0.172725f, -0.056284f, 0.261669f);
	pTmp3->pBone[19].S = Vect(1.000000f, 1.000000f, 1.000000f);



	//------- Key Time: 4  ms: 133 ------------------ 

	Frame_Bucket *pTmp4 = new Frame_Bucket();
	pTmp4->prevBucket = pTmp3;
	pTmp4->nextBucket = 0;
	pTmp4->KeyTime = 4 * Time(TIME_NTSC_30_FRAME);
	pTmp4->pBone = new Bone[NUM_BONES];
	pTmp3->nextBucket = pTmp4;

	//     Node Name: RootNode
	//     Node Name: Bip01
	pTmp4->pBone[0].T = Vect(3.521286f, -6.142822f, -0.184750f);
	pTmp4->pBone[0].Q = Quat(ROT_XYZ, 1.814112f, -1.428722f, 2.954906f);
	pTmp4->pBone[0].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Pelvis
	pTmp4->pBone[1].T = Vect(0.0f, 0.0f, 0.0f);
	pTmp4->pBone[1].Q = Quat(ROT_XYZ, 0.0f, 0.0f, 0.0f);
	pTmp4->pBone[1].S = Vect(1.0f, 1.0f, 1.0f);

	//     Node Name: Bip01 Spine
	pTmp4->pBone[2].T = Vect(6.016934f, -0.014115f, -0.000028f);
	pTmp4->pBone[2].Q = Quat(ROT_XYZ, -0.076338f, 0.042306f, 0.008912f);
	pTmp4->pBone[2].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Thigh
	pTmp4->pBone[3].T = Vect(-6.589337f, -0.954719f, 13.370780f);
	pTmp4->pBone[3].Q = Quat(ROT_XYZ, 3.045212f, 0.077102f, -2.896038f);
	pTmp4->pBone[3].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Calf
	pTmp4->pBone[4].T = Vect(16.961559f, -0.000002f, -0.000004f);
	pTmp4->pBone[4].Q = Quat(ROT_XYZ, -0.109989f, -0.022677f, -0.779769f);
	pTmp4->pBone[4].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Foot
	pTmp4->pBone[5].T = Vect(13.263550f, 0.000003f, 0.000002f);
	pTmp4->pBone[5].Q = Quat(ROT_XYZ, -0.095364f, -0.124625f, 0.846360f);
	pTmp4->pBone[5].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Thigh
	pTmp4->pBone[6].T = Vect(-5.432866f, 1.128621f, -13.867910f);
	pTmp4->pBone[6].Q = Quat(ROT_XYZ, 3.059966f, 0.070836f, 2.218847f);
	pTmp4->pBone[6].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Calf
	pTmp4->pBone[7].T = Vect(16.961571f, 0.000003f, -0.000003f);
	pTmp4->pBone[7].Q = Quat(ROT_XYZ, -0.168750f, -0.004615f, -0.897350f);
	pTmp4->pBone[7].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Foot
	pTmp4->pBone[8].T = Vect(13.263530f, -0.000000f, 0.000002f);
	pTmp4->pBone[8].Q = Quat(ROT_XYZ, 0.008212f, 0.015747f, -0.029608f);
	pTmp4->pBone[8].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Spine1
	pTmp4->pBone[9].T = Vect(17.730431f, -0.017629f, 0.000002f);
	pTmp4->pBone[9].Q = Quat(ROT_XYZ, -0.152528f, 0.085111f, 0.012994f);
	pTmp4->pBone[9].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Neck
	pTmp4->pBone[10].T = Vect(22.140881f, -0.008821f, 0.000003f);
	pTmp4->pBone[10].Q = Quat(ROT_XYZ, 0.211928f, -0.036995f, -0.094086f);
	pTmp4->pBone[10].S = Vect(1.000000f, 1.000000f, 1.000001f);

	//     Node Name: Bip01 L Clavicle
	pTmp4->pBone[11].T = Vect(0.201175f, 1.156585f, 5.333807f);
	pTmp4->pBone[11].Q = Quat(ROT_XYZ, 1.004775f, -1.054696f, 2.228744f);
	pTmp4->pBone[11].S = Vect(0.999999f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L UpperArm
	pTmp4->pBone[12].T = Vect(13.067860f, 0.000004f, -0.000004f);
	pTmp4->pBone[12].Q = Quat(ROT_XYZ, -0.116218f, 0.721306f, 0.032951f);
	pTmp4->pBone[12].S = Vect(1.000000f, 1.000001f, 1.000001f);

	//     Node Name: Bip01 L Forearm
	pTmp4->pBone[13].T = Vect(15.099380f, -0.000442f, 0.000085f);
	pTmp4->pBone[13].Q = Quat(ROT_XYZ, 0.724992f, 0.052341f, -0.653925f);
	pTmp4->pBone[13].S = Vect(1.000000f, 1.000001f, 1.000001f);

	//     Node Name: Bip01 L Hand
	pTmp4->pBone[14].T = Vect(15.099410f, 0.000082f, 0.000072f);
	pTmp4->pBone[14].Q = Quat(ROT_XYZ, -1.589645f, 0.194646f, -0.182686f);
	pTmp4->pBone[14].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Clavicle
	pTmp4->pBone[15].T = Vect(-0.202811f, -1.139398f, -5.337445f);
	pTmp4->pBone[15].Q = Quat(ROT_XYZ, -0.327258f, 1.142041f, 2.789321f);
	pTmp4->pBone[15].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R UpperArm
	pTmp4->pBone[16].T = Vect(13.067840f, -0.000001f, 0.000001f);
	pTmp4->pBone[16].Q = Quat(ROT_XYZ, -0.055350f, -0.822235f, 0.388331f);
	pTmp4->pBone[16].S = Vect(1.000000f, 1.000000f, 1.000001f);

	//     Node Name: Bip01 R Forearm
	pTmp4->pBone[17].T = Vect(15.099410f, -0.000708f, -0.000115f);
	pTmp4->pBone[17].Q = Quat(ROT_XYZ, -0.818688f, -0.024577f, -0.045774f);
	pTmp4->pBone[17].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Hand
	pTmp4->pBone[18].T = Vect(15.099400f, 0.000180f, -0.000104f);
	pTmp4->pBone[18].Q = Quat(ROT_XYZ, 1.566299f, -0.065462f, -0.115560f);
	pTmp4->pBone[18].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Head
	pTmp4->pBone[19].T = Vect(11.079190f, 0.000001f, 0.000001f);
	pTmp4->pBone[19].Q = Quat(ROT_XYZ, 0.162291f, -0.062710f, 0.276245f);
	pTmp4->pBone[19].S = Vect(1.000000f, 1.000000f, 1.000000f);



	//------- Key Time: 5  ms: 166 ------------------ 

	Frame_Bucket *pTmp5 = new Frame_Bucket();
	pTmp5->prevBucket = pTmp4;
	pTmp5->nextBucket = 0;
	pTmp5->KeyTime = 5 * Time(TIME_NTSC_30_FRAME);
	pTmp5->pBone = new Bone[NUM_BONES];
	pTmp4->nextBucket = pTmp5;

	//     Node Name: RootNode
	//     Node Name: Bip01
	pTmp5->pBone[0].T = Vect(3.580180f, -5.973682f, -0.343237f);
	pTmp5->pBone[0].Q = Quat(ROT_XYZ, 2.003191f, -1.455347f, 2.759081f);
	pTmp5->pBone[0].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Pelvis
	pTmp5->pBone[1].T = Vect(0.0f, 0.0f, 0.0f);
	pTmp5->pBone[1].Q = Quat(ROT_XYZ, 0.0f, 0.0f, 0.0f);
	pTmp5->pBone[1].S = Vect(1.0f, 1.0f, 1.0f);

	//     Node Name: Bip01 Spine
	pTmp5->pBone[2].T = Vect(6.016934f, -0.014115f, -0.000028f);
	pTmp5->pBone[2].Q = Quat(ROT_XYZ, -0.078955f, 0.047278f, 0.002584f);
	pTmp5->pBone[2].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Thigh
	pTmp5->pBone[3].T = Vect(-6.656324f, -1.025085f, 13.332400f);
	pTmp5->pBone[3].Q = Quat(ROT_XYZ, 3.027481f, 0.030159f, -2.903792f);
	pTmp5->pBone[3].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Calf
	pTmp5->pBone[4].T = Vect(16.961559f, -0.000002f, -0.000004f);
	pTmp5->pBone[4].Q = Quat(ROT_XYZ, -0.012259f, -0.026042f, -0.891846f);
	pTmp5->pBone[4].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Foot
	pTmp5->pBone[5].T = Vect(13.263550f, 0.000003f, 0.000002f);
	pTmp5->pBone[5].Q = Quat(ROT_XYZ, -0.082862f, -0.117104f, 0.756842f);
	pTmp5->pBone[5].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Thigh
	pTmp5->pBone[6].T = Vect(-5.364012f, 1.129087f, -13.894700f);
	pTmp5->pBone[6].Q = Quat(ROT_XYZ, 3.050321f, 0.065857f, 2.249065f);
	pTmp5->pBone[6].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Calf
	pTmp5->pBone[7].T = Vect(16.961571f, 0.000003f, -0.000003f);
	pTmp5->pBone[7].Q = Quat(ROT_XYZ, -0.176290f, -0.001879f, -0.975466f);
	pTmp5->pBone[7].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Foot
	pTmp5->pBone[8].T = Vect(13.263530f, -0.000000f, 0.000002f);
	pTmp5->pBone[8].Q = Quat(ROT_XYZ, 0.000621f, -0.058322f, 0.054393f);
	pTmp5->pBone[8].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Spine1
	pTmp5->pBone[9].T = Vect(17.730431f, -0.017629f, 0.000002f);
	pTmp5->pBone[9].Q = Quat(ROT_XYZ, -0.158084f, 0.094554f, -0.000168f);
	pTmp5->pBone[9].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Neck
	pTmp5->pBone[10].T = Vect(22.140881f, -0.008821f, 0.000003f);
	pTmp5->pBone[10].Q = Quat(ROT_XYZ, 0.218866f, -0.053245f, -0.077946f);
	pTmp5->pBone[10].S = Vect(1.000000f, 1.000000f, 1.000001f);

	//     Node Name: Bip01 L Clavicle
	pTmp5->pBone[11].T = Vect(0.289979f, 1.192711f, 5.321751f);
	pTmp5->pBone[11].Q = Quat(ROT_XYZ, 0.988013f, -1.058042f, 2.232020f);
	pTmp5->pBone[11].S = Vect(0.999999f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L UpperArm
	pTmp5->pBone[12].T = Vect(13.067860f, 0.000004f, -0.000004f);
	pTmp5->pBone[12].Q = Quat(ROT_XYZ, -0.093004f, 0.759274f, 0.048985f);
	pTmp5->pBone[12].S = Vect(1.000000f, 1.000001f, 1.000001f);

	//     Node Name: Bip01 L Forearm
	pTmp5->pBone[13].T = Vect(15.099380f, -0.000071f, -0.000089f);
	pTmp5->pBone[13].Q = Quat(ROT_XYZ, 0.762605f, 0.049853f, -0.607640f);
	pTmp5->pBone[13].S = Vect(1.000000f, 1.000001f, 1.000001f);

	//     Node Name: Bip01 L Hand
	pTmp5->pBone[14].T = Vect(15.099410f, 0.000082f, 0.000072f);
	pTmp5->pBone[14].Q = Quat(ROT_XYZ, -1.587345f, 0.182691f, -0.205773f);
	pTmp5->pBone[14].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Clavicle
	pTmp5->pBone[15].T = Vect(-0.291334f, -1.175523f, -5.325499f);
	pTmp5->pBone[15].Q = Quat(ROT_XYZ, -0.262909f, 1.138003f, 2.835746f);
	pTmp5->pBone[15].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R UpperArm
	pTmp5->pBone[16].T = Vect(13.067840f, -0.000001f, 0.000001f);
	pTmp5->pBone[16].Q = Quat(ROT_XYZ, -0.063001f, -0.816578f, 0.391013f);
	pTmp5->pBone[16].S = Vect(1.000000f, 1.000000f, 1.000001f);

	//     Node Name: Bip01 R Forearm
	pTmp5->pBone[17].T = Vect(15.099410f, -0.000011f, 0.000100f);
	pTmp5->pBone[17].Q = Quat(ROT_XYZ, -0.856490f, -0.023958f, -0.037067f);
	pTmp5->pBone[17].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Hand
	pTmp5->pBone[18].T = Vect(15.099400f, 0.000180f, -0.000104f);
	pTmp5->pBone[18].Q = Quat(ROT_XYZ, 1.568327f, -0.083352f, -0.111419f);
	pTmp5->pBone[18].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Head
	pTmp5->pBone[19].T = Vect(11.079190f, 0.000001f, 0.000001f);
	pTmp5->pBone[19].Q = Quat(ROT_XYZ, 0.150936f, -0.053558f, 0.289431f);
	pTmp5->pBone[19].S = Vect(1.000000f, 1.000000f, 1.000000f);



	//------- Key Time: 6  ms: 200 ------------------ 

	Frame_Bucket *pTmp6 = new Frame_Bucket();
	pTmp6->prevBucket = pTmp5;
	pTmp6->nextBucket = 0;
	pTmp6->KeyTime = 6 * Time(TIME_NTSC_30_FRAME);
	pTmp6->pBone = new Bone[NUM_BONES];
	pTmp5->nextBucket = pTmp6;

	//     Node Name: RootNode
	//     Node Name: Bip01
	pTmp6->pBone[0].T = Vect(3.451018f, -5.464783f, 0.018024f);
	pTmp6->pBone[0].Q = Quat(ROT_XYZ, 2.236743f, -1.456398f, 2.515219f);
	pTmp6->pBone[0].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Pelvis
	pTmp6->pBone[1].T = Vect(0.0f, 0.0f, 0.0f);
	pTmp6->pBone[1].Q = Quat(ROT_XYZ, 0.0f, 0.0f, 0.0f);
	pTmp6->pBone[1].S = Vect(1.0f, 1.0f, 1.0f);

	//     Node Name: Bip01 Spine
	pTmp6->pBone[2].T = Vect(6.016934f, -0.014115f, -0.000028f);
	pTmp6->pBone[2].Q = Quat(ROT_XYZ, -0.073432f, 0.049660f, 0.003894f);
	pTmp6->pBone[2].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Thigh
	pTmp6->pBone[3].T = Vect(-6.688056f, -0.942405f, 13.322590f);
	pTmp6->pBone[3].Q = Quat(ROT_XYZ, 3.008768f, -0.027241f, -2.918211f);
	pTmp6->pBone[3].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Calf
	pTmp6->pBone[4].T = Vect(16.961559f, -0.000002f, -0.000004f);
	pTmp6->pBone[4].Q = Quat(ROT_XYZ, 0.082271f, -0.030508f, -1.020630f);
	pTmp6->pBone[4].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Foot
	pTmp6->pBone[5].T = Vect(13.263550f, 0.000003f, 0.000002f);
	pTmp6->pBone[5].Q = Quat(ROT_XYZ, -0.072118f, -0.120664f, 0.647919f);
	pTmp6->pBone[5].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Thigh
	pTmp6->pBone[6].T = Vect(-5.330676f, 1.061123f, -13.912840f);
	pTmp6->pBone[6].Q = Quat(ROT_XYZ, 3.059350f, 0.060210f, 2.300501f);
	pTmp6->pBone[6].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Calf
	pTmp6->pBone[7].T = Vect(16.961571f, 0.000003f, -0.000003f);
	pTmp6->pBone[7].Q = Quat(ROT_XYZ, -0.179144f, 0.000921f, -1.038701f);
	pTmp6->pBone[7].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Foot
	pTmp6->pBone[8].T = Vect(13.263530f, -0.000000f, 0.000002f);
	pTmp6->pBone[8].Q = Quat(ROT_XYZ, 0.000910f, -0.153339f, 0.132482f);
	pTmp6->pBone[8].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Spine1
	pTmp6->pBone[9].T = Vect(17.730431f, -0.017629f, 0.000002f);
	pTmp6->pBone[9].Q = Quat(ROT_XYZ, -0.146976f, 0.099419f, 0.002539f);
	pTmp6->pBone[9].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Neck
	pTmp6->pBone[10].T = Vect(22.140881f, -0.008821f, 0.000003f);
	pTmp6->pBone[10].Q = Quat(ROT_XYZ, 0.217788f, -0.063335f, -0.081184f);
	pTmp6->pBone[10].S = Vect(1.000000f, 1.000000f, 1.000001f);

	//     Node Name: Bip01 L Clavicle
	pTmp6->pBone[11].T = Vect(0.344969f, 1.186275f, 5.319908f);
	pTmp6->pBone[11].Q = Quat(ROT_XYZ, 0.977715f, -1.059558f, 2.245525f);
	pTmp6->pBone[11].S = Vect(0.999999f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L UpperArm
	pTmp6->pBone[12].T = Vect(13.067860f, 0.000004f, -0.000004f);
	pTmp6->pBone[12].Q = Quat(ROT_XYZ, -0.060241f, 0.801118f, 0.089059f);
	pTmp6->pBone[12].S = Vect(1.000000f, 1.000001f, 1.000001f);

	//     Node Name: Bip01 L Forearm
	pTmp6->pBone[13].T = Vect(15.099380f, -0.000849f, 0.000336f);
	pTmp6->pBone[13].Q = Quat(ROT_XYZ, 0.764798f, 0.046213f, -0.568218f);
	pTmp6->pBone[13].S = Vect(1.000000f, 1.000001f, 1.000001f);

	//     Node Name: Bip01 L Hand
	pTmp6->pBone[14].T = Vect(15.099410f, 0.000082f, 0.000072f);
	pTmp6->pBone[14].Q = Quat(ROT_XYZ, -1.584537f, 0.170981f, -0.213083f);
	pTmp6->pBone[14].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Clavicle
	pTmp6->pBone[15].T = Vect(-0.346380f, -1.169084f, -5.323622f);
	pTmp6->pBone[15].Q = Quat(ROT_XYZ, -0.248313f, 1.117032f, 2.847506f);
	pTmp6->pBone[15].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R UpperArm
	pTmp6->pBone[16].T = Vect(13.067840f, -0.000001f, 0.000001f);
	pTmp6->pBone[16].Q = Quat(ROT_XYZ, -0.057036f, -0.807177f, 0.385402f);
	pTmp6->pBone[16].S = Vect(1.000000f, 1.000000f, 1.000001f);

	//     Node Name: Bip01 R Forearm
	pTmp6->pBone[17].T = Vect(15.099410f, -0.000312f, -0.000006f);
	pTmp6->pBone[17].Q = Quat(ROT_XYZ, -0.877783f, -0.023913f, -0.032898f);
	pTmp6->pBone[17].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Hand
	pTmp6->pBone[18].T = Vect(15.099400f, 0.000180f, -0.000104f);
	pTmp6->pBone[18].Q = Quat(ROT_XYZ, 1.569602f, -0.100730f, -0.105726f);
	pTmp6->pBone[18].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Head
	pTmp6->pBone[19].T = Vect(11.079190f, 0.000001f, 0.000001f);
	pTmp6->pBone[19].Q = Quat(ROT_XYZ, 0.140443f, -0.041052f, 0.303770f);
	pTmp6->pBone[19].S = Vect(1.000000f, 1.000000f, 1.000000f);



	//------- Key Time: 7  ms: 233 ------------------ 

	Frame_Bucket *pTmp7 = new Frame_Bucket();
	pTmp7->prevBucket = pTmp6;
	pTmp7->nextBucket = 0;
	pTmp7->KeyTime = 7 * Time(TIME_NTSC_30_FRAME);
	pTmp7->pBone = new Bone[NUM_BONES];
	pTmp6->nextBucket = pTmp7;

	//     Node Name: RootNode
	//     Node Name: Bip01
	pTmp7->pBone[0].T = Vect(2.838766f, -4.633778f, 0.405576f);
	pTmp7->pBone[0].Q = Quat(ROT_XYZ, 2.517321f, -1.463172f, 2.219733f);
	pTmp7->pBone[0].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Pelvis
	pTmp7->pBone[1].T = Vect(0.0f, 0.0f, 0.0f);
	pTmp7->pBone[1].Q = Quat(ROT_XYZ, 0.0f, 0.0f, 0.0f);
	pTmp7->pBone[1].S = Vect(1.0f, 1.0f, 1.0f);

	//     Node Name: Bip01 Spine
	pTmp7->pBone[2].T = Vect(6.016934f, -0.014115f, -0.000028f);
	pTmp7->pBone[2].Q = Quat(ROT_XYZ, -0.068441f, 0.050783f, 0.005791f);
	pTmp7->pBone[2].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Thigh
	pTmp7->pBone[3].T = Vect(-6.702901f, -0.863988f, 13.320410f);
	pTmp7->pBone[3].Q = Quat(ROT_XYZ, 2.983908f, -0.112928f, -2.987978f);
	pTmp7->pBone[3].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Calf
	pTmp7->pBone[4].T = Vect(16.961559f, -0.000002f, -0.000004f);
	pTmp7->pBone[4].Q = Quat(ROT_XYZ, 0.170156f, -0.037084f, -1.205630f);
	pTmp7->pBone[4].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Foot
	pTmp7->pBone[5].T = Vect(13.263550f, 0.000003f, 0.000002f);
	pTmp7->pBone[5].Q = Quat(ROT_XYZ, -0.049092f, -0.067565f, 0.509919f);
	pTmp7->pBone[5].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Thigh
	pTmp7->pBone[6].T = Vect(-5.314929f, 1.003463f, -13.923130f);
	pTmp7->pBone[6].Q = Quat(ROT_XYZ, 3.066619f, 0.066282f, 2.379782f);
	pTmp7->pBone[6].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Calf
	pTmp7->pBone[7].T = Vect(16.961571f, 0.000003f, -0.000003f);
	pTmp7->pBone[7].Q = Quat(ROT_XYZ, -0.173736f, 0.003644f, -1.062866f);
	pTmp7->pBone[7].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Foot
	pTmp7->pBone[8].T = Vect(13.263530f, -0.000000f, 0.000002f);
	pTmp7->pBone[8].Q = Quat(ROT_XYZ, 0.000652f, -0.151057f, 0.206987f);
	pTmp7->pBone[8].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Spine1
	pTmp7->pBone[9].T = Vect(17.730431f, -0.017629f, 0.000002f);
	pTmp7->pBone[9].Q = Quat(ROT_XYZ, -0.136886f, 0.101790f, 0.006510f);
	pTmp7->pBone[9].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Neck
	pTmp7->pBone[10].T = Vect(22.140881f, -0.008821f, 0.000003f);
	pTmp7->pBone[10].Q = Quat(ROT_XYZ, 0.206744f, -0.070979f, -0.074007f);
	pTmp7->pBone[10].S = Vect(1.000000f, 1.000000f, 1.000001f);

	//     Node Name: Bip01 L Clavicle
	pTmp7->pBone[11].T = Vect(0.386686f, 1.126879f, 5.329941f);
	pTmp7->pBone[11].Q = Quat(ROT_XYZ, 0.960715f, -1.070591f, 2.253798f);
	pTmp7->pBone[11].S = Vect(0.999999f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L UpperArm
	pTmp7->pBone[12].T = Vect(13.067860f, 0.000004f, -0.000004f);
	pTmp7->pBone[12].Q = Quat(ROT_XYZ, -0.041536f, 0.848676f, 0.110679f);
	pTmp7->pBone[12].S = Vect(1.000000f, 1.000001f, 1.000001f);

	//     Node Name: Bip01 L Forearm
	pTmp7->pBone[13].T = Vect(15.099380f, -0.000572f, 0.000280f);
	pTmp7->pBone[13].Q = Quat(ROT_XYZ, 0.789029f, 0.039590f, -0.503130f);
	pTmp7->pBone[13].S = Vect(1.000000f, 1.000001f, 1.000001f);

	//     Node Name: Bip01 L Hand
	pTmp7->pBone[14].T = Vect(15.099410f, 0.000081f, 0.000072f);
	pTmp7->pBone[14].Q = Quat(ROT_XYZ, -1.582597f, 0.164157f, -0.220776f);
	pTmp7->pBone[14].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Clavicle
	pTmp7->pBone[15].T = Vect(-0.387962f, -1.109637f, -5.333463f);
	pTmp7->pBone[15].Q = Quat(ROT_XYZ, -0.269899f, 1.091767f, 2.812722f);
	pTmp7->pBone[15].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R UpperArm
	pTmp7->pBone[16].T = Vect(13.067840f, -0.000001f, 0.000001f);
	pTmp7->pBone[16].Q = Quat(ROT_XYZ, -0.058494f, -0.804292f, 0.391095f);
	pTmp7->pBone[16].S = Vect(1.000000f, 1.000000f, 1.000001f);

	//     Node Name: Bip01 R Forearm
	pTmp7->pBone[17].T = Vect(15.099410f, -0.000462f, -0.000048f);
	pTmp7->pBone[17].Q = Quat(ROT_XYZ, -0.873409f, -0.024418f, -0.063882f);
	pTmp7->pBone[17].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Hand
	pTmp7->pBone[18].T = Vect(15.099400f, 0.000180f, -0.000104f);
	pTmp7->pBone[18].Q = Quat(ROT_XYZ, 1.568836f, -0.097069f, -0.115528f);
	pTmp7->pBone[18].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Head
	pTmp7->pBone[19].T = Vect(11.079190f, 0.000001f, 0.000001f);
	pTmp7->pBone[19].Q = Quat(ROT_XYZ, 0.130291f, -0.030056f, 0.296635f);
	pTmp7->pBone[19].S = Vect(1.000000f, 1.000000f, 1.000000f);



	//------- Key Time: 8  ms: 266 ------------------ 

	Frame_Bucket *pTmp8 = new Frame_Bucket();
	pTmp8->prevBucket = pTmp7;
	pTmp8->nextBucket = 0;
	pTmp8->KeyTime = 8 * Time(TIME_NTSC_30_FRAME);
	pTmp8->pBone = new Bone[NUM_BONES];
	pTmp7->nextBucket = pTmp8;

	//     Node Name: RootNode
	//     Node Name: Bip01
	pTmp8->pBone[0].T = Vect(2.085836f, -3.609762f, 0.716252f);
	pTmp8->pBone[0].Q = Quat(ROT_XYZ, 2.894800f, -1.471522f, 1.825253f);
	pTmp8->pBone[0].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Pelvis
	pTmp8->pBone[1].T = Vect(0.0f, 0.0f, 0.0f);
	pTmp8->pBone[1].Q = Quat(ROT_XYZ, 0.0f, 0.0f, 0.0f);
	pTmp8->pBone[1].S = Vect(1.0f, 1.0f, 1.0f);

	//     Node Name: Bip01 Spine
	pTmp8->pBone[2].T = Vect(6.016934f, -0.014115f, -0.000028f);
	pTmp8->pBone[2].Q = Quat(ROT_XYZ, -0.062256f, 0.049257f, 0.006787f);
	pTmp8->pBone[2].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Thigh
	pTmp8->pBone[3].T = Vect(-6.682542f, -0.776248f, 13.336070f);
	pTmp8->pBone[3].Q = Quat(ROT_XYZ, 2.978739f, -0.206146f, -3.111899f);
	pTmp8->pBone[3].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Calf
	pTmp8->pBone[4].T = Vect(16.961559f, -0.000002f, -0.000004f);
	pTmp8->pBone[4].Q = Quat(ROT_XYZ, 0.220649f, -0.038670f, -1.399094f);
	pTmp8->pBone[4].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Foot
	pTmp8->pBone[5].T = Vect(13.263550f, 0.000003f, 0.000002f);
	pTmp8->pBone[5].Q = Quat(ROT_XYZ, -0.031455f, 0.005599f, 0.336514f);
	pTmp8->pBone[5].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Thigh
	pTmp8->pBone[6].T = Vect(-5.336171f, 0.922801f, -13.920600f);
	pTmp8->pBone[6].Q = Quat(ROT_XYZ, 3.077099f, 0.065305f, 2.485824f);
	pTmp8->pBone[6].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Calf
	pTmp8->pBone[7].T = Vect(16.961571f, 0.000003f, -0.000003f);
	pTmp8->pBone[7].Q = Quat(ROT_XYZ, -0.139023f, 0.009749f, -1.054121f);
	pTmp8->pBone[7].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Foot
	pTmp8->pBone[8].T = Vect(13.263530f, -0.000000f, 0.000002f);
	pTmp8->pBone[8].Q = Quat(ROT_XYZ, 0.001490f, -0.147562f, 0.295230f);
	pTmp8->pBone[8].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Spine1
	pTmp8->pBone[9].T = Vect(17.730431f, -0.017629f, 0.000002f);
	pTmp8->pBone[9].Q = Quat(ROT_XYZ, -0.124435f, 0.098796f, 0.008915f);
	pTmp8->pBone[9].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Neck
	pTmp8->pBone[10].T = Vect(22.140881f, -0.008821f, 0.000003f);
	pTmp8->pBone[10].Q = Quat(ROT_XYZ, 0.173604f, -0.054926f, -0.067634f);
	pTmp8->pBone[10].S = Vect(1.000000f, 1.000000f, 1.000001f);

	//     Node Name: Bip01 L Clavicle
	pTmp8->pBone[11].T = Vect(0.299232f, 0.950614f, 5.369769f);
	pTmp8->pBone[11].Q = Quat(ROT_XYZ, 0.901384f, -1.082054f, 2.300490f);
	pTmp8->pBone[11].S = Vect(0.999999f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L UpperArm
	pTmp8->pBone[12].T = Vect(13.067860f, 0.000004f, -0.000004f);
	pTmp8->pBone[12].Q = Quat(ROT_XYZ, -0.033524f, 0.892498f, 0.121710f);
	pTmp8->pBone[12].S = Vect(1.000000f, 1.000001f, 1.000001f);

	//     Node Name: Bip01 L Forearm
	pTmp8->pBone[13].T = Vect(15.099380f, -0.000572f, 0.000385f);
	pTmp8->pBone[13].Q = Quat(ROT_XYZ, 0.851791f, 0.032215f, -0.425167f);
	pTmp8->pBone[13].S = Vect(1.000000f, 1.000001f, 1.000001f);

	//     Node Name: Bip01 L Hand
	pTmp8->pBone[14].T = Vect(15.099410f, 0.000081f, 0.000072f);
	pTmp8->pBone[14].Q = Quat(ROT_XYZ, -1.580733f, 0.159293f, -0.217599f);
	pTmp8->pBone[14].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Clavicle
	pTmp8->pBone[15].T = Vect(-0.300424f, -0.933267f, -5.372749f);
	pTmp8->pBone[15].Q = Quat(ROT_XYZ, -0.347538f, 1.079255f, 2.734561f);
	pTmp8->pBone[15].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R UpperArm
	pTmp8->pBone[16].T = Vect(13.067840f, -0.000001f, 0.000001f);
	pTmp8->pBone[16].Q = Quat(ROT_XYZ, -0.067992f, -0.809045f, 0.410003f);
	pTmp8->pBone[16].S = Vect(1.000000f, 1.000000f, 1.000001f);

	//     Node Name: Bip01 R Forearm
	pTmp8->pBone[17].T = Vect(15.099410f, 0.000125f, 0.000128f);
	pTmp8->pBone[17].Q = Quat(ROT_XYZ, -0.849644f, -0.025308f, -0.133310f);
	pTmp8->pBone[17].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Hand
	pTmp8->pBone[18].T = Vect(15.099400f, 0.000180f, -0.000104f);
	pTmp8->pBone[18].Q = Quat(ROT_XYZ, 1.564882f, -0.067672f, -0.140758f);
	pTmp8->pBone[18].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Head
	pTmp8->pBone[19].T = Vect(11.079190f, 0.000001f, 0.000001f);
	pTmp8->pBone[19].Q = Quat(ROT_XYZ, 0.112765f, -0.036621f, 0.295145f);
	pTmp8->pBone[19].S = Vect(1.000000f, 1.000000f, 1.000000f);



	//------- Key Time: 9  ms: 300 ------------------ 

	Frame_Bucket *pTmp9 = new Frame_Bucket();
	pTmp9->prevBucket = pTmp8;
	pTmp9->nextBucket = 0;
	pTmp9->KeyTime = 9 * Time(TIME_NTSC_30_FRAME);
	pTmp9->pBone = new Bone[NUM_BONES];
	pTmp8->nextBucket = pTmp9;

	//     Node Name: RootNode
	//     Node Name: Bip01
	pTmp9->pBone[0].T = Vect(1.512756f, -2.702288f, 1.005490f);
	pTmp9->pBone[0].Q = Quat(ROT_XYZ, -3.050727f, -1.470720f, 1.473433f);
	pTmp9->pBone[0].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Pelvis
	pTmp9->pBone[1].T = Vect(0.0f, 0.0f, 0.0f);
	pTmp9->pBone[1].Q = Quat(ROT_XYZ, 0.0f, 0.0f, 0.0f);
	pTmp9->pBone[1].S = Vect(1.0f, 1.0f, 1.0f);

	//     Node Name: Bip01 Spine
	pTmp9->pBone[2].T = Vect(6.016934f, -0.014115f, -0.000028f);
	pTmp9->pBone[2].Q = Quat(ROT_XYZ, -0.051056f, 0.042021f, 0.006448f);
	pTmp9->pBone[2].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Thigh
	pTmp9->pBone[3].T = Vect(-6.585657f, -0.631323f, 13.391760f);
	pTmp9->pBone[3].Q = Quat(ROT_XYZ, 3.001213f, -0.276431f, 3.027159f);
	pTmp9->pBone[3].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Calf
	pTmp9->pBone[4].T = Vect(16.961559f, -0.000002f, -0.000004f);
	pTmp9->pBone[4].Q = Quat(ROT_XYZ, 0.209822f, -0.036548f, -1.535007f);
	pTmp9->pBone[4].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Foot
	pTmp9->pBone[5].T = Vect(13.263550f, 0.000003f, 0.000002f);
	pTmp9->pBone[5].Q = Quat(ROT_XYZ, -0.032088f, 0.032202f, 0.092665f);
	pTmp9->pBone[5].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Thigh
	pTmp9->pBone[6].T = Vect(-5.436934f, 0.762813f, -13.891240f);
	pTmp9->pBone[6].Q = Quat(ROT_XYZ, 3.092261f, 0.057137f, 2.591580f);
	pTmp9->pBone[6].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Calf
	pTmp9->pBone[7].T = Vect(16.961571f, 0.000003f, -0.000003f);
	pTmp9->pBone[7].Q = Quat(ROT_XYZ, -0.099989f, 0.015023f, -1.028554f);
	pTmp9->pBone[7].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Foot
	pTmp9->pBone[8].T = Vect(13.263530f, -0.000000f, 0.000002f);
	pTmp9->pBone[8].Q = Quat(ROT_XYZ, 0.004197f, -0.137985f, 0.370855f);
	pTmp9->pBone[8].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Spine1
	pTmp9->pBone[9].T = Vect(17.730431f, -0.017629f, 0.000002f);
	pTmp9->pBone[9].Q = Quat(ROT_XYZ, -0.102001f, 0.084281f, 0.009161f);
	pTmp9->pBone[9].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Neck
	pTmp9->pBone[10].T = Vect(22.140881f, -0.008821f, 0.000003f);
	pTmp9->pBone[10].Q = Quat(ROT_XYZ, 0.133252f, -0.032753f, -0.063306f);
	pTmp9->pBone[10].S = Vect(1.000000f, 1.000000f, 1.000001f);

	//     Node Name: Bip01 L Clavicle
	pTmp9->pBone[11].T = Vect(0.178298f, 0.733926f, 5.408994f);
	pTmp9->pBone[11].Q = Quat(ROT_XYZ, 0.827592f, -1.091555f, 2.361170f);
	pTmp9->pBone[11].S = Vect(0.999999f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L UpperArm
	pTmp9->pBone[12].T = Vect(13.067860f, 0.000004f, -0.000004f);
	pTmp9->pBone[12].Q = Quat(ROT_XYZ, -0.019166f, 0.918002f, 0.144212f);
	pTmp9->pBone[12].S = Vect(1.000000f, 1.000001f, 1.000001f);

	//     Node Name: Bip01 L Forearm
	pTmp9->pBone[13].T = Vect(15.099380f, -0.000566f, 0.000389f);
	pTmp9->pBone[13].Q = Quat(ROT_XYZ, 0.896970f, 0.026229f, -0.357250f);
	pTmp9->pBone[13].S = Vect(1.000000f, 1.000001f, 1.000001f);

	//     Node Name: Bip01 L Hand
	pTmp9->pBone[14].T = Vect(15.099410f, 0.000081f, 0.000071f);
	pTmp9->pBone[14].Q = Quat(ROT_XYZ, -1.579742f, 0.157712f, -0.214081f);
	pTmp9->pBone[14].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Clavicle
	pTmp9->pBone[15].T = Vect(-0.179405f, -0.716468f, -5.411300f);
	pTmp9->pBone[15].Q = Quat(ROT_XYZ, -0.429738f, 1.056470f, 2.654152f);
	pTmp9->pBone[15].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R UpperArm
	pTmp9->pBone[16].T = Vect(13.067840f, -0.000001f, 0.000001f);
	pTmp9->pBone[16].Q = Quat(ROT_XYZ, -0.070428f, -0.813363f, 0.416206f);
	pTmp9->pBone[16].S = Vect(1.000000f, 1.000000f, 1.000001f);

	//     Node Name: Bip01 R Forearm
	pTmp9->pBone[17].T = Vect(15.099410f, -0.000488f, -0.000058f);
	pTmp9->pBone[17].Q = Quat(ROT_XYZ, -0.796458f, -0.026828f, -0.198517f);
	pTmp9->pBone[17].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Hand
	pTmp9->pBone[18].T = Vect(15.099400f, 0.000180f, -0.000104f);
	pTmp9->pBone[18].Q = Quat(ROT_XYZ, 1.561261f, -0.040811f, -0.145535f);
	pTmp9->pBone[18].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Head
	pTmp9->pBone[19].T = Vect(11.079190f, 0.000001f, 0.000001f);
	pTmp9->pBone[19].Q = Quat(ROT_XYZ, 0.092148f, -0.032297f, 0.305984f);
	pTmp9->pBone[19].S = Vect(1.000000f, 1.000000f, 1.000000f);



	//------- Key Time: 10  ms: 333 ------------------ 

	Frame_Bucket *pTmp10 = new Frame_Bucket();
	pTmp10->prevBucket = pTmp9;
	pTmp10->nextBucket = 0;
	pTmp10->KeyTime = 10 * Time(TIME_NTSC_30_FRAME);
	pTmp10->pBone = new Bone[NUM_BONES];
	pTmp9->nextBucket = pTmp10;

	//     Node Name: RootNode
	//     Node Name: Bip01
	pTmp10->pBone[0].T = Vect(1.495171f, -1.854068f, 1.402145f);
	pTmp10->pBone[0].Q = Quat(ROT_XYZ, -2.867626f, -1.465084f, 1.286162f);
	pTmp10->pBone[0].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Pelvis
	pTmp10->pBone[1].T = Vect(0.0f, 0.0f, 0.0f);
	pTmp10->pBone[1].Q = Quat(ROT_XYZ, 0.0f, 0.0f, 0.0f);
	pTmp10->pBone[1].S = Vect(1.0f, 1.0f, 1.0f);

	//     Node Name: Bip01 Spine
	pTmp10->pBone[2].T = Vect(6.016934f, -0.014115f, -0.000028f);
	pTmp10->pBone[2].Q = Quat(ROT_XYZ, -0.035481f, 0.032394f, 0.007703f);
	pTmp10->pBone[2].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Thigh
	pTmp10->pBone[3].T = Vect(-6.456293f, -0.417351f, 13.462990f);
	pTmp10->pBone[3].Q = Quat(ROT_XYZ, 3.036427f, -0.308958f, 2.901255f);
	pTmp10->pBone[3].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Calf
	pTmp10->pBone[4].T = Vect(16.961559f, -0.000002f, -0.000004f);
	pTmp10->pBone[4].Q = Quat(ROT_XYZ, 0.182567f, -0.033603f, -1.620753f);
	pTmp10->pBone[4].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Foot
	pTmp10->pBone[5].T = Vect(13.263550f, 0.000003f, 0.000002f);
	pTmp10->pBone[5].Q = Quat(ROT_XYZ, -0.034670f, 0.009300f, -0.027349f);
	pTmp10->pBone[5].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Thigh
	pTmp10->pBone[6].T = Vect(-5.570634f, 0.552039f, -13.848220f);
	pTmp10->pBone[6].Q = Quat(ROT_XYZ, 3.114426f, 0.051322f, 2.685772f);
	pTmp10->pBone[6].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Calf
	pTmp10->pBone[7].T = Vect(16.961571f, 0.000003f, -0.000003f);
	pTmp10->pBone[7].Q = Quat(ROT_XYZ, -0.079153f, 0.018220f, -0.995541f);
	pTmp10->pBone[7].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Foot
	pTmp10->pBone[8].T = Vect(13.263530f, -0.000000f, 0.000002f);
	pTmp10->pBone[8].Q = Quat(ROT_XYZ, 0.006347f, -0.135904f, 0.430432f);
	pTmp10->pBone[8].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Spine1
	pTmp10->pBone[9].T = Vect(17.730431f, -0.017629f, 0.000002f);
	pTmp10->pBone[9].Q = Quat(ROT_XYZ, -0.070784f, 0.065017f, 0.012669f);
	pTmp10->pBone[9].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Neck
	pTmp10->pBone[10].T = Vect(22.140881f, -0.008821f, 0.000003f);
	pTmp10->pBone[10].Q = Quat(ROT_XYZ, 0.093726f, -0.020170f, -0.059053f);
	pTmp10->pBone[10].S = Vect(1.000000f, 1.000000f, 1.000001f);

	//     Node Name: Bip01 L Clavicle
	pTmp10->pBone[11].T = Vect(0.109632f, 0.519784f, 5.435572f);
	pTmp10->pBone[11].Q = Quat(ROT_XYZ, 0.743441f, -1.100252f, 2.431984f);
	pTmp10->pBone[11].S = Vect(0.999999f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L UpperArm
	pTmp10->pBone[12].T = Vect(13.067860f, 0.000004f, -0.000004f);
	pTmp10->pBone[12].Q = Quat(ROT_XYZ, -0.007263f, 0.926589f, 0.169176f);
	pTmp10->pBone[12].S = Vect(1.000000f, 1.000001f, 1.000001f);

	//     Node Name: Bip01 L Forearm
	pTmp10->pBone[13].T = Vect(15.099380f, -0.000550f, 0.000388f);
	pTmp10->pBone[13].Q = Quat(ROT_XYZ, 0.916693f, 0.021940f, -0.299087f);
	pTmp10->pBone[13].S = Vect(1.000000f, 1.000001f, 1.000001f);

	//     Node Name: Bip01 L Hand
	pTmp10->pBone[14].T = Vect(15.099410f, 0.000080f, 0.000071f);
	pTmp10->pBone[14].Q = Quat(ROT_XYZ, -1.579707f, 0.158136f, -0.206780f);
	pTmp10->pBone[14].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Clavicle
	pTmp10->pBone[15].T = Vect(-0.110662f, -0.502246f, -5.437202f);
	pTmp10->pBone[15].Q = Quat(ROT_XYZ, -0.521981f, 1.031043f, 2.563292f);
	pTmp10->pBone[15].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R UpperArm
	pTmp10->pBone[16].T = Vect(13.067840f, -0.000001f, 0.000001f);
	pTmp10->pBone[16].Q = Quat(ROT_XYZ, -0.060409f, -0.825723f, 0.402945f);
	pTmp10->pBone[16].S = Vect(1.000000f, 1.000000f, 1.000001f);

	//     Node Name: Bip01 R Forearm
	pTmp10->pBone[17].T = Vect(15.099410f, -0.000411f, -0.000048f);
	pTmp10->pBone[17].Q = Quat(ROT_XYZ, -0.747303f, -0.028590f, -0.251033f);
	pTmp10->pBone[17].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Hand
	pTmp10->pBone[18].T = Vect(15.099400f, 0.000180f, -0.000104f);
	pTmp10->pBone[18].Q = Quat(ROT_XYZ, 1.555862f, -0.005454f, -0.151767f);
	pTmp10->pBone[18].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Head
	pTmp10->pBone[19].T = Vect(11.079190f, 0.000001f, 0.000001f);
	pTmp10->pBone[19].Q = Quat(ROT_XYZ, 0.071614f, -0.026578f, 0.309501f);
	pTmp10->pBone[19].S = Vect(1.000000f, 1.000000f, 1.000000f);



	//------- Key Time: 11  ms: 366 ------------------ 

	Frame_Bucket *pTmp11 = new Frame_Bucket();
	pTmp11->prevBucket = pTmp10;
	pTmp11->nextBucket = 0;
	pTmp11->KeyTime = 11 * Time(TIME_NTSC_30_FRAME);
	pTmp11->pBone = new Bone[NUM_BONES];
	pTmp10->nextBucket = pTmp11;

	//     Node Name: RootNode
	//     Node Name: Bip01
	pTmp11->pBone[0].T = Vect(1.351138f, -1.020213f, 1.831257f);
	pTmp11->pBone[0].Q = Quat(ROT_XYZ, -2.743189f, -1.460718f, 1.159314f);
	pTmp11->pBone[0].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Pelvis
	pTmp11->pBone[1].T = Vect(0.0f, 0.0f, 0.0f);
	pTmp11->pBone[1].Q = Quat(ROT_XYZ, 0.0f, 0.0f, 0.0f);
	pTmp11->pBone[1].S = Vect(1.0f, 1.0f, 1.0f);

	//     Node Name: Bip01 Spine
	pTmp11->pBone[2].T = Vect(6.016934f, -0.014115f, -0.000028f);
	pTmp11->pBone[2].Q = Quat(ROT_XYZ, -0.015894f, 0.017584f, 0.008324f);
	pTmp11->pBone[2].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Thigh
	pTmp11->pBone[3].T = Vect(-6.256018f, -0.151355f, 13.562760f);
	pTmp11->pBone[3].Q = Quat(ROT_XYZ, 3.081891f, -0.323495f, 2.752726f);
	pTmp11->pBone[3].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Calf
	pTmp11->pBone[4].T = Vect(16.961559f, -0.000002f, -0.000004f);
	pTmp11->pBone[4].Q = Quat(ROT_XYZ, 0.211490f, -0.027520f, -1.691835f);
	pTmp11->pBone[4].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Foot
	pTmp11->pBone[5].T = Vect(13.263550f, 0.000003f, 0.000002f);
	pTmp11->pBone[5].Q = Quat(ROT_XYZ, -0.029921f, 0.027041f, 0.011055f);
	pTmp11->pBone[5].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Thigh
	pTmp11->pBone[6].T = Vect(-5.775205f, 0.283107f, -13.772270f);
	pTmp11->pBone[6].Q = Quat(ROT_XYZ, -3.138575f, 0.035434f, 2.782074f);
	pTmp11->pBone[6].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Calf
	pTmp11->pBone[7].T = Vect(16.961571f, 0.000003f, -0.000003f);
	pTmp11->pBone[7].Q = Quat(ROT_XYZ, -0.061509f, 0.020513f, -0.942616f);
	pTmp11->pBone[7].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Foot
	pTmp11->pBone[8].T = Vect(13.263530f, -0.000000f, 0.000002f);
	pTmp11->pBone[8].Q = Quat(ROT_XYZ, 0.008681f, -0.137477f, 0.473768f);
	pTmp11->pBone[8].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Spine1
	pTmp11->pBone[9].T = Vect(17.730431f, -0.017629f, 0.000002f);
	pTmp11->pBone[9].Q = Quat(ROT_XYZ, -0.031655f, 0.035287f, 0.014780f);
	pTmp11->pBone[9].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Neck
	pTmp11->pBone[10].T = Vect(22.140881f, -0.008821f, 0.000003f);
	pTmp11->pBone[10].Q = Quat(ROT_XYZ, 0.046530f, 0.004486f, -0.048892f);
	pTmp11->pBone[10].S = Vect(1.000000f, 1.000000f, 1.000001f);

	//     Node Name: Bip01 L Clavicle
	pTmp11->pBone[11].T = Vect(-0.024929f, 0.262816f, 5.455086f);
	pTmp11->pBone[11].Q = Quat(ROT_XYZ, 0.672089f, -1.114477f, 2.481684f);
	pTmp11->pBone[11].S = Vect(0.999999f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L UpperArm
	pTmp11->pBone[12].T = Vect(13.067860f, 0.000004f, -0.000004f);
	pTmp11->pBone[12].Q = Quat(ROT_XYZ, 0.012961f, 0.937851f, 0.210419f);
	pTmp11->pBone[12].S = Vect(1.000000f, 1.000001f, 1.000001f);

	//     Node Name: Bip01 L Forearm
	pTmp11->pBone[13].T = Vect(15.099380f, -0.000527f, 0.000383f);
	pTmp11->pBone[13].Q = Quat(ROT_XYZ, 0.925389f, 0.019703f, -0.258993f);
	pTmp11->pBone[13].S = Vect(1.000000f, 1.000001f, 1.000001f);

	//     Node Name: Bip01 L Hand
	pTmp11->pBone[14].T = Vect(15.099410f, 0.000080f, 0.000071f);
	pTmp11->pBone[14].Q = Quat(ROT_XYZ, -1.580741f, 0.164916f, -0.178324f);
	pTmp11->pBone[14].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Clavicle
	pTmp11->pBone[15].T = Vect(0.024075f, -0.245212f, -5.455911f);
	pTmp11->pBone[15].Q = Quat(ROT_XYZ, -0.597848f, 0.990606f, 2.482626f);
	pTmp11->pBone[15].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R UpperArm
	pTmp11->pBone[16].T = Vect(13.067840f, -0.000001f, 0.000001f);
	pTmp11->pBone[16].Q = Quat(ROT_XYZ, -0.045825f, -0.813595f, 0.384792f);
	pTmp11->pBone[16].S = Vect(1.000000f, 1.000000f, 1.000001f);

	//     Node Name: Bip01 R Forearm
	pTmp11->pBone[17].T = Vect(15.099410f, -0.000162f, 0.000039f);
	pTmp11->pBone[17].Q = Quat(ROT_XYZ, -0.705895f, -0.031527f, -0.301164f);
	pTmp11->pBone[17].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Hand
	pTmp11->pBone[18].T = Vect(15.099400f, 0.000180f, -0.000104f);
	pTmp11->pBone[18].Q = Quat(ROT_XYZ, 1.552273f, 0.016609f, -0.154960f);
	pTmp11->pBone[18].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Head
	pTmp11->pBone[19].T = Vect(11.079190f, 0.000001f, 0.000001f);
	pTmp11->pBone[19].Q = Quat(ROT_XYZ, 0.050679f, -0.024169f, 0.302480f);
	pTmp11->pBone[19].S = Vect(1.000000f, 1.000000f, 1.000000f);



	//------- Key Time: 12  ms: 400 ------------------ 

	Frame_Bucket *pTmp12 = new Frame_Bucket();
	pTmp12->prevBucket = pTmp11;
	pTmp12->nextBucket = 0;
	pTmp12->KeyTime = 12 * Time(TIME_NTSC_30_FRAME);
	pTmp12->pBone = new Bone[NUM_BONES];
	pTmp11->nextBucket = pTmp12;

	//     Node Name: RootNode
	//     Node Name: Bip01
	pTmp12->pBone[0].T = Vect(1.087648f, -0.583442f, 2.526230f);
	pTmp12->pBone[0].Q = Quat(ROT_XYZ, -2.695662f, -1.456408f, 1.108411f);
	pTmp12->pBone[0].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Pelvis
	pTmp12->pBone[1].T = Vect(0.0f, 0.0f, 0.0f);
	pTmp12->pBone[1].Q = Quat(ROT_XYZ, 0.0f, 0.0f, 0.0f);
	pTmp12->pBone[1].S = Vect(1.0f, 1.0f, 1.0f);

	//     Node Name: Bip01 Spine
	pTmp12->pBone[2].T = Vect(6.016934f, -0.014115f, -0.000028f);
	pTmp12->pBone[2].Q = Quat(ROT_XYZ, 0.004204f, 0.003911f, 0.011678f);
	pTmp12->pBone[2].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Thigh
	pTmp12->pBone[3].T = Vect(-6.069733f, 0.141772f, 13.647240f);
	pTmp12->pBone[3].Q = Quat(ROT_XYZ, 3.121528f, -0.317971f, 2.594772f);
	pTmp12->pBone[3].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Calf
	pTmp12->pBone[4].T = Vect(16.961559f, -0.000002f, -0.000004f);
	pTmp12->pBone[4].Q = Quat(ROT_XYZ, 0.259046f, -0.022554f, -1.717491f);
	pTmp12->pBone[4].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Foot
	pTmp12->pBone[5].T = Vect(13.263550f, 0.000003f, 0.000002f);
	pTmp12->pBone[5].Q = Quat(ROT_XYZ, -0.021511f, 0.064182f, 0.074153f);
	pTmp12->pBone[5].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Thigh
	pTmp12->pBone[6].T = Vect(-5.962764f, 0.026791f, -13.695020f);
	pTmp12->pBone[6].Q = Quat(ROT_XYZ, -3.111837f, 0.013256f, 2.864009f);
	pTmp12->pBone[6].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Calf
	pTmp12->pBone[7].T = Vect(16.961571f, 0.000003f, -0.000003f);
	pTmp12->pBone[7].Q = Quat(ROT_XYZ, -0.052131f, 0.022388f, -0.900609f);
	pTmp12->pBone[7].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Foot
	pTmp12->pBone[8].T = Vect(13.263530f, -0.000000f, 0.000002f);
	pTmp12->pBone[8].Q = Quat(ROT_XYZ, 0.009291f, -0.142983f, 0.515472f);
	pTmp12->pBone[8].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Spine1
	pTmp12->pBone[9].T = Vect(17.730431f, -0.017629f, 0.000002f);
	pTmp12->pBone[9].Q = Quat(ROT_XYZ, 0.008458f, 0.007779f, 0.021781f);
	pTmp12->pBone[9].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Neck
	pTmp12->pBone[10].T = Vect(22.140881f, -0.008821f, 0.000003f);
	pTmp12->pBone[10].Q = Quat(ROT_XYZ, 0.004726f, 0.023505f, -0.045630f);
	pTmp12->pBone[10].S = Vect(1.000000f, 1.000000f, 1.000001f);

	//     Node Name: Bip01 L Clavicle
	pTmp12->pBone[11].T = Vect(-0.128744f, 0.034604f, 5.459842f);
	pTmp12->pBone[11].Q = Quat(ROT_XYZ, 0.604967f, -1.129852f, 2.535810f);
	pTmp12->pBone[11].S = Vect(0.999999f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L UpperArm
	pTmp12->pBone[12].T = Vect(13.067860f, 0.000004f, -0.000004f);
	pTmp12->pBone[12].Q = Quat(ROT_XYZ, 0.029270f, 0.937772f, 0.246028f);
	pTmp12->pBone[12].S = Vect(1.000000f, 1.000001f, 1.000001f);

	//     Node Name: Bip01 L Forearm
	pTmp12->pBone[13].T = Vect(15.099380f, -0.000500f, 0.000378f);
	pTmp12->pBone[13].Q = Quat(ROT_XYZ, 0.924683f, 0.018428f, -0.232489f);
	pTmp12->pBone[13].S = Vect(1.000000f, 1.000001f, 1.000001f);

	//     Node Name: Bip01 L Hand
	pTmp12->pBone[14].T = Vect(15.099410f, 0.000079f, 0.000070f);
	pTmp12->pBone[14].Q = Quat(ROT_XYZ, -1.583130f, 0.183593f, -0.149230f);
	pTmp12->pBone[14].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Clavicle
	pTmp12->pBone[15].T = Vect(0.127972f, -0.016976f, -5.459943f);
	pTmp12->pBone[15].Q = Quat(ROT_XYZ, -0.658553f, 0.938671f, 2.418820f);
	pTmp12->pBone[15].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R UpperArm
	pTmp12->pBone[16].T = Vect(13.067840f, -0.000001f, 0.000001f);
	pTmp12->pBone[16].Q = Quat(ROT_XYZ, -0.032145f, -0.780086f, 0.364855f);
	pTmp12->pBone[16].S = Vect(1.000000f, 1.000000f, 1.000001f);

	//     Node Name: Bip01 R Forearm
	pTmp12->pBone[17].T = Vect(15.099410f, -0.000084f, 0.000062f);
	pTmp12->pBone[17].Q = Quat(ROT_XYZ, -0.669899f, -0.036949f, -0.350855f);
	pTmp12->pBone[17].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Hand
	pTmp12->pBone[18].T = Vect(15.099400f, 0.000180f, -0.000104f);
	pTmp12->pBone[18].Q = Quat(ROT_XYZ, 1.549030f, 0.032104f, -0.155319f);
	pTmp12->pBone[18].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Head
	pTmp12->pBone[19].T = Vect(11.079190f, 0.000001f, 0.000001f);
	pTmp12->pBone[19].Q = Quat(ROT_XYZ, 0.033196f, -0.017292f, 0.294152f);
	pTmp12->pBone[19].S = Vect(1.000000f, 1.000000f, 1.000000f);



	//------- Key Time: 13  ms: 433 ------------------ 

	Frame_Bucket *pTmp13 = new Frame_Bucket();
	pTmp13->prevBucket = pTmp12;
	pTmp13->nextBucket = 0;
	pTmp13->KeyTime = 13 * Time(TIME_NTSC_30_FRAME);
	pTmp13->pBone = new Bone[NUM_BONES];
	pTmp12->nextBucket = pTmp13;

	//     Node Name: RootNode
	//     Node Name: Bip01
	pTmp13->pBone[0].T = Vect(0.745596f, -0.792079f, 3.032356f);
	pTmp13->pBone[0].Q = Quat(ROT_XYZ, -2.677280f, -1.458784f, 1.083325f);
	pTmp13->pBone[0].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Pelvis
	pTmp13->pBone[1].T = Vect(0.0f, 0.0f, 0.0f);
	pTmp13->pBone[1].Q = Quat(ROT_XYZ, 0.0f, 0.0f, 0.0f);
	pTmp13->pBone[1].S = Vect(1.0f, 1.0f, 1.0f);

	//     Node Name: Bip01 Spine
	pTmp13->pBone[2].T = Vect(6.016934f, -0.014115f, -0.000028f);
	pTmp13->pBone[2].Q = Quat(ROT_XYZ, 0.023619f, -0.009976f, 0.016114f);
	pTmp13->pBone[2].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Thigh
	pTmp13->pBone[3].T = Vect(-5.879236f, 0.435336f, 13.724230f);
	pTmp13->pBone[3].Q = Quat(ROT_XYZ, -3.129160f, -0.309560f, 2.454699f);
	pTmp13->pBone[3].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Calf
	pTmp13->pBone[4].T = Vect(16.961559f, -0.000002f, -0.000004f);
	pTmp13->pBone[4].Q = Quat(ROT_XYZ, 0.294754f, -0.020984f, -1.686983f);
	pTmp13->pBone[4].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Foot
	pTmp13->pBone[5].T = Vect(13.263550f, 0.000003f, 0.000002f);
	pTmp13->pBone[5].Q = Quat(ROT_XYZ, -0.015081f, 0.059613f, 0.121421f);
	pTmp13->pBone[5].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Thigh
	pTmp13->pBone[6].T = Vect(-6.151959f, -0.210429f, -13.609500f);
	pTmp13->pBone[6].Q = Quat(ROT_XYZ, -3.095699f, -0.015476f, 2.915605f);
	pTmp13->pBone[6].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Calf
	pTmp13->pBone[7].T = Vect(16.961571f, 0.000003f, -0.000003f);
	pTmp13->pBone[7].Q = Quat(ROT_XYZ, -0.047738f, 0.023665f, -0.891055f);
	pTmp13->pBone[7].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Foot
	pTmp13->pBone[8].T = Vect(13.263530f, -0.000000f, 0.000002f);
	pTmp13->pBone[8].Q = Quat(ROT_XYZ, 0.008211f, -0.148340f, 0.566678f);
	pTmp13->pBone[8].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Spine1
	pTmp13->pBone[9].T = Vect(17.730431f, -0.017629f, 0.000002f);
	pTmp13->pBone[9].Q = Quat(ROT_XYZ, 0.047095f, -0.020308f, 0.030398f);
	pTmp13->pBone[9].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Neck
	pTmp13->pBone[10].T = Vect(22.140881f, -0.008821f, 0.000003f);
	pTmp13->pBone[10].Q = Quat(ROT_XYZ, -0.034943f, 0.054650f, -0.058054f);
	pTmp13->pBone[10].S = Vect(1.000000f, 1.000000f, 1.000001f);

	//     Node Name: Bip01 L Clavicle
	pTmp13->pBone[11].T = Vect(-0.298835f, -0.181728f, 5.450257f);
	pTmp13->pBone[11].Q = Quat(ROT_XYZ, 0.538572f, -1.144352f, 2.605960f);
	pTmp13->pBone[11].S = Vect(0.999999f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L UpperArm
	pTmp13->pBone[12].T = Vect(13.067860f, 0.000004f, -0.000004f);
	pTmp13->pBone[12].Q = Quat(ROT_XYZ, 0.040696f, 0.937394f, 0.273565f);
	pTmp13->pBone[12].S = Vect(1.000000f, 1.000001f, 1.000001f);

	//     Node Name: Bip01 L Forearm
	pTmp13->pBone[13].T = Vect(15.099380f, -0.000474f, 0.000374f);
	pTmp13->pBone[13].Q = Quat(ROT_XYZ, 0.931714f, 0.017222f, -0.215694f);
	pTmp13->pBone[13].S = Vect(1.000000f, 1.000001f, 1.000001f);

	//     Node Name: Bip01 L Hand
	pTmp13->pBone[14].T = Vect(15.099410f, 0.000078f, 0.000070f);
	pTmp13->pBone[14].Q = Quat(ROT_XYZ, -1.584113f, 0.197758f, -0.130371f);
	pTmp13->pBone[14].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Clavicle
	pTmp13->pBone[15].T = Vect(0.297808f, 0.199332f, -5.449697f);
	pTmp13->pBone[15].Q = Quat(ROT_XYZ, -0.714411f, 0.886975f, 2.374644f);
	pTmp13->pBone[15].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R UpperArm
	pTmp13->pBone[16].T = Vect(13.067840f, -0.000001f, 0.000001f);
	pTmp13->pBone[16].Q = Quat(ROT_XYZ, -0.016827f, -0.723065f, 0.341970f);
	pTmp13->pBone[16].S = Vect(1.000000f, 1.000000f, 1.000001f);

	//     Node Name: Bip01 R Forearm
	pTmp13->pBone[17].T = Vect(15.099410f, -0.025964f, -0.005078f);
	pTmp13->pBone[17].Q = Quat(ROT_XYZ, -0.670755f, -0.044661f, -0.398769f);
	pTmp13->pBone[17].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Hand
	pTmp13->pBone[18].T = Vect(15.099400f, 0.000180f, -0.000104f);
	pTmp13->pBone[18].Q = Quat(ROT_XYZ, 1.545940f, 0.046504f, -0.158631f);
	pTmp13->pBone[18].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Head
	pTmp13->pBone[19].T = Vect(11.079190f, 0.000001f, 0.000001f);
	pTmp13->pBone[19].Q = Quat(ROT_XYZ, 0.020555f, -0.014758f, 0.289412f);
	pTmp13->pBone[19].S = Vect(1.000000f, 1.000000f, 1.000000f);



	//------- Key Time: 14  ms: 466 ------------------ 

	Frame_Bucket *pTmp14 = new Frame_Bucket();
	pTmp14->prevBucket = pTmp13;
	pTmp14->nextBucket = 0;
	pTmp14->KeyTime = 14 * Time(TIME_NTSC_30_FRAME);
	pTmp14->pBone = new Bone[NUM_BONES];
	pTmp13->nextBucket = pTmp14;

	//     Node Name: RootNode
	//     Node Name: Bip01
	pTmp14->pBone[0].T = Vect(0.420889f, -1.442035f, 3.625495f);
	pTmp14->pBone[0].Q = Quat(ROT_XYZ, -2.681034f, -1.463564f, 1.078116f);
	pTmp14->pBone[0].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Pelvis
	pTmp14->pBone[1].T = Vect(0.0f, 0.0f, 0.0f);
	pTmp14->pBone[1].Q = Quat(ROT_XYZ, 0.0f, 0.0f, 0.0f);
	pTmp14->pBone[1].S = Vect(1.0f, 1.0f, 1.0f);

	//     Node Name: Bip01 Spine
	pTmp14->pBone[2].T = Vect(6.016934f, -0.014115f, -0.000028f);
	pTmp14->pBone[2].Q = Quat(ROT_XYZ, 0.040019f, -0.020170f, 0.019106f);
	pTmp14->pBone[2].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Thigh
	pTmp14->pBone[3].T = Vect(-5.738610f, 0.680692f, 13.773700f);
	pTmp14->pBone[3].Q = Quat(ROT_XYZ, -3.107010f, -0.300321f, 2.344486f);
	pTmp14->pBone[3].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Calf
	pTmp14->pBone[4].T = Vect(16.961559f, -0.000002f, -0.000004f);
	pTmp14->pBone[4].Q = Quat(ROT_XYZ, 0.299858f, -0.022541f, -1.605814f);
	pTmp14->pBone[4].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Foot
	pTmp14->pBone[5].T = Vect(13.263550f, 0.000003f, 0.000002f);
	pTmp14->pBone[5].Q = Quat(ROT_XYZ, -0.012910f, 0.030838f, 0.152449f);
	pTmp14->pBone[5].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Thigh
	pTmp14->pBone[6].T = Vect(-6.290031f, -0.413054f, -13.541580f);
	pTmp14->pBone[6].Q = Quat(ROT_XYZ, -3.091198f, -0.046489f, 2.956640f);
	pTmp14->pBone[6].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Calf
	pTmp14->pBone[7].T = Vect(16.961571f, 0.000003f, -0.000003f);
	pTmp14->pBone[7].Q = Quat(ROT_XYZ, -0.038597f, 0.025115f, -0.903080f);
	pTmp14->pBone[7].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Foot
	pTmp14->pBone[8].T = Vect(13.263530f, -0.000000f, 0.000002f);
	pTmp14->pBone[8].Q = Quat(ROT_XYZ, 0.005623f, -0.151279f, 0.629754f);
	pTmp14->pBone[8].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Spine1
	pTmp14->pBone[9].T = Vect(17.730431f, -0.017629f, 0.000002f);
	pTmp14->pBone[9].Q = Quat(ROT_XYZ, 0.079695f, -0.041052f, 0.035809f);
	pTmp14->pBone[9].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Neck
	pTmp14->pBone[10].T = Vect(22.140881f, -0.008821f, 0.000003f);
	pTmp14->pBone[10].Q = Quat(ROT_XYZ, -0.066283f, 0.071876f, -0.080371f);
	pTmp14->pBone[10].S = Vect(1.000000f, 1.000000f, 1.000001f);

	//     Node Name: Bip01 L Clavicle
	pTmp14->pBone[11].T = Vect(-0.392910f, -0.352039f, 5.435927f);
	pTmp14->pBone[11].Q = Quat(ROT_XYZ, 0.486058f, -1.152016f, 2.673501f);
	pTmp14->pBone[11].S = Vect(0.999999f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L UpperArm
	pTmp14->pBone[12].T = Vect(13.067860f, 0.000004f, -0.000004f);
	pTmp14->pBone[12].Q = Quat(ROT_XYZ, 0.057666f, 0.924018f, 0.304644f);
	pTmp14->pBone[12].S = Vect(1.000000f, 1.000001f, 1.000001f);

	//     Node Name: Bip01 L Forearm
	pTmp14->pBone[13].T = Vect(15.099380f, -0.000451f, 0.000372f);
	pTmp14->pBone[13].Q = Quat(ROT_XYZ, 0.953043f, 0.016199f, -0.203209f);
	pTmp14->pBone[13].S = Vect(1.000000f, 1.000001f, 1.000001f);

	//     Node Name: Bip01 L Hand
	pTmp14->pBone[14].T = Vect(15.099410f, 0.000078f, 0.000069f);
	pTmp14->pBone[14].Q = Quat(ROT_XYZ, -1.585272f, 0.217041f, -0.119306f);
	pTmp14->pBone[14].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Clavicle
	pTmp14->pBone[15].T = Vect(0.391514f, 0.369596f, -5.434866f);
	pTmp14->pBone[15].Q = Quat(ROT_XYZ, -0.762760f, 0.855723f, 2.351470f);
	pTmp14->pBone[15].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R UpperArm
	pTmp14->pBone[16].T = Vect(13.067840f, -0.000001f, 0.000001f);
	pTmp14->pBone[16].Q = Quat(ROT_XYZ, 0.004555f, -0.671832f, 0.299603f);
	pTmp14->pBone[16].S = Vect(1.000000f, 1.000000f, 1.000001f);

	//     Node Name: Bip01 R Forearm
	pTmp14->pBone[17].T = Vect(15.099410f, -0.000076f, -0.000105f);
	pTmp14->pBone[17].Q = Quat(ROT_XYZ, -0.675878f, -0.052259f, -0.433062f);
	pTmp14->pBone[17].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Hand
	pTmp14->pBone[18].T = Vect(15.099400f, 0.000180f, -0.000104f);
	pTmp14->pBone[18].Q = Quat(ROT_XYZ, 1.544713f, 0.046397f, -0.170041f);
	pTmp14->pBone[18].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Head
	pTmp14->pBone[19].T = Vect(11.079190f, 0.000001f, 0.000001f);
	pTmp14->pBone[19].Q = Quat(ROT_XYZ, 0.008367f, -0.007581f, 0.294701f);
	pTmp14->pBone[19].S = Vect(1.000000f, 1.000000f, 1.000000f);



	//------- Key Time: 15  ms: 500 ------------------ 

	Frame_Bucket *pTmp15 = new Frame_Bucket();
	pTmp15->prevBucket = pTmp14;
	pTmp15->nextBucket = 0;
	pTmp15->KeyTime = 15 * Time(TIME_NTSC_30_FRAME);
	pTmp15->pBone = new Bone[NUM_BONES];
	pTmp14->nextBucket = pTmp15;

	//     Node Name: RootNode
	//     Node Name: Bip01
	pTmp15->pBone[0].T = Vect(0.115211f, -2.220148f, 4.372371f);
	pTmp15->pBone[0].Q = Quat(ROT_XYZ, -2.583800f, -1.462564f, 0.972610f);
	pTmp15->pBone[0].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Pelvis
	pTmp15->pBone[1].T = Vect(0.0f, 0.0f, 0.0f);
	pTmp15->pBone[1].Q = Quat(ROT_XYZ, 0.0f, 0.0f, 0.0f);
	pTmp15->pBone[1].S = Vect(1.0f, 1.0f, 1.0f);

	//     Node Name: Bip01 Spine
	pTmp15->pBone[2].T = Vect(6.016934f, -0.014115f, -0.000028f);
	pTmp15->pBone[2].Q = Quat(ROT_XYZ, 0.047177f, -0.027474f, 0.022712f);
	pTmp15->pBone[2].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Thigh
	pTmp15->pBone[3].T = Vect(-5.637249f, 0.802885f, 13.808930f);
	pTmp15->pBone[3].Q = Quat(ROT_XYZ, -3.099742f, -0.276977f, 2.260572f);
	pTmp15->pBone[3].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Calf
	pTmp15->pBone[4].T = Vect(16.961559f, -0.000002f, -0.000004f);
	pTmp15->pBone[4].Q = Quat(ROT_XYZ, 0.282034f, -0.023468f, -1.454374f);
	pTmp15->pBone[4].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Foot
	pTmp15->pBone[5].T = Vect(13.263550f, 0.000003f, 0.000002f);
	pTmp15->pBone[5].Q = Quat(ROT_XYZ, -0.011696f, 0.015387f, 0.142810f);
	pTmp15->pBone[5].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Thigh
	pTmp15->pBone[6].T = Vect(-6.388315f, -0.486126f, -13.493060f);
	pTmp15->pBone[6].Q = Quat(ROT_XYZ, -3.091936f, -0.071089f, 3.012900f);
	pTmp15->pBone[6].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Calf
	pTmp15->pBone[7].T = Vect(16.961571f, 0.000003f, -0.000003f);
	pTmp15->pBone[7].Q = Quat(ROT_XYZ, -0.024160f, 0.027423f, -0.904486f);
	pTmp15->pBone[7].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Foot
	pTmp15->pBone[8].T = Vect(13.263530f, -0.000000f, 0.000002f);
	pTmp15->pBone[8].Q = Quat(ROT_XYZ, 0.003498f, -0.146161f, 0.698280f);
	pTmp15->pBone[8].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Spine1
	pTmp15->pBone[9].T = Vect(17.730431f, -0.017629f, 0.000002f);
	pTmp15->pBone[9].Q = Quat(ROT_XYZ, 0.093802f, -0.055942f, 0.042536f);
	pTmp15->pBone[9].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Neck
	pTmp15->pBone[10].T = Vect(22.140881f, -0.008821f, 0.000003f);
	pTmp15->pBone[10].Q = Quat(ROT_XYZ, -0.085267f, 0.074376f, -0.097844f);
	pTmp15->pBone[10].S = Vect(1.000000f, 1.000000f, 1.000001f);

	//     Node Name: Bip01 L Clavicle
	pTmp15->pBone[11].T = Vect(-0.406697f, -0.455091f, 5.427258f);
	pTmp15->pBone[11].Q = Quat(ROT_XYZ, 0.423546f, -1.160139f, 2.751257f);
	pTmp15->pBone[11].S = Vect(0.999999f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L UpperArm
	pTmp15->pBone[12].T = Vect(13.067860f, 0.000004f, -0.000004f);
	pTmp15->pBone[12].Q = Quat(ROT_XYZ, 0.061015f, 0.909305f, 0.316694f);
	pTmp15->pBone[12].S = Vect(1.000000f, 1.000001f, 1.000001f);

	//     Node Name: Bip01 L Forearm
	pTmp15->pBone[13].T = Vect(15.099380f, -0.000434f, 0.000371f);
	pTmp15->pBone[13].Q = Quat(ROT_XYZ, 0.973286f, 0.014976f, -0.190741f);
	pTmp15->pBone[13].S = Vect(1.000000f, 1.000001f, 1.000001f);

	//     Node Name: Bip01 L Hand
	pTmp15->pBone[14].T = Vect(15.099410f, 0.000077f, 0.000068f);
	pTmp15->pBone[14].Q = Quat(ROT_XYZ, -1.584962f, 0.224824f, -0.109824f);
	pTmp15->pBone[14].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Clavicle
	pTmp15->pBone[15].T = Vect(0.404963f, 0.472597f, -5.425889f);
	pTmp15->pBone[15].Q = Quat(ROT_XYZ, -0.798536f, 0.829984f, 2.332629f);
	pTmp15->pBone[15].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R UpperArm
	pTmp15->pBone[16].T = Vect(13.067840f, -0.000001f, 0.000001f);
	pTmp15->pBone[16].Q = Quat(ROT_XYZ, 0.030888f, -0.620130f, 0.255091f);
	pTmp15->pBone[16].S = Vect(1.000000f, 1.000000f, 1.000001f);

	//     Node Name: Bip01 R Forearm
	pTmp15->pBone[17].T = Vect(15.099410f, -0.000700f, -0.000160f);
	pTmp15->pBone[17].Q = Quat(ROT_XYZ, -0.689384f, -0.059678f, -0.459468f);
	pTmp15->pBone[17].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Hand
	pTmp15->pBone[18].T = Vect(15.099400f, 0.000180f, -0.000104f);
	pTmp15->pBone[18].Q = Quat(ROT_XYZ, 1.544522f, 0.047604f, -0.168847f);
	pTmp15->pBone[18].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Head
	pTmp15->pBone[19].T = Vect(11.079190f, 0.000001f, 0.000001f);
	pTmp15->pBone[19].Q = Quat(ROT_XYZ, 0.001365f, 0.005485f, 0.289952f);
	pTmp15->pBone[19].S = Vect(1.000000f, 1.000000f, 1.000000f);



	//------- Key Time: 16  ms: 533 ------------------ 

	Frame_Bucket *pTmp16 = new Frame_Bucket();
	pTmp16->prevBucket = pTmp15;
	pTmp16->nextBucket = 0;
	pTmp16->KeyTime = 16 * Time(TIME_NTSC_30_FRAME);
	pTmp16->pBone = new Bone[NUM_BONES];
	pTmp15->nextBucket = pTmp16;

	//     Node Name: RootNode
	//     Node Name: Bip01
	pTmp16->pBone[0].T = Vect(-0.042185f, -2.758780f, 4.664886f);
	pTmp16->pBone[0].Q = Quat(ROT_XYZ, -2.424813f, -1.466447f, 0.807488f);
	pTmp16->pBone[0].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Pelvis
	pTmp16->pBone[1].T = Vect(0.0f, 0.0f, 0.0f);
	pTmp16->pBone[1].Q = Quat(ROT_XYZ, 0.0f, 0.0f, 0.0f);
	pTmp16->pBone[1].S = Vect(1.0f, 1.0f, 1.0f);

	//     Node Name: Bip01 Spine
	pTmp16->pBone[2].T = Vect(6.016934f, -0.014115f, -0.000028f);
	pTmp16->pBone[2].Q = Quat(ROT_XYZ, 0.054609f, -0.033660f, 0.020279f);
	pTmp16->pBone[2].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Thigh
	pTmp16->pBone[3].T = Vect(-5.551884f, 0.892769f, 13.837930f);
	pTmp16->pBone[3].Q = Quat(ROT_XYZ, -3.098568f, -0.245535f, 2.226984f);
	pTmp16->pBone[3].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Calf
	pTmp16->pBone[4].T = Vect(16.961559f, -0.000002f, -0.000004f);
	pTmp16->pBone[4].Q = Quat(ROT_XYZ, 0.244699f, -0.021220f, -1.222078f);
	pTmp16->pBone[4].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Foot
	pTmp16->pBone[5].T = Vect(13.263550f, 0.000003f, 0.000002f);
	pTmp16->pBone[5].Q = Quat(ROT_XYZ, -0.011244f, -0.016427f, 0.111508f);
	pTmp16->pBone[5].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Thigh
	pTmp16->pBone[6].T = Vect(-6.472013f, -0.598828f, -13.448550f);
	pTmp16->pBone[6].Q = Quat(ROT_XYZ, -3.091757f, -0.093714f, 3.087466f);
	pTmp16->pBone[6].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Calf
	pTmp16->pBone[7].T = Vect(16.961571f, 0.000003f, -0.000003f);
	pTmp16->pBone[7].Q = Quat(ROT_XYZ, -0.007181f, 0.027580f, -0.862119f);
	pTmp16->pBone[7].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Foot
	pTmp16->pBone[8].T = Vect(13.263530f, -0.000000f, 0.000002f);
	pTmp16->pBone[8].Q = Quat(ROT_XYZ, 0.004482f, -0.137343f, 0.743802f);
	pTmp16->pBone[8].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Spine1
	pTmp16->pBone[9].T = Vect(17.730431f, -0.017629f, 0.000002f);
	pTmp16->pBone[9].Q = Quat(ROT_XYZ, 0.108654f, -0.068325f, 0.037130f);
	pTmp16->pBone[9].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Neck
	pTmp16->pBone[10].T = Vect(22.140881f, -0.008821f, 0.000003f);
	pTmp16->pBone[10].Q = Quat(ROT_XYZ, -0.108035f, 0.076748f, -0.103727f);
	pTmp16->pBone[10].S = Vect(1.000000f, 1.000000f, 1.000001f);

	//     Node Name: Bip01 L Clavicle
	pTmp16->pBone[11].T = Vect(-0.419652f, -0.578432f, 5.414514f);
	pTmp16->pBone[11].Q = Quat(ROT_XYZ, 0.351176f, -1.170501f, 2.826821f);
	pTmp16->pBone[11].S = Vect(0.999999f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L UpperArm
	pTmp16->pBone[12].T = Vect(13.067860f, 0.000004f, -0.000004f);
	pTmp16->pBone[12].Q = Quat(ROT_XYZ, 0.063245f, 0.903230f, 0.330965f);
	pTmp16->pBone[12].S = Vect(1.000000f, 1.000001f, 1.000001f);

	//     Node Name: Bip01 L Forearm
	pTmp16->pBone[13].T = Vect(15.099380f, -0.000428f, 0.000371f);
	pTmp16->pBone[13].Q = Quat(ROT_XYZ, 0.988224f, 0.014430f, -0.176618f);
	pTmp16->pBone[13].S = Vect(1.000000f, 1.000001f, 1.000001f);

	//     Node Name: Bip01 L Hand
	pTmp16->pBone[14].T = Vect(15.099410f, 0.000076f, 0.000068f);
	pTmp16->pBone[14].Q = Quat(ROT_XYZ, -1.584803f, 0.229204f, -0.098258f);
	pTmp16->pBone[14].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Clavicle
	pTmp16->pBone[15].T = Vect(0.417836f, 0.595894f, -5.412762f);
	pTmp16->pBone[15].Q = Quat(ROT_XYZ, -0.826918f, 0.814015f, 2.316066f);
	pTmp16->pBone[15].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R UpperArm
	pTmp16->pBone[16].T = Vect(13.067840f, -0.000001f, 0.000001f);
	pTmp16->pBone[16].Q = Quat(ROT_XYZ, 0.057234f, -0.578815f, 0.217238f);
	pTmp16->pBone[16].S = Vect(1.000000f, 1.000000f, 1.000001f);

	//     Node Name: Bip01 R Forearm
	pTmp16->pBone[17].T = Vect(15.099410f, -0.000802f, -0.000126f);
	pTmp16->pBone[17].Q = Quat(ROT_XYZ, -0.694479f, -0.063944f, -0.471549f);
	pTmp16->pBone[17].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Hand
	pTmp16->pBone[18].T = Vect(15.099400f, 0.000180f, -0.000104f);
	pTmp16->pBone[18].Q = Quat(ROT_XYZ, 1.543428f, 0.060414f, -0.162042f);
	pTmp16->pBone[18].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Head
	pTmp16->pBone[19].T = Vect(11.079190f, 0.000001f, 0.000001f);
	pTmp16->pBone[19].Q = Quat(ROT_XYZ, -0.009770f, 0.013575f, 0.285019f);
	pTmp16->pBone[19].S = Vect(1.000000f, 1.000000f, 1.000000f);



	//------- Key Time: 17  ms: 566 ------------------ 

	Frame_Bucket *pTmp17 = new Frame_Bucket();
	pTmp17->prevBucket = pTmp16;
	pTmp17->nextBucket = 0;
	pTmp17->KeyTime = 17 * Time(TIME_NTSC_30_FRAME);
	pTmp17->pBone = new Bone[NUM_BONES];
	pTmp16->nextBucket = pTmp17;

	//     Node Name: RootNode
	//     Node Name: Bip01
	pTmp17->pBone[0].T = Vect(-0.059924f, -2.962282f, 4.915075f);
	pTmp17->pBone[0].Q = Quat(ROT_XYZ, -2.302589f, -1.472631f, 0.682177f);
	pTmp17->pBone[0].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Pelvis
	pTmp17->pBone[1].T = Vect(0.0f, 0.0f, 0.0f);
	pTmp17->pBone[1].Q = Quat(ROT_XYZ, 0.0f, 0.0f, 0.0f);
	pTmp17->pBone[1].S = Vect(1.0f, 1.0f, 1.0f);

	//     Node Name: Bip01 Spine
	pTmp17->pBone[2].T = Vect(6.016934f, -0.014115f, -0.000028f);
	pTmp17->pBone[2].Q = Quat(ROT_XYZ, 0.061648f, -0.038539f, 0.014561f);
	pTmp17->pBone[2].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Thigh
	pTmp17->pBone[3].T = Vect(-5.484912f, 0.957488f, 13.860340f);
	pTmp17->pBone[3].Q = Quat(ROT_XYZ, -3.097407f, -0.212546f, 2.244371f);
	pTmp17->pBone[3].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Calf
	pTmp17->pBone[4].T = Vect(16.961559f, -0.000002f, -0.000004f);
	pTmp17->pBone[4].Q = Quat(ROT_XYZ, 0.181871f, -0.013742f, -0.928572f);
	pTmp17->pBone[4].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Foot
	pTmp17->pBone[5].T = Vect(13.263550f, 0.000003f, 0.000002f);
	pTmp17->pBone[5].Q = Quat(ROT_XYZ, -0.008706f, -0.037858f, 0.047400f);
	pTmp17->pBone[5].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Thigh
	pTmp17->pBone[6].T = Vect(-6.538342f, -0.725858f, -13.410180f);
	pTmp17->pBone[6].Q = Quat(ROT_XYZ, -3.095835f, -0.113494f, -3.095094f);
	pTmp17->pBone[6].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Calf
	pTmp17->pBone[7].T = Vect(16.961571f, 0.000003f, -0.000003f);
	pTmp17->pBone[7].Q = Quat(ROT_XYZ, 0.008352f, 0.025339f, -0.767499f);
	pTmp17->pBone[7].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Foot
	pTmp17->pBone[8].T = Vect(13.263530f, -0.000000f, 0.000002f);
	pTmp17->pBone[8].Q = Quat(ROT_XYZ, 0.007535f, -0.127529f, 0.758374f);
	pTmp17->pBone[8].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Spine1
	pTmp17->pBone[9].T = Vect(17.730431f, -0.017629f, 0.000002f);
	pTmp17->pBone[9].Q = Quat(ROT_XYZ, 0.122907f, -0.077847f, 0.025154f);
	pTmp17->pBone[9].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Neck
	pTmp17->pBone[10].T = Vect(22.140881f, -0.008821f, 0.000003f);
	pTmp17->pBone[10].Q = Quat(ROT_XYZ, -0.124183f, 0.079620f, -0.101822f);
	pTmp17->pBone[10].S = Vect(1.000000f, 1.000000f, 1.000001f);

	//     Node Name: Bip01 L Clavicle
	pTmp17->pBone[11].T = Vect(-0.435271f, -0.665634f, 5.403249f);
	pTmp17->pBone[11].Q = Quat(ROT_XYZ, 0.290734f, -1.190147f, 2.886993f);
	pTmp17->pBone[11].S = Vect(0.999999f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L UpperArm
	pTmp17->pBone[12].T = Vect(13.067860f, 0.000004f, -0.000004f);
	pTmp17->pBone[12].Q = Quat(ROT_XYZ, 0.067523f, 0.916919f, 0.351050f);
	pTmp17->pBone[12].S = Vect(1.000000f, 1.000001f, 1.000001f);

	//     Node Name: Bip01 L Forearm
	pTmp17->pBone[13].T = Vect(15.099380f, -0.000461f, 0.000371f);
	pTmp17->pBone[13].Q = Quat(ROT_XYZ, 0.977127f, 0.014698f, -0.161530f);
	pTmp17->pBone[13].S = Vect(1.000000f, 1.000001f, 1.000001f);

	//     Node Name: Bip01 L Hand
	pTmp17->pBone[14].T = Vect(15.099410f, 0.000075f, 0.000067f);
	pTmp17->pBone[14].Q = Quat(ROT_XYZ, -1.585603f, 0.244963f, -0.068705f);
	pTmp17->pBone[14].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Clavicle
	pTmp17->pBone[15].T = Vect(0.433492f, 0.683070f, -5.401217f);
	pTmp17->pBone[15].Q = Quat(ROT_XYZ, -0.846360f, 0.808062f, 2.301121f);
	pTmp17->pBone[15].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R UpperArm
	pTmp17->pBone[16].T = Vect(13.067840f, -0.000001f, 0.000001f);
	pTmp17->pBone[16].Q = Quat(ROT_XYZ, 0.082143f, -0.550107f, 0.193021f);
	pTmp17->pBone[16].S = Vect(1.000000f, 1.000000f, 1.000001f);

	//     Node Name: Bip01 R Forearm
	pTmp17->pBone[17].T = Vect(15.099410f, -0.000740f, -0.000069f);
	pTmp17->pBone[17].Q = Quat(ROT_XYZ, -0.697876f, -0.066506f, -0.479444f);
	pTmp17->pBone[17].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Hand
	pTmp17->pBone[18].T = Vect(15.099400f, 0.000180f, -0.000104f);
	pTmp17->pBone[18].Q = Quat(ROT_XYZ, 1.541847f, 0.080797f, -0.152182f);
	pTmp17->pBone[18].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Head
	pTmp17->pBone[19].T = Vect(11.079190f, 0.000001f, 0.000001f);
	pTmp17->pBone[19].Q = Quat(ROT_XYZ, -0.017838f, 0.016996f, 0.291346f);
	pTmp17->pBone[19].S = Vect(1.000000f, 1.000000f, 1.000000f);



	//------- Key Time: 18  ms: 600 ------------------ 

	Frame_Bucket *pTmp18 = new Frame_Bucket();
	pTmp18->prevBucket = pTmp17;
	pTmp18->nextBucket = 0;
	pTmp18->KeyTime = 18 * Time(TIME_NTSC_30_FRAME);
	pTmp18->pBone = new Bone[NUM_BONES];
	pTmp17->nextBucket = pTmp18;

	//     Node Name: RootNode
	//     Node Name: Bip01
	pTmp18->pBone[0].T = Vect(-0.092059f, -3.184256f, 4.939685f);
	pTmp18->pBone[0].Q = Quat(ROT_XYZ, -2.185850f, -1.479994f, 0.563697f);
	pTmp18->pBone[0].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Pelvis
	pTmp18->pBone[1].T = Vect(0.0f, 0.0f, 0.0f);
	pTmp18->pBone[1].Q = Quat(ROT_XYZ, 0.0f, 0.0f, 0.0f);
	pTmp18->pBone[1].S = Vect(1.0f, 1.0f, 1.0f);

	//     Node Name: Bip01 Spine
	pTmp18->pBone[2].T = Vect(6.016844f, -0.014115f, -0.000028f);
	pTmp18->pBone[2].Q = Quat(ROT_XYZ, 0.064386f, -0.038889f, 0.008216f);
	pTmp18->pBone[2].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Thigh
	pTmp18->pBone[3].T = Vect(-5.480478f, 0.957462f, 13.862060f);
	pTmp18->pBone[3].Q = Quat(ROT_XYZ, -3.088949f, -0.182595f, 2.270422f);
	pTmp18->pBone[3].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Calf
	pTmp18->pBone[4].T = Vect(16.961559f, -0.000002f, -0.000004f);
	pTmp18->pBone[4].Q = Quat(ROT_XYZ, 0.122574f, 0.000028f, -0.623738f);
	pTmp18->pBone[4].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Foot
	pTmp18->pBone[5].T = Vect(13.263550f, 0.000003f, 0.000002f);
	pTmp18->pBone[5].Q = Quat(ROT_XYZ, -0.005922f, -0.051081f, -0.058762f);
	pTmp18->pBone[5].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Thigh
	pTmp18->pBone[6].T = Vect(-6.543499f, -0.800528f, -13.403390f);
	pTmp18->pBone[6].Q = Quat(ROT_XYZ, -3.104314f, -0.123878f, -2.978805f);
	pTmp18->pBone[6].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Calf
	pTmp18->pBone[7].T = Vect(16.961571f, 0.000003f, -0.000003f);
	pTmp18->pBone[7].Q = Quat(ROT_XYZ, 0.010909f, 0.021284f, -0.639054f);
	pTmp18->pBone[7].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Foot
	pTmp18->pBone[8].T = Vect(13.263530f, -0.000000f, 0.000002f);
	pTmp18->pBone[8].Q = Quat(ROT_XYZ, 0.009948f, -0.121737f, 0.752274f);
	pTmp18->pBone[8].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Spine1
	pTmp18->pBone[9].T = Vect(17.730431f, -0.017629f, 0.000002f);
	pTmp18->pBone[9].Q = Quat(ROT_XYZ, 0.128638f, -0.078172f, 0.012332f);
	pTmp18->pBone[9].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Neck
	pTmp18->pBone[10].T = Vect(22.140881f, -0.008821f, 0.000003f);
	pTmp18->pBone[10].Q = Quat(ROT_XYZ, -0.122612f, 0.069778f, -0.098540f);
	pTmp18->pBone[10].S = Vect(1.000000f, 1.000000f, 1.000001f);

	//     Node Name: Bip01 L Clavicle
	pTmp18->pBone[11].T = Vect(-0.381636f, -0.657628f, 5.408282f);
	pTmp18->pBone[11].Q = Quat(ROT_XYZ, 0.284118f, -1.217732f, 2.897304f);
	pTmp18->pBone[11].S = Vect(0.999999f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L UpperArm
	pTmp18->pBone[12].T = Vect(13.067860f, 0.000004f, -0.000004f);
	pTmp18->pBone[12].Q = Quat(ROT_XYZ, 0.092269f, 0.941284f, 0.392740f);
	pTmp18->pBone[12].S = Vect(1.000000f, 1.000001f, 1.000001f);

	//     Node Name: Bip01 L Forearm
	pTmp18->pBone[13].T = Vect(15.099380f, -0.000409f, 0.000371f);
	pTmp18->pBone[13].Q = Quat(ROT_XYZ, 0.941164f, 0.016091f, -0.150097f);
	pTmp18->pBone[13].S = Vect(1.000000f, 1.000001f, 1.000001f);

	//     Node Name: Bip01 L Hand
	pTmp18->pBone[14].T = Vect(15.099410f, 0.000074f, 0.000066f);
	pTmp18->pBone[14].Q = Quat(ROT_XYZ, -1.587804f, 0.260786f, -0.061217f);
	pTmp18->pBone[14].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Clavicle
	pTmp18->pBone[15].T = Vect(0.379926f, 0.675072f, -5.406253f);
	pTmp18->pBone[15].Q = Quat(ROT_XYZ, -0.847410f, 0.817152f, 2.299880f);
	pTmp18->pBone[15].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R UpperArm
	pTmp18->pBone[16].T = Vect(13.067840f, -0.000001f, 0.000001f);
	pTmp18->pBone[16].Q = Quat(ROT_XYZ, 0.103958f, -0.530921f, 0.172576f);
	pTmp18->pBone[16].S = Vect(1.000000f, 1.000000f, 1.000001f);

	//     Node Name: Bip01 R Forearm
	pTmp18->pBone[17].T = Vect(15.099410f, -0.000739f, -0.000020f);
	pTmp18->pBone[17].Q = Quat(ROT_XYZ, -0.677078f, -0.068492f, -0.504560f);
	pTmp18->pBone[17].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Hand
	pTmp18->pBone[18].T = Vect(15.099400f, 0.000180f, -0.000106f);
	pTmp18->pBone[18].Q = Quat(ROT_XYZ, 1.545232f, 0.078656f, -0.132109f);
	pTmp18->pBone[18].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Head
	pTmp18->pBone[19].T = Vect(11.079190f, 0.000001f, 0.000001f);
	pTmp18->pBone[19].Q = Quat(ROT_XYZ, -0.023138f, 0.022954f, 0.297951f);
	pTmp18->pBone[19].S = Vect(1.000000f, 1.000000f, 1.000000f);



	//------- Key Time: 19  ms: 633 ------------------ 

	Frame_Bucket *pTmp19 = new Frame_Bucket();
	pTmp19->prevBucket = pTmp18;
	pTmp19->nextBucket = 0;
	pTmp19->KeyTime = 19 * Time(TIME_NTSC_30_FRAME);
	pTmp19->pBone = new Bone[NUM_BONES];
	pTmp18->nextBucket = pTmp19;

	//     Node Name: RootNode
	//     Node Name: Bip01
	pTmp19->pBone[0].T = Vect(-0.218599f, -3.364302f, 4.628236f);
	pTmp19->pBone[0].Q = Quat(ROT_XYZ, -2.052267f, -1.485817f, 0.427153f);
	pTmp19->pBone[0].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Pelvis
	pTmp19->pBone[1].T = Vect(0.0f, 0.0f, 0.0f);
	pTmp19->pBone[1].Q = Quat(ROT_XYZ, 0.0f, 0.0f, 0.0f);
	pTmp19->pBone[1].S = Vect(1.0f, 1.0f, 1.0f);

	//     Node Name: Bip01 Spine
	pTmp19->pBone[2].T = Vect(6.016924f, -0.014115f, -0.000028f);
	pTmp19->pBone[2].Q = Quat(ROT_XYZ, 0.064191f, -0.040870f, 0.004107f);
	pTmp19->pBone[2].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Thigh
	pTmp19->pBone[3].T = Vect(-5.453230f, 0.930776f, 13.874650f);
	pTmp19->pBone[3].Q = Quat(ROT_XYZ, -3.069178f, -0.168637f, 2.214684f);
	pTmp19->pBone[3].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Calf
	pTmp19->pBone[4].T = Vect(16.961559f, -0.000002f, -0.000004f);
	pTmp19->pBone[4].Q = Quat(ROT_XYZ, 0.085122f, 0.003311f, -0.497993f);
	pTmp19->pBone[4].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Foot
	pTmp19->pBone[5].T = Vect(13.263550f, 0.000003f, 0.000002f);
	pTmp19->pBone[5].Q = Quat(ROT_XYZ, -0.005221f, -0.058966f, -0.097547f);
	pTmp19->pBone[5].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Thigh
	pTmp19->pBone[6].T = Vect(-6.570345f, -0.821742f, -13.388990f);
	pTmp19->pBone[6].Q = Quat(ROT_XYZ, -3.116771f, -0.136108f, -2.843249f);
	pTmp19->pBone[6].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Calf
	pTmp19->pBone[7].T = Vect(16.961571f, 0.000003f, -0.000003f);
	pTmp19->pBone[7].Q = Quat(ROT_XYZ, 0.015058f, 0.015257f, -0.450106f);
	pTmp19->pBone[7].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Foot
	pTmp19->pBone[8].T = Vect(13.263530f, -0.000000f, 0.000002f);
	pTmp19->pBone[8].Q = Quat(ROT_XYZ, 0.014899f, -0.112836f, 0.706130f);
	pTmp19->pBone[8].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Spine1
	pTmp19->pBone[9].T = Vect(17.730431f, -0.017629f, 0.000002f);
	pTmp19->pBone[9].Q = Quat(ROT_XYZ, 0.128415f, -0.081865f, 0.003993f);
	pTmp19->pBone[9].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Neck
	pTmp19->pBone[10].T = Vect(22.140881f, -0.008821f, 0.000003f);
	pTmp19->pBone[10].Q = Quat(ROT_XYZ, -0.121652f, 0.080737f, -0.098624f);
	pTmp19->pBone[10].S = Vect(1.000000f, 1.000000f, 1.000001f);

	//     Node Name: Bip01 L Clavicle
	pTmp19->pBone[11].T = Vect(-0.441314f, -0.651891f, 5.404433f);
	pTmp19->pBone[11].Q = Quat(ROT_XYZ, 0.294820f, -1.235522f, 2.890885f);
	pTmp19->pBone[11].S = Vect(0.999999f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L UpperArm
	pTmp19->pBone[12].T = Vect(13.067860f, 0.000004f, -0.000004f);
	pTmp19->pBone[12].Q = Quat(ROT_XYZ, 0.130014f, 0.968885f, 0.441122f);
	pTmp19->pBone[12].S = Vect(1.000000f, 1.000001f, 1.000001f);

	//     Node Name: Bip01 L Forearm
	pTmp19->pBone[13].T = Vect(15.099380f, -0.000356f, 0.000371f);
	pTmp19->pBone[13].Q = Quat(ROT_XYZ, 0.914870f, 0.016899f, -0.147972f);
	pTmp19->pBone[13].S = Vect(1.000000f, 1.000001f, 1.000001f);

	//     Node Name: Bip01 L Hand
	pTmp19->pBone[14].T = Vect(15.099410f, 0.000073f, 0.000066f);
	pTmp19->pBone[14].Q = Quat(ROT_XYZ, -1.591473f, 0.284292f, -0.071789f);
	pTmp19->pBone[14].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Clavicle
	pTmp19->pBone[15].T = Vect(0.439605f, 0.669340f, -5.402444f);
	pTmp19->pBone[15].Q = Quat(ROT_XYZ, -0.860914f, 0.833368f, 2.290895f);
	pTmp19->pBone[15].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R UpperArm
	pTmp19->pBone[16].T = Vect(13.067840f, -0.000001f, 0.000001f);
	pTmp19->pBone[16].Q = Quat(ROT_XYZ, 0.120628f, -0.510994f, 0.153702f);
	pTmp19->pBone[16].S = Vect(1.000000f, 1.000000f, 1.000001f);

	//     Node Name: Bip01 R Forearm
	pTmp19->pBone[17].T = Vect(15.099410f, -0.000642f, -0.000001f);
	pTmp19->pBone[17].Q = Quat(ROT_XYZ, -0.669140f, -0.070595f, -0.534941f);
	pTmp19->pBone[17].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Hand
	pTmp19->pBone[18].T = Vect(15.099400f, 0.000180f, -0.000109f);
	pTmp19->pBone[18].Q = Quat(ROT_XYZ, 1.550136f, 0.061832f, -0.117195f);
	pTmp19->pBone[18].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Head
	pTmp19->pBone[19].T = Vect(11.079190f, 0.000001f, 0.000001f);
	pTmp19->pBone[19].Q = Quat(ROT_XYZ, -0.025067f, 0.015860f, 0.288459f);
	pTmp19->pBone[19].S = Vect(1.000000f, 1.000000f, 1.000000f);



	//------- Key Time: 20  ms: 666 ------------------ 

	Frame_Bucket *pTmp20 = new Frame_Bucket();
	pTmp20->prevBucket = pTmp19;
	pTmp20->nextBucket = 0;
	pTmp20->KeyTime = 20 * Time(TIME_NTSC_30_FRAME);
	pTmp20->pBone = new Bone[NUM_BONES];
	pTmp19->nextBucket = pTmp20;

	//     Node Name: RootNode
	//     Node Name: Bip01
	pTmp20->pBone[0].T = Vect(-0.614698f, -3.670427f, 3.907164f);
	pTmp20->pBone[0].Q = Quat(ROT_XYZ, -1.930801f, -1.493535f, 0.302426f);
	pTmp20->pBone[0].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Pelvis
	pTmp20->pBone[1].T = Vect(0.0f, 0.0f, 0.0f);
	pTmp20->pBone[1].Q = Quat(ROT_XYZ, 0.0f, 0.0f, 0.0f);
	pTmp20->pBone[1].S = Vect(1.0f, 1.0f, 1.0f);

	//     Node Name: Bip01 Spine
	pTmp20->pBone[2].T = Vect(6.016924f, -0.014115f, -0.000028f);
	pTmp20->pBone[2].Q = Quat(ROT_XYZ, 0.063097f, -0.041995f, 0.002954f);
	pTmp20->pBone[2].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Thigh
	pTmp20->pBone[3].T = Vect(-5.437526f, 0.909069f, 13.882210f);
	pTmp20->pBone[3].Q = Quat(ROT_XYZ, -3.062670f, -0.160113f, 2.147790f);
	pTmp20->pBone[3].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Calf
	pTmp20->pBone[4].T = Vect(16.961559f, -0.000002f, -0.000004f);
	pTmp20->pBone[4].Q = Quat(ROT_XYZ, 0.014384f, -0.001096f, -0.492625f);
	pTmp20->pBone[4].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Foot
	pTmp20->pBone[5].T = Vect(13.263550f, 0.000003f, 0.000002f);
	pTmp20->pBone[5].Q = Quat(ROT_XYZ, -0.010089f, -0.066347f, 0.043987f);
	pTmp20->pBone[5].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Thigh
	pTmp20->pBone[6].T = Vect(-6.585438f, -0.813554f, -13.382040f);
	pTmp20->pBone[6].Q = Quat(ROT_XYZ, -3.131204f, -0.145459f, -2.695383f);
	pTmp20->pBone[6].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Calf
	pTmp20->pBone[7].T = Vect(16.961571f, 0.000003f, -0.000003f);
	pTmp20->pBone[7].Q = Quat(ROT_XYZ, 0.010414f, 0.006832f, -0.204281f);
	pTmp20->pBone[7].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Foot
	pTmp20->pBone[8].T = Vect(13.263530f, -0.000000f, 0.000002f);
	pTmp20->pBone[8].Q = Quat(ROT_XYZ, 0.020800f, -0.105657f, 0.616206f);
	pTmp20->pBone[8].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Spine1
	pTmp20->pBone[9].T = Vect(17.730431f, -0.017629f, 0.000002f);
	pTmp20->pBone[9].Q = Quat(ROT_XYZ, 0.126279f, -0.084042f, 0.001660f);
	pTmp20->pBone[9].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Neck
	pTmp20->pBone[10].T = Vect(22.140881f, -0.008821f, 0.000003f);
	pTmp20->pBone[10].Q = Quat(ROT_XYZ, -0.112200f, 0.074877f, -0.115263f);
	pTmp20->pBone[10].S = Vect(1.000000f, 1.000000f, 1.000001f);

	//     Node Name: Bip01 L Clavicle
	pTmp20->pBone[11].T = Vect(-0.409564f, -0.601072f, 5.412821f);
	pTmp20->pBone[11].Q = Quat(ROT_XYZ, 0.343183f, -1.252708f, 2.864693f);
	pTmp20->pBone[11].S = Vect(0.999999f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L UpperArm
	pTmp20->pBone[12].T = Vect(13.067860f, 0.000004f, -0.000004f);
	pTmp20->pBone[12].Q = Quat(ROT_XYZ, 0.164857f, 0.982196f, 0.478227f);
	pTmp20->pBone[12].S = Vect(1.000000f, 1.000001f, 1.000001f);

	//     Node Name: Bip01 L Forearm
	pTmp20->pBone[13].T = Vect(15.099380f, -0.000355f, 0.000368f);
	pTmp20->pBone[13].Q = Quat(ROT_XYZ, 0.906923f, 0.017198f, -0.153696f);
	pTmp20->pBone[13].S = Vect(1.000000f, 1.000001f, 1.000001f);

	//     Node Name: Bip01 L Hand
	pTmp20->pBone[14].T = Vect(15.099410f, 0.000072f, 0.000065f);
	pTmp20->pBone[14].Q = Quat(ROT_XYZ, -1.595199f, 0.296832f, -0.102568f);
	pTmp20->pBone[14].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Clavicle
	pTmp20->pBone[15].T = Vect(0.407550f, 0.618508f, -5.411011f);
	pTmp20->pBone[15].Q = Quat(ROT_XYZ, -0.857393f, 0.844878f, 2.309828f);
	pTmp20->pBone[15].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R UpperArm
	pTmp20->pBone[16].T = Vect(13.067840f, -0.000001f, 0.000001f);
	pTmp20->pBone[16].Q = Quat(ROT_XYZ, 0.130433f, -0.491096f, 0.139351f);
	pTmp20->pBone[16].S = Vect(1.000000f, 1.000000f, 1.000001f);

	//     Node Name: Bip01 R Forearm
	pTmp20->pBone[17].T = Vect(15.099410f, -0.000745f, 0.000048f);
	pTmp20->pBone[17].Q = Quat(ROT_XYZ, -0.672164f, -0.074023f, -0.572750f);
	pTmp20->pBone[17].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Hand
	pTmp20->pBone[18].T = Vect(15.099400f, 0.000180f, -0.000115f);
	pTmp20->pBone[18].Q = Quat(ROT_XYZ, 1.553982f, 0.046080f, -0.109010f);
	pTmp20->pBone[18].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Head
	pTmp20->pBone[19].T = Vect(11.079190f, 0.000001f, 0.000001f);
	pTmp20->pBone[19].Q = Quat(ROT_XYZ, -0.025857f, 0.021111f, 0.281302f);
	pTmp20->pBone[19].S = Vect(1.000000f, 1.000000f, 1.000000f);



	//------- Key Time: 21  ms: 700 ------------------ 

	Frame_Bucket *pTmp21 = new Frame_Bucket();
	pTmp21->prevBucket = pTmp20;
	pTmp21->nextBucket = 0;
	pTmp21->KeyTime = 21 * Time(TIME_NTSC_30_FRAME);
	pTmp21->pBone = new Bone[NUM_BONES];
	pTmp20->nextBucket = pTmp21;

	//     Node Name: RootNode
	//     Node Name: Bip01
	pTmp21->pBone[0].T = Vect(-1.140201f, -4.368587f, 3.125632f);
	pTmp21->pBone[0].Q = Quat(ROT_XYZ, -1.874975f, -1.504992f, 0.242319f);
	pTmp21->pBone[0].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Pelvis
	pTmp21->pBone[1].T = Vect(0.0f, 0.0f, 0.0f);
	pTmp21->pBone[1].Q = Quat(ROT_XYZ, 0.0f, 0.0f, 0.0f);
	pTmp21->pBone[1].S = Vect(1.0f, 1.0f, 1.0f);

	//     Node Name: Bip01 Spine
	pTmp21->pBone[2].T = Vect(6.016923f, -0.014115f, -0.000028f);
	pTmp21->pBone[2].Q = Quat(ROT_XYZ, 0.062964f, -0.041653f, 0.005042f);
	pTmp21->pBone[2].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Thigh
	pTmp21->pBone[3].T = Vect(-5.442210f, 0.919641f, 13.879680f);
	pTmp21->pBone[3].Q = Quat(ROT_XYZ, -3.061709f, -0.160078f, 2.119689f);
	pTmp21->pBone[3].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Calf
	pTmp21->pBone[4].T = Vect(16.961559f, -0.000002f, -0.000004f);
	pTmp21->pBone[4].Q = Quat(ROT_XYZ, -0.071671f, -0.006606f, -0.488220f);
	pTmp21->pBone[4].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Foot
	pTmp21->pBone[5].T = Vect(13.263550f, 0.000003f, 0.000002f);
	pTmp21->pBone[5].Q = Quat(ROT_XYZ, -0.023852f, -0.125200f, 0.198024f);
	pTmp21->pBone[5].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Thigh
	pTmp21->pBone[6].T = Vect(-6.580740f, -0.799384f, -13.385200f);
	pTmp21->pBone[6].Q = Quat(ROT_XYZ, -3.132143f, -0.143152f, -2.636882f);
	pTmp21->pBone[6].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Calf
	pTmp21->pBone[7].T = Vect(16.961571f, 0.000003f, -0.000003f);
	pTmp21->pBone[7].Q = Quat(ROT_XYZ, -0.012265f, 0.004540f, -0.144249f);
	pTmp21->pBone[7].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Foot
	pTmp21->pBone[8].T = Vect(13.263530f, -0.000000f, 0.000002f);
	pTmp21->pBone[8].Q = Quat(ROT_XYZ, 0.020635f, -0.102361f, 0.621719f);
	pTmp21->pBone[8].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Spine1
	pTmp21->pBone[9].T = Vect(17.730431f, -0.017629f, 0.000002f);
	pTmp21->pBone[9].Q = Quat(ROT_XYZ, 0.125924f, -0.083488f, 0.005863f);
	pTmp21->pBone[9].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Neck
	pTmp21->pBone[10].T = Vect(22.140881f, -0.008821f, 0.000003f);
	pTmp21->pBone[10].Q = Quat(ROT_XYZ, -0.108951f, 0.077316f, -0.151539f);
	pTmp21->pBone[10].S = Vect(1.000000f, 1.000000f, 1.000001f);

	//     Node Name: Bip01 L Clavicle
	pTmp21->pBone[11].T = Vect(-0.423159f, -0.583414f, 5.413705f);
	pTmp21->pBone[11].Q = Quat(ROT_XYZ, 0.356248f, -1.249789f, 2.889005f);
	pTmp21->pBone[11].S = Vect(0.999999f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L UpperArm
	pTmp21->pBone[12].T = Vect(13.067860f, 0.000004f, -0.000004f);
	pTmp21->pBone[12].Q = Quat(ROT_XYZ, 0.184220f, 0.983079f, 0.492914f);
	pTmp21->pBone[12].S = Vect(1.000000f, 1.000001f, 1.000001f);

	//     Node Name: Bip01 L Forearm
	pTmp21->pBone[13].T = Vect(15.099380f, -0.000354f, 0.000362f);
	pTmp21->pBone[13].Q = Quat(ROT_XYZ, 0.915164f, 0.017118f, -0.165414f);
	pTmp21->pBone[13].S = Vect(1.000000f, 1.000001f, 1.000001f);

	//     Node Name: Bip01 L Hand
	pTmp21->pBone[14].T = Vect(15.099410f, 0.000071f, 0.000064f);
	pTmp21->pBone[14].Q = Quat(ROT_XYZ, -1.597644f, 0.303203f, -0.131531f);
	pTmp21->pBone[14].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Clavicle
	pTmp21->pBone[15].T = Vect(0.420523f, 0.600778f, -5.412013f);
	pTmp21->pBone[15].Q = Quat(ROT_XYZ, -0.881343f, 0.865289f, 2.325922f);
	pTmp21->pBone[15].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R UpperArm
	pTmp21->pBone[16].T = Vect(13.067840f, -0.000001f, 0.000001f);
	pTmp21->pBone[16].Q = Quat(ROT_XYZ, 0.138615f, -0.472784f, 0.121624f);
	pTmp21->pBone[16].S = Vect(1.000000f, 1.000000f, 1.000001f);

	//     Node Name: Bip01 R Forearm
	pTmp21->pBone[17].T = Vect(15.099410f, -0.000762f, 0.000092f);
	pTmp21->pBone[17].Q = Quat(ROT_XYZ, -0.684260f, -0.077122f, -0.612987f);
	pTmp21->pBone[17].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Hand
	pTmp21->pBone[18].T = Vect(15.099400f, 0.000180f, -0.000121f);
	pTmp21->pBone[18].Q = Quat(ROT_XYZ, 1.557138f, 0.028301f, -0.107590f);
	pTmp21->pBone[18].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Head
	pTmp21->pBone[19].T = Vect(11.079190f, 0.000001f, 0.000001f);
	pTmp21->pBone[19].Q = Quat(ROT_XYZ, -0.026672f, 0.000720f, 0.296310f);
	pTmp21->pBone[19].S = Vect(1.000000f, 1.000000f, 1.000000f);



	//------- Key Time: 22  ms: 733 ------------------ 

	Frame_Bucket *pTmp22 = new Frame_Bucket();
	pTmp22->prevBucket = pTmp21;
	pTmp22->nextBucket = 0;
	pTmp22->KeyTime = 22 * Time(TIME_NTSC_30_FRAME);
	pTmp22->pBone = new Bone[NUM_BONES];
	pTmp21->nextBucket = pTmp22;

	//     Node Name: RootNode
	//     Node Name: Bip01
	pTmp22->pBone[0].T = Vect(-1.979282f, -5.105843f, 1.970479f);
	pTmp22->pBone[0].Q = Quat(ROT_XYZ, -1.769162f, -1.512045f, 0.130849f);
	pTmp22->pBone[0].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Pelvis
	pTmp22->pBone[1].T = Vect(0.0f, 0.0f, 0.0f);
	pTmp22->pBone[1].Q = Quat(ROT_XYZ, 0.0f, 0.0f, 0.0f);
	pTmp22->pBone[1].S = Vect(1.0f, 1.0f, 1.0f);

	//     Node Name: Bip01 Spine
	pTmp22->pBone[2].T = Vect(6.016923f, -0.014115f, -0.000028f);
	pTmp22->pBone[2].Q = Quat(ROT_XYZ, 0.061727f, -0.041379f, 0.006374f);
	pTmp22->pBone[2].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Thigh
	pTmp22->pBone[3].T = Vect(-5.445940f, 0.910374f, 13.878820f);
	pTmp22->pBone[3].Q = Quat(ROT_XYZ, -3.064981f, -0.146023f, 2.087418f);
	pTmp22->pBone[3].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Calf
	pTmp22->pBone[4].T = Vect(16.961559f, -0.000002f, -0.000004f);
	pTmp22->pBone[4].Q = Quat(ROT_XYZ, -0.131131f, -0.010444f, -0.516538f);
	pTmp22->pBone[4].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Foot
	pTmp22->pBone[5].T = Vect(13.263550f, 0.000003f, 0.000002f);
	pTmp22->pBone[5].Q = Quat(ROT_XYZ, -0.011856f, -0.061018f, 0.158705f);
	pTmp22->pBone[5].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Thigh
	pTmp22->pBone[6].T = Vect(-6.576995f, -0.774926f, -13.388470f);
	pTmp22->pBone[6].Q = Quat(ROT_XYZ, -3.137564f, -0.146864f, -2.610955f);
	pTmp22->pBone[6].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Calf
	pTmp22->pBone[7].T = Vect(16.961571f, 0.000003f, -0.000003f);
	pTmp22->pBone[7].Q = Quat(ROT_XYZ, -0.020936f, 0.004578f, -0.144882f);
	pTmp22->pBone[7].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Foot
	pTmp22->pBone[8].T = Vect(13.263530f, -0.000000f, 0.000002f);
	pTmp22->pBone[8].Q = Quat(ROT_XYZ, 0.017442f, -0.101361f, 0.655996f);
	pTmp22->pBone[8].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Spine1
	pTmp22->pBone[9].T = Vect(17.730431f, -0.017629f, 0.000002f);
	pTmp22->pBone[9].Q = Quat(ROT_XYZ, 0.123389f, -0.083022f, 0.008599f);
	pTmp22->pBone[9].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Neck
	pTmp22->pBone[10].T = Vect(22.140881f, -0.008821f, 0.000003f);
	pTmp22->pBone[10].Q = Quat(ROT_XYZ, -0.103329f, 0.079473f, -0.174262f);
	pTmp22->pBone[10].S = Vect(1.000000f, 1.000000f, 1.000001f);

	//     Node Name: Bip01 L Clavicle
	pTmp22->pBone[11].T = Vect(-0.435095f, -0.552903f, 5.415963f);
	pTmp22->pBone[11].Q = Quat(ROT_XYZ, 0.376007f, -1.246196f, 2.893065f);
	pTmp22->pBone[11].S = Vect(0.999999f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L UpperArm
	pTmp22->pBone[12].T = Vect(13.067860f, 0.000004f, -0.000004f);
	pTmp22->pBone[12].Q = Quat(ROT_XYZ, 0.189384f, 0.977192f, 0.494004f);
	pTmp22->pBone[12].S = Vect(1.000000f, 1.000001f, 1.000001f);

	//     Node Name: Bip01 L Forearm
	pTmp22->pBone[13].T = Vect(15.099380f, -0.000354f, 0.000356f);
	pTmp22->pBone[13].Q = Quat(ROT_XYZ, 0.920569f, 0.017072f, -0.182333f);
	pTmp22->pBone[13].S = Vect(1.000000f, 1.000001f, 1.000001f);

	//     Node Name: Bip01 L Hand
	pTmp22->pBone[14].T = Vect(15.099410f, 0.000070f, 0.000063f);
	pTmp22->pBone[14].Q = Quat(ROT_XYZ, -1.600886f, 0.312608f, -0.161056f);
	pTmp22->pBone[14].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Clavicle
	pTmp22->pBone[15].T = Vect(0.432065f, 0.570216f, -5.414411f);
	pTmp22->pBone[15].Q = Quat(ROT_XYZ, -0.891103f, 0.881644f, 2.340570f);
	pTmp22->pBone[15].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R UpperArm
	pTmp22->pBone[16].T = Vect(13.067840f, -0.000001f, 0.000001f);
	pTmp22->pBone[16].Q = Quat(ROT_XYZ, 0.144630f, -0.454905f, 0.111358f);
	pTmp22->pBone[16].S = Vect(1.000000f, 1.000000f, 1.000001f);

	//     Node Name: Bip01 R Forearm
	pTmp22->pBone[17].T = Vect(15.099410f, -0.000750f, 0.000113f);
	pTmp22->pBone[17].Q = Quat(ROT_XYZ, -0.690219f, -0.080184f, -0.650515f);
	pTmp22->pBone[17].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Hand
	pTmp22->pBone[18].T = Vect(15.099400f, 0.000180f, -0.000127f);
	pTmp22->pBone[18].Q = Quat(ROT_XYZ, 1.558862f, 0.017165f, -0.105527f);
	pTmp22->pBone[18].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Head
	pTmp22->pBone[19].T = Vect(11.079190f, 0.000001f, 0.000001f);
	pTmp22->pBone[19].Q = Quat(ROT_XYZ, -0.026153f, -0.005701f, 0.293729f);
	pTmp22->pBone[19].S = Vect(1.000000f, 1.000000f, 1.000000f);



	//------- Key Time: 23  ms: 766 ------------------ 

	Frame_Bucket *pTmp23 = new Frame_Bucket();
	pTmp23->prevBucket = pTmp22;
	pTmp23->nextBucket = 0;
	pTmp23->KeyTime = 23 * Time(TIME_NTSC_30_FRAME);
	pTmp23->pBone = new Bone[NUM_BONES];
	pTmp22->nextBucket = pTmp23;

	//     Node Name: RootNode
	//     Node Name: Bip01
	pTmp23->pBone[0].T = Vect(-3.049559f, -5.927010f, 0.280116f);
	pTmp23->pBone[0].Q = Quat(ROT_XYZ, -1.549875f, -1.529267f, -0.097096f);
	pTmp23->pBone[0].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Pelvis
	pTmp23->pBone[1].T = Vect(0.0f, 0.0f, 0.0f);
	pTmp23->pBone[1].Q = Quat(ROT_XYZ, 0.0f, 0.0f, 0.0f);
	pTmp23->pBone[1].S = Vect(1.0f, 1.0f, 1.0f);

	//     Node Name: Bip01 Spine
	pTmp23->pBone[2].T = Vect(6.016922f, -0.014115f, 0.000008f);
	pTmp23->pBone[2].Q = Quat(ROT_XYZ, 0.062893f, -0.040589f, 0.005055f);
	pTmp23->pBone[2].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Thigh
	pTmp23->pBone[3].T = Vect(-5.457065f, 0.918366f, 13.873950f);
	pTmp23->pBone[3].Q = Quat(ROT_XYZ, -3.067668f, -0.138235f, 2.070999f);
	pTmp23->pBone[3].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Calf
	pTmp23->pBone[4].T = Vect(16.961559f, -0.000002f, -0.000004f);
	pTmp23->pBone[4].Q = Quat(ROT_XYZ, -0.160511f, -0.012511f, -0.551269f);
	pTmp23->pBone[4].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Foot
	pTmp23->pBone[5].T = Vect(13.263550f, 0.000003f, 0.000002f);
	pTmp23->pBone[5].Q = Quat(ROT_XYZ, -0.000698f, 0.020089f, 0.085328f);
	pTmp23->pBone[5].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Thigh
	pTmp23->pBone[6].T = Vect(-6.566516f, -0.798782f, -13.392240f);
	pTmp23->pBone[6].Q = Quat(ROT_XYZ, -3.138578f, -0.145838f, -2.591424f);
	pTmp23->pBone[6].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Calf
	pTmp23->pBone[7].T = Vect(16.961571f, 0.000003f, -0.000003f);
	pTmp23->pBone[7].Q = Quat(ROT_XYZ, -0.041489f, 0.004642f, -0.146461f);
	pTmp23->pBone[7].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Foot
	pTmp23->pBone[8].T = Vect(13.263530f, -0.000000f, 0.000002f);
	pTmp23->pBone[8].Q = Quat(ROT_XYZ, 0.015231f, -0.100114f, 0.685615f);
	pTmp23->pBone[8].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Spine1
	pTmp23->pBone[9].T = Vect(17.730431f, -0.017629f, 0.000002f);
	pTmp23->pBone[9].Q = Quat(ROT_XYZ, 0.125776f, -0.081362f, 0.005961f);
	pTmp23->pBone[9].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Neck
	pTmp23->pBone[10].T = Vect(22.140881f, -0.008821f, 0.000003f);
	pTmp23->pBone[10].Q = Quat(ROT_XYZ, -0.096025f, 0.082844f, -0.187153f);
	pTmp23->pBone[10].S = Vect(1.000000f, 1.000000f, 1.000001f);

	//     Node Name: Bip01 L Clavicle
	pTmp23->pBone[11].T = Vect(-0.453547f, -0.513202f, 5.418349f);
	pTmp23->pBone[11].Q = Quat(ROT_XYZ, 0.389701f, -1.243659f, 2.894011f);
	pTmp23->pBone[11].S = Vect(0.999999f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L UpperArm
	pTmp23->pBone[12].T = Vect(13.067860f, 0.000004f, -0.000004f);
	pTmp23->pBone[12].Q = Quat(ROT_XYZ, 0.181522f, 0.973794f, 0.487862f);
	pTmp23->pBone[12].S = Vect(1.000000f, 1.000001f, 1.000001f);

	//     Node Name: Bip01 L Forearm
	pTmp23->pBone[13].T = Vect(15.099380f, -0.000354f, 0.000353f);
	pTmp23->pBone[13].Q = Quat(ROT_XYZ, 0.917091f, 0.017196f, -0.194230f);
	pTmp23->pBone[13].S = Vect(1.000000f, 1.000001f, 1.000001f);

	//     Node Name: Bip01 L Hand
	pTmp23->pBone[14].T = Vect(15.099410f, 0.000068f, 0.000062f);
	pTmp23->pBone[14].Q = Quat(ROT_XYZ, -1.597670f, 0.294622f, -0.158947f);
	pTmp23->pBone[14].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Clavicle
	pTmp23->pBone[15].T = Vect(0.450312f, 0.530490f, -5.416964f);
	pTmp23->pBone[15].Q = Quat(ROT_XYZ, -0.890260f, 0.888222f, 2.352213f);
	pTmp23->pBone[15].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R UpperArm
	pTmp23->pBone[16].T = Vect(13.067840f, -0.000001f, 0.000001f);
	pTmp23->pBone[16].Q = Quat(ROT_XYZ, 0.144068f, -0.442443f, 0.115295f);
	pTmp23->pBone[16].S = Vect(1.000000f, 1.000000f, 1.000001f);

	//     Node Name: Bip01 R Forearm
	pTmp23->pBone[17].T = Vect(15.099410f, -0.000646f, 0.000125f);
	pTmp23->pBone[17].Q = Quat(ROT_XYZ, -0.693065f, -0.083399f, -0.675069f);
	pTmp23->pBone[17].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Hand
	pTmp23->pBone[18].T = Vect(15.099400f, 0.000180f, -0.000133f);
	pTmp23->pBone[18].Q = Quat(ROT_XYZ, 1.559383f, 0.017259f, -0.106353f);
	pTmp23->pBone[18].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Head
	pTmp23->pBone[19].T = Vect(11.079190f, 0.000001f, 0.000001f);
	pTmp23->pBone[19].Q = Quat(ROT_XYZ, -0.022494f, 0.000463f, 0.290053f);
	pTmp23->pBone[19].S = Vect(1.000000f, 1.000000f, 1.000000f);



	//------- Key Time: 24  ms: 800 ------------------ 

	Frame_Bucket *pTmp24 = new Frame_Bucket();
	pTmp24->prevBucket = pTmp23;
	pTmp24->nextBucket = 0;
	pTmp24->KeyTime = 24 * Time(TIME_NTSC_30_FRAME);
	pTmp24->pBone = new Bone[NUM_BONES];
	pTmp23->nextBucket = pTmp24;

	//     Node Name: RootNode
	//     Node Name: Bip01
	pTmp24->pBone[0].T = Vect(-3.743819f, -6.739059f, -1.275618f);
	pTmp24->pBone[0].Q = Quat(ROT_XYZ, -1.028456f, -1.557629f, -0.621985f);
	pTmp24->pBone[0].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Pelvis
	pTmp24->pBone[1].T = Vect(0.0f, 0.0f, 0.0f);
	pTmp24->pBone[1].Q = Quat(ROT_XYZ, 0.0f, 0.0f, 0.0f);
	pTmp24->pBone[1].S = Vect(1.0f, 1.0f, 1.0f);

	//     Node Name: Bip01 Spine
	pTmp24->pBone[2].T = Vect(6.016922f, -0.014115f, 0.000333f);
	pTmp24->pBone[2].Q = Quat(ROT_XYZ, 0.066804f, -0.043143f, 0.003174f);
	pTmp24->pBone[2].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Thigh
	pTmp24->pBone[3].T = Vect(-5.421480f, 0.962259f, 13.884940f);
	pTmp24->pBone[3].Q = Quat(ROT_XYZ, -3.069458f, -0.147156f, 2.067063f);
	pTmp24->pBone[3].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Calf
	pTmp24->pBone[4].T = Vect(16.961559f, -0.000002f, -0.000004f);
	pTmp24->pBone[4].Q = Quat(ROT_XYZ, -0.156954f, -0.020022f, -0.668424f);
	pTmp24->pBone[4].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Foot
	pTmp24->pBone[5].T = Vect(13.263550f, 0.000003f, 0.000002f);
	pTmp24->pBone[5].Q = Quat(ROT_XYZ, 0.002938f, 0.009157f, 0.151026f);
	pTmp24->pBone[5].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Thigh
	pTmp24->pBone[6].T = Vect(-6.601032f, -0.861319f, -13.371400f);
	pTmp24->pBone[6].Q = Quat(ROT_XYZ, -3.118077f, -0.128800f, -2.566092f);
	pTmp24->pBone[6].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Calf
	pTmp24->pBone[7].T = Vect(16.961571f, 0.000003f, -0.000003f);
	pTmp24->pBone[7].Q = Quat(ROT_XYZ, -0.090759f, 0.003732f, -0.146546f);
	pTmp24->pBone[7].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Foot
	pTmp24->pBone[8].T = Vect(13.263530f, -0.000000f, 0.000002f);
	pTmp24->pBone[8].Q = Quat(ROT_XYZ, 0.018762f, -0.089728f, 0.673997f);
	pTmp24->pBone[8].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Spine1
	pTmp24->pBone[9].T = Vect(17.730431f, -0.017629f, 0.000002f);
	pTmp24->pBone[9].Q = Quat(ROT_XYZ, 0.133700f, -0.086369f, 0.001866f);
	pTmp24->pBone[9].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Neck
	pTmp24->pBone[10].T = Vect(22.140881f, -0.008821f, 0.000003f);
	pTmp24->pBone[10].Q = Quat(ROT_XYZ, -0.084282f, 0.080095f, -0.189376f);
	pTmp24->pBone[10].S = Vect(1.000000f, 1.000000f, 1.000001f);

	//     Node Name: Bip01 L Clavicle
	pTmp24->pBone[11].T = Vect(-0.438620f, -0.449652f, 5.425226f);
	pTmp24->pBone[11].Q = Quat(ROT_XYZ, 0.409685f, -1.246243f, 2.879596f);
	pTmp24->pBone[11].S = Vect(0.999999f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L UpperArm
	pTmp24->pBone[12].T = Vect(13.067860f, 0.000004f, -0.000004f);
	pTmp24->pBone[12].Q = Quat(ROT_XYZ, 0.167031f, 0.967018f, 0.482022f);
	pTmp24->pBone[12].S = Vect(1.000000f, 1.000001f, 1.000001f);

	//     Node Name: Bip01 L Forearm
	pTmp24->pBone[13].T = Vect(15.099380f, -0.000353f, 0.000360f);
	pTmp24->pBone[13].Q = Quat(ROT_XYZ, 0.886501f, 0.017547f, -0.197155f);
	pTmp24->pBone[13].S = Vect(1.000000f, 1.000001f, 1.000001f);

	//     Node Name: Bip01 L Hand
	pTmp24->pBone[14].T = Vect(15.099410f, 0.000067f, 0.000062f);
	pTmp24->pBone[14].Q = Quat(ROT_XYZ, -1.592454f, 0.262924f, -0.142625f);
	pTmp24->pBone[14].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Clavicle
	pTmp24->pBone[15].T = Vect(0.435304f, 0.466942f, -5.424031f);
	pTmp24->pBone[15].Q = Quat(ROT_XYZ, -0.863397f, 0.884046f, 2.374102f);
	pTmp24->pBone[15].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R UpperArm
	pTmp24->pBone[16].T = Vect(13.067840f, -0.000001f, 0.000001f);
	pTmp24->pBone[16].Q = Quat(ROT_XYZ, 0.136047f, -0.448393f, 0.125806f);
	pTmp24->pBone[16].S = Vect(1.000000f, 1.000000f, 1.000001f);

	//     Node Name: Bip01 R Forearm
	pTmp24->pBone[17].T = Vect(15.099410f, -0.000693f, 0.000129f);
	pTmp24->pBone[17].Q = Quat(ROT_XYZ, -0.693308f, -0.085138f, -0.673253f);
	pTmp24->pBone[17].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Hand
	pTmp24->pBone[18].T = Vect(15.099400f, 0.000180f, -0.000139f);
	pTmp24->pBone[18].Q = Quat(ROT_XYZ, 1.558306f, 0.030794f, -0.110765f);
	pTmp24->pBone[18].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Head
	pTmp24->pBone[19].T = Vect(11.079190f, 0.000001f, 0.000001f);
	pTmp24->pBone[19].Q = Quat(ROT_XYZ, -0.015154f, 0.036809f, 0.286704f);
	pTmp24->pBone[19].S = Vect(1.000000f, 1.000000f, 1.000000f);



	//------- Key Time: 25  ms: 833 ------------------ 

	Frame_Bucket *pTmp25 = new Frame_Bucket();
	pTmp25->prevBucket = pTmp24;
	pTmp25->nextBucket = 0;
	pTmp25->KeyTime = 25 * Time(TIME_NTSC_30_FRAME);
	pTmp25->pBone = new Bone[NUM_BONES];
	pTmp24->nextBucket = pTmp25;

	//     Node Name: RootNode
	//     Node Name: Bip01
	pTmp25->pBone[0].T = Vect(-3.933082f, -7.007935f, -2.309389f);
	pTmp25->pBone[0].Q = Quat(ROT_XYZ, 1.280330f, -1.545893f, -2.926466f);
	pTmp25->pBone[0].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Pelvis
	pTmp25->pBone[1].T = Vect(0.0f, 0.0f, 0.0f);
	pTmp25->pBone[1].Q = Quat(ROT_XYZ, 0.0f, 0.0f, 0.0f);
	pTmp25->pBone[1].S = Vect(1.0f, 1.0f, 1.0f);

	//     Node Name: Bip01 Spine
	pTmp25->pBone[2].T = Vect(6.016921f, -0.014115f, -0.000009f);
	pTmp25->pBone[2].Q = Quat(ROT_XYZ, 0.074261f, -0.050080f, -0.001777f);
	pTmp25->pBone[2].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Thigh
	pTmp25->pBone[3].T = Vect(-5.325019f, 1.038821f, 13.916690f);
	pTmp25->pBone[3].Q = Quat(ROT_XYZ, -3.065522f, -0.146367f, 2.089662f);
	pTmp25->pBone[3].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Calf
	pTmp25->pBone[4].T = Vect(16.961559f, -0.000002f, -0.000004f);
	pTmp25->pBone[4].Q = Quat(ROT_XYZ, -0.110112f, -0.020986f, -0.721712f);
	pTmp25->pBone[4].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Foot
	pTmp25->pBone[5].T = Vect(13.263550f, 0.000003f, 0.000002f);
	pTmp25->pBone[5].Q = Quat(ROT_XYZ, 0.012557f, 0.069002f, 0.099093f);
	pTmp25->pBone[5].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Thigh
	pTmp25->pBone[6].T = Vect(-6.693723f, -0.987298f, -13.316480f);
	pTmp25->pBone[6].Q = Quat(ROT_XYZ, -3.080180f, -0.101254f, -2.535433f);
	pTmp25->pBone[6].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Calf
	pTmp25->pBone[7].T = Vect(16.961571f, 0.000003f, -0.000003f);
	pTmp25->pBone[7].Q = Quat(ROT_XYZ, -0.150176f, 0.002680f, -0.143823f);
	pTmp25->pBone[7].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Foot
	pTmp25->pBone[8].T = Vect(13.263530f, -0.000000f, 0.000002f);
	pTmp25->pBone[8].Q = Quat(ROT_XYZ, 0.027915f, -0.063864f, 0.589564f);
	pTmp25->pBone[8].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Spine1
	pTmp25->pBone[9].T = Vect(17.730431f, -0.017629f, 0.000002f);
	pTmp25->pBone[9].Q = Quat(ROT_XYZ, 0.148938f, -0.099827f, -0.008883f);
	pTmp25->pBone[9].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Neck
	pTmp25->pBone[10].T = Vect(22.140881f, -0.008821f, 0.000003f);
	pTmp25->pBone[10].Q = Quat(ROT_XYZ, -0.089131f, 0.091635f, -0.187284f);
	pTmp25->pBone[10].S = Vect(1.000000f, 1.000000f, 1.000001f);

	//     Node Name: Bip01 L Clavicle
	pTmp25->pBone[11].T = Vect(-0.501389f, -0.475465f, 5.417585f);
	pTmp25->pBone[11].Q = Quat(ROT_XYZ, 0.375337f, -1.232653f, 2.909157f);
	pTmp25->pBone[11].S = Vect(0.999999f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L UpperArm
	pTmp25->pBone[12].T = Vect(13.067860f, 0.000004f, -0.000004f);
	pTmp25->pBone[12].Q = Quat(ROT_XYZ, 0.146759f, 0.954691f, 0.470336f);
	pTmp25->pBone[12].S = Vect(1.000000f, 1.000001f, 1.000001f);

	//     Node Name: Bip01 L Forearm
	pTmp25->pBone[13].T = Vect(15.099380f, -0.000353f, 0.000341f);
	pTmp25->pBone[13].Q = Quat(ROT_XYZ, 0.847603f, 0.017328f, -0.180011f);
	pTmp25->pBone[13].S = Vect(1.000000f, 1.000001f, 1.000001f);

	//     Node Name: Bip01 L Hand
	pTmp25->pBone[14].T = Vect(15.099410f, 0.000066f, 0.000061f);
	pTmp25->pBone[14].Q = Quat(ROT_XYZ, -1.587467f, 0.221124f, -0.131486f);
	pTmp25->pBone[14].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Clavicle
	pTmp25->pBone[15].T = Vect(0.498123f, 0.492760f, -5.416338f);
	pTmp25->pBone[15].Q = Quat(ROT_XYZ, -0.856756f, 0.876097f, 2.376988f);
	pTmp25->pBone[15].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R UpperArm
	pTmp25->pBone[16].T = Vect(13.067840f, -0.000001f, 0.000001f);
	pTmp25->pBone[16].Q = Quat(ROT_XYZ, 0.117400f, -0.481102f, 0.141720f);
	pTmp25->pBone[16].S = Vect(1.000000f, 1.000000f, 1.000001f);

	//     Node Name: Bip01 R Forearm
	pTmp25->pBone[17].T = Vect(15.099410f, -0.000800f, 0.000081f);
	pTmp25->pBone[17].Q = Quat(ROT_XYZ, -0.721484f, -0.081742f, -0.612865f);
	pTmp25->pBone[17].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Hand
	pTmp25->pBone[18].T = Vect(15.099400f, 0.000180f, -0.000142f);
	pTmp25->pBone[18].Q = Quat(ROT_XYZ, 1.552222f, 0.059357f, -0.142333f);
	pTmp25->pBone[18].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Head
	pTmp25->pBone[19].T = Vect(11.079190f, 0.000001f, 0.000001f);
	pTmp25->pBone[19].Q = Quat(ROT_XYZ, -0.002284f, 0.058816f, 0.296891f);
	pTmp25->pBone[19].S = Vect(1.000000f, 1.000000f, 1.000000f);



	//------- Key Time: 26  ms: 866 ------------------ 

	Frame_Bucket *pTmp26 = new Frame_Bucket();
	pTmp26->prevBucket = pTmp25;
	pTmp26->nextBucket = 0;
	pTmp26->KeyTime = 26 * Time(TIME_NTSC_30_FRAME);
	pTmp26->pBone = new Bone[NUM_BONES];
	pTmp25->nextBucket = pTmp26;

	//     Node Name: RootNode
	//     Node Name: Bip01
	pTmp26->pBone[0].T = Vect(-3.293897f, -7.202480f, -3.182208f);
	pTmp26->pBone[0].Q = Quat(ROT_XYZ, 1.343781f, -1.525519f, -2.976460f);
	pTmp26->pBone[0].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Pelvis
	pTmp26->pBone[1].T = Vect(0.0f, 0.0f, 0.0f);
	pTmp26->pBone[1].Q = Quat(ROT_XYZ, 0.0f, 0.0f, 0.0f);
	pTmp26->pBone[1].S = Vect(1.0f, 1.0f, 1.0f);

	//     Node Name: Bip01 Spine
	pTmp26->pBone[2].T = Vect(6.016920f, -0.014115f, 0.000003f);
	pTmp26->pBone[2].Q = Quat(ROT_XYZ, 0.075160f, -0.055528f, -0.010024f);
	pTmp26->pBone[2].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Thigh
	pTmp26->pBone[3].T = Vect(-5.248755f, 1.004023f, 13.948200f);
	pTmp26->pBone[3].Q = Quat(ROT_XYZ, -3.057271f, -0.134966f, 2.120233f);
	pTmp26->pBone[3].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Calf
	pTmp26->pBone[4].T = Vect(16.961559f, -0.000002f, -0.000004f);
	pTmp26->pBone[4].Q = Quat(ROT_XYZ, -0.063720f, -0.014033f, -0.692927f);
	pTmp26->pBone[4].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Foot
	pTmp26->pBone[5].T = Vect(13.263550f, 0.000003f, 0.000002f);
	pTmp26->pBone[5].Q = Quat(ROT_XYZ, 0.010139f, 0.086055f, -0.076709f);
	pTmp26->pBone[5].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Thigh
	pTmp26->pBone[6].T = Vect(-6.766216f, -1.046009f, -13.275310f);
	pTmp26->pBone[6].Q = Quat(ROT_XYZ, -3.038923f, -0.055008f, -2.599965f);
	pTmp26->pBone[6].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Calf
	pTmp26->pBone[7].T = Vect(16.961571f, 0.000003f, -0.000003f);
	pTmp26->pBone[7].Q = Quat(ROT_XYZ, -0.193556f, 0.007362f, -0.374995f);
	pTmp26->pBone[7].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Foot
	pTmp26->pBone[8].T = Vect(13.263530f, -0.000000f, 0.000002f);
	pTmp26->pBone[8].Q = Quat(ROT_XYZ, 0.014553f, -0.077084f, 0.608519f);
	pTmp26->pBone[8].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Spine1
	pTmp26->pBone[9].T = Vect(17.730431f, -0.017629f, 0.000002f);
	pTmp26->pBone[9].Q = Quat(ROT_XYZ, 0.151272f, -0.110078f, -0.025853f);
	pTmp26->pBone[9].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Neck
	pTmp26->pBone[10].T = Vect(22.140881f, -0.008821f, 0.000003f);
	pTmp26->pBone[10].Q = Quat(ROT_XYZ, -0.098159f, 0.097909f, -0.180817f);
	pTmp26->pBone[10].S = Vect(1.000000f, 1.000000f, 1.000001f);

	//     Node Name: Bip01 L Clavicle
	pTmp26->pBone[11].T = Vect(-0.535437f, -0.524028f, 5.409837f);
	pTmp26->pBone[11].Q = Quat(ROT_XYZ, 0.310325f, -1.218149f, 2.964433f);
	pTmp26->pBone[11].S = Vect(0.999999f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L UpperArm
	pTmp26->pBone[12].T = Vect(13.067860f, 0.000004f, -0.000004f);
	pTmp26->pBone[12].Q = Quat(ROT_XYZ, 0.130443f, 0.936070f, 0.463939f);
	pTmp26->pBone[12].S = Vect(1.000000f, 1.000001f, 1.000001f);

	//     Node Name: Bip01 L Forearm
	pTmp26->pBone[13].T = Vect(15.099380f, -0.000360f, 0.000298f);
	pTmp26->pBone[13].Q = Quat(ROT_XYZ, 0.829380f, 0.017057f, -0.169846f);
	pTmp26->pBone[13].S = Vect(1.000000f, 1.000001f, 1.000001f);

	//     Node Name: Bip01 L Hand
	pTmp26->pBone[14].T = Vect(15.099410f, 0.000065f, 0.000060f);
	pTmp26->pBone[14].Q = Quat(ROT_XYZ, -1.586627f, 0.209622f, -0.121292f);
	pTmp26->pBone[14].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Clavicle
	pTmp26->pBone[15].T = Vect(0.532298f, 0.541334f, -5.408445f);
	pTmp26->pBone[15].Q = Quat(ROT_XYZ, -0.843307f, 0.868021f, 2.385017f);
	pTmp26->pBone[15].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R UpperArm
	pTmp26->pBone[16].T = Vect(13.067840f, -0.000001f, 0.000001f);
	pTmp26->pBone[16].Q = Quat(ROT_XYZ, 0.096389f, -0.519648f, 0.165982f);
	pTmp26->pBone[16].S = Vect(1.000000f, 1.000000f, 1.000001f);

	//     Node Name: Bip01 R Forearm
	pTmp26->pBone[17].T = Vect(15.099410f, -0.000744f, -0.000022f);
	pTmp26->pBone[17].Q = Quat(ROT_XYZ, -0.760470f, -0.075961f, -0.545446f);
	pTmp26->pBone[17].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Hand
	pTmp26->pBone[18].T = Vect(15.099400f, 0.000180f, -0.000144f);
	pTmp26->pBone[18].Q = Quat(ROT_XYZ, 1.543471f, 0.099810f, -0.160051f);
	pTmp26->pBone[18].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Head
	pTmp26->pBone[19].T = Vect(11.079190f, 0.000001f, 0.000001f);
	pTmp26->pBone[19].Q = Quat(ROT_XYZ, 0.008114f, 0.072747f, 0.311551f);
	pTmp26->pBone[19].S = Vect(1.000000f, 1.000000f, 1.000000f);



	//------- Key Time: 27  ms: 900 ------------------ 

	Frame_Bucket *pTmp27 = new Frame_Bucket();
	pTmp27->prevBucket = pTmp26;
	pTmp27->nextBucket = 0;
	pTmp27->KeyTime = 27 * Time(TIME_NTSC_30_FRAME);
	pTmp27->pBone = new Bone[NUM_BONES];
	pTmp26->nextBucket = pTmp27;

	//     Node Name: RootNode
	//     Node Name: Bip01
	pTmp27->pBone[0].T = Vect(-2.487148f, -7.185501f, -3.729600f);
	pTmp27->pBone[0].Q = Quat(ROT_XYZ, 1.462632f, -1.522481f, -3.080174f);
	pTmp27->pBone[0].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Pelvis
	pTmp27->pBone[1].T = Vect(0.0f, 0.0f, 0.0f);
	pTmp27->pBone[1].Q = Quat(ROT_XYZ, 0.0f, 0.0f, 0.0f);
	pTmp27->pBone[1].S = Vect(1.0f, 1.0f, 1.0f);

	//     Node Name: Bip01 Spine
	pTmp27->pBone[2].T = Vect(6.016919f, -0.014115f, 0.000003f);
	pTmp27->pBone[2].Q = Quat(ROT_XYZ, 0.071592f, -0.061301f, -0.014479f);
	pTmp27->pBone[2].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Thigh
	pTmp27->pBone[3].T = Vect(-5.167661f, 0.929669f, 13.983590f);
	pTmp27->pBone[3].Q = Quat(ROT_XYZ, -3.053782f, -0.136513f, 2.144540f);
	pTmp27->pBone[3].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Calf
	pTmp27->pBone[4].T = Vect(16.961559f, -0.000002f, -0.000004f);
	pTmp27->pBone[4].Q = Quat(ROT_XYZ, -0.049870f, -0.010989f, -0.736626f);
	pTmp27->pBone[4].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Foot
	pTmp27->pBone[5].T = Vect(13.263550f, 0.000003f, 0.000002f);
	pTmp27->pBone[5].Q = Quat(ROT_XYZ, 0.011503f, 0.033901f, -0.135214f);
	pTmp27->pBone[5].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Thigh
	pTmp27->pBone[6].T = Vect(-6.842711f, -1.022559f, -13.237880f);
	pTmp27->pBone[6].Q = Quat(ROT_XYZ, -3.016794f, -0.008976f, -2.636926f);
	pTmp27->pBone[6].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Calf
	pTmp27->pBone[7].T = Vect(16.961571f, 0.000003f, -0.000003f);
	pTmp27->pBone[7].Q = Quat(ROT_XYZ, -0.232632f, 0.012107f, -0.527144f);
	pTmp27->pBone[7].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Foot
	pTmp27->pBone[8].T = Vect(13.263530f, -0.000000f, 0.000002f);
	pTmp27->pBone[8].Q = Quat(ROT_XYZ, 0.007094f, -0.068261f, 0.532885f);
	pTmp27->pBone[8].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Spine1
	pTmp27->pBone[9].T = Vect(17.730431f, -0.017629f, 0.000002f);
	pTmp27->pBone[9].Q = Quat(ROT_XYZ, 0.144526f, -0.121338f, -0.035011f);
	pTmp27->pBone[9].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Neck
	pTmp27->pBone[10].T = Vect(22.140881f, -0.008821f, 0.000003f);
	pTmp27->pBone[10].Q = Quat(ROT_XYZ, -0.111641f, 0.118856f, -0.179923f);
	pTmp27->pBone[10].S = Vect(1.000000f, 1.000000f, 1.000001f);

	//     Node Name: Bip01 L Clavicle
	pTmp27->pBone[11].T = Vect(-0.649160f, -0.595534f, 5.389955f);
	pTmp27->pBone[11].Q = Quat(ROT_XYZ, 0.284126f, -1.194679f, 2.982392f);
	pTmp27->pBone[11].S = Vect(0.999999f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L UpperArm
	pTmp27->pBone[12].T = Vect(13.067860f, 0.000004f, -0.000004f);
	pTmp27->pBone[12].Q = Quat(ROT_XYZ, 0.129218f, 0.918234f, 0.471316f);
	pTmp27->pBone[12].S = Vect(1.000000f, 1.000001f, 1.000001f);

	//     Node Name: Bip01 L Forearm
	pTmp27->pBone[13].T = Vect(15.099380f, -0.000367f, 0.000254f);
	pTmp27->pBone[13].Q = Quat(ROT_XYZ, 0.849168f, 0.017082f, -0.175005f);
	pTmp27->pBone[13].S = Vect(1.000000f, 1.000001f, 1.000001f);

	//     Node Name: Bip01 L Hand
	pTmp27->pBone[14].T = Vect(15.099410f, 0.000064f, 0.000059f);
	pTmp27->pBone[14].Q = Quat(ROT_XYZ, -1.586783f, 0.211881f, -0.115498f);
	pTmp27->pBone[14].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Clavicle
	pTmp27->pBone[15].T = Vect(0.646026f, 0.612826f, -5.388391f);
	pTmp27->pBone[15].Q = Quat(ROT_XYZ, -0.850890f, 0.873219f, 2.382681f);
	pTmp27->pBone[15].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R UpperArm
	pTmp27->pBone[16].T = Vect(13.067840f, -0.000001f, 0.000001f);
	pTmp27->pBone[16].Q = Quat(ROT_XYZ, 0.070199f, -0.570949f, 0.208218f);
	pTmp27->pBone[16].S = Vect(1.000000f, 1.000000f, 1.000001f);

	//     Node Name: Bip01 R Forearm
	pTmp27->pBone[17].T = Vect(15.099410f, -0.000649f, -0.000106f);
	pTmp27->pBone[17].Q = Quat(ROT_XYZ, -0.732028f, -0.069634f, -0.501635f);
	pTmp27->pBone[17].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Hand
	pTmp27->pBone[18].T = Vect(15.099400f, 0.000181f, -0.000110f);
	pTmp27->pBone[18].Q = Quat(ROT_XYZ, 1.537089f, 0.118425f, -0.173138f);
	pTmp27->pBone[18].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Head
	pTmp27->pBone[19].T = Vect(11.079190f, 0.000001f, 0.000001f);
	pTmp27->pBone[19].Q = Quat(ROT_XYZ, 0.012663f, 0.071337f, 0.318290f);
	pTmp27->pBone[19].S = Vect(1.000000f, 1.000000f, 1.000000f);



	//------- Key Time: 28  ms: 933 ------------------ 

	Frame_Bucket *pTmp28 = new Frame_Bucket();
	pTmp28->prevBucket = pTmp27;
	pTmp28->nextBucket = 0;
	pTmp28->KeyTime = 28 * Time(TIME_NTSC_30_FRAME);
	pTmp28->pBone = new Bone[NUM_BONES];
	pTmp27->nextBucket = pTmp28;

	//     Node Name: RootNode
	//     Node Name: Bip01
	pTmp28->pBone[0].T = Vect(-1.881800f, -6.908234f, -4.168232f);
	pTmp28->pBone[0].Q = Quat(ROT_XYZ, 1.630231f, -1.514058f, 3.046975f);
	pTmp28->pBone[0].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Pelvis
	pTmp28->pBone[1].T = Vect(0.0f, 0.0f, 0.0f);
	pTmp28->pBone[1].Q = Quat(ROT_XYZ, 0.0f, 0.0f, 0.0f);
	pTmp28->pBone[1].S = Vect(1.0f, 1.0f, 1.0f);

	//     Node Name: Bip01 Spine
	pTmp28->pBone[2].T = Vect(6.016918f, -0.014115f, 0.000003f);
	pTmp28->pBone[2].Q = Quat(ROT_XYZ, 0.064275f, -0.063185f, -0.013069f);
	pTmp28->pBone[2].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Thigh
	pTmp28->pBone[3].T = Vect(-5.141232f, 0.836402f, 13.999160f);
	pTmp28->pBone[3].Q = Quat(ROT_XYZ, -3.064013f, -0.152263f, 2.171018f);
	pTmp28->pBone[3].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Calf
	pTmp28->pBone[4].T = Vect(16.961559f, -0.000002f, -0.000004f);
	pTmp28->pBone[4].Q = Quat(ROT_XYZ, -0.042834f, -0.009857f, -0.819732f);
	pTmp28->pBone[4].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Foot
	pTmp28->pBone[5].T = Vect(13.263550f, 0.000003f, 0.000002f);
	pTmp28->pBone[5].Q = Quat(ROT_XYZ, 0.012814f, -0.025347f, -0.097499f);
	pTmp28->pBone[5].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Thigh
	pTmp28->pBone[6].T = Vect(-6.867721f, -0.916367f, -13.232650f);
	pTmp28->pBone[6].Q = Quat(ROT_XYZ, -3.002058f, 0.045475f, -2.696591f);
	pTmp28->pBone[6].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Calf
	pTmp28->pBone[7].T = Vect(16.961571f, 0.000003f, -0.000003f);
	pTmp28->pBone[7].Q = Quat(ROT_XYZ, -0.267917f, 0.018955f, -0.720983f);
	pTmp28->pBone[7].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Foot
	pTmp28->pBone[8].T = Vect(13.263530f, -0.000000f, 0.000002f);
	pTmp28->pBone[8].Q = Quat(ROT_XYZ, -0.003631f, -0.076682f, 0.491983f);
	pTmp28->pBone[8].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Spine1
	pTmp28->pBone[9].T = Vect(17.730431f, -0.017629f, 0.000002f);
	pTmp28->pBone[9].Q = Quat(ROT_XYZ, 0.129815f, -0.125338f, -0.031873f);
	pTmp28->pBone[9].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Neck
	pTmp28->pBone[10].T = Vect(22.140881f, -0.008821f, 0.000003f);
	pTmp28->pBone[10].Q = Quat(ROT_XYZ, -0.106763f, 0.129623f, -0.181275f);
	pTmp28->pBone[10].S = Vect(1.000000f, 1.000000f, 1.000001f);

	//     Node Name: Bip01 L Clavicle
	pTmp28->pBone[11].T = Vect(-0.707512f, -0.568453f, 5.385529f);
	pTmp28->pBone[11].Q = Quat(ROT_XYZ, 0.319817f, -1.170613f, 2.943288f);
	pTmp28->pBone[11].S = Vect(0.999999f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L UpperArm
	pTmp28->pBone[12].T = Vect(13.067860f, 0.000004f, -0.000004f);
	pTmp28->pBone[12].Q = Quat(ROT_XYZ, 0.127426f, 0.901197f, 0.472938f);
	pTmp28->pBone[12].S = Vect(1.000000f, 1.000001f, 1.000001f);

	//     Node Name: Bip01 L Forearm
	pTmp28->pBone[13].T = Vect(15.099380f, -0.000360f, 0.000217f);
	pTmp28->pBone[13].Q = Quat(ROT_XYZ, 0.859162f, 0.017245f, -0.196481f);
	pTmp28->pBone[13].S = Vect(1.000000f, 1.000001f, 1.000001f);

	//     Node Name: Bip01 L Hand
	pTmp28->pBone[14].T = Vect(15.099410f, 0.000063f, 0.000058f);
	pTmp28->pBone[14].Q = Quat(ROT_XYZ, -1.588231f, 0.225057f, -0.113380f);
	pTmp28->pBone[14].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Clavicle
	pTmp28->pBone[15].T = Vect(0.704376f, 0.585756f, -5.384091f);
	pTmp28->pBone[15].Q = Quat(ROT_XYZ, -0.843212f, 0.883311f, 2.391354f);
	pTmp28->pBone[15].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R UpperArm
	pTmp28->pBone[16].T = Vect(13.067840f, -0.000001f, 0.000001f);
	pTmp28->pBone[16].Q = Quat(ROT_XYZ, 0.045783f, -0.623391f, 0.244964f);
	pTmp28->pBone[16].S = Vect(1.000000f, 1.000000f, 1.000001f);

	//     Node Name: Bip01 R Forearm
	pTmp28->pBone[17].T = Vect(15.099410f, -0.000498f, -0.000142f);
	pTmp28->pBone[17].Q = Quat(ROT_XYZ, -0.703314f, -0.062588f, -0.462200f);
	pTmp28->pBone[17].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Hand
	pTmp28->pBone[18].T = Vect(15.099400f, 0.000182f, -0.000110f);
	pTmp28->pBone[18].Q = Quat(ROT_XYZ, 1.536596f, 0.112771f, -0.170494f);
	pTmp28->pBone[18].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Head
	pTmp28->pBone[19].T = Vect(11.079190f, 0.000001f, 0.000001f);
	pTmp28->pBone[19].Q = Quat(ROT_XYZ, 0.020315f, 0.073098f, 0.313604f);
	pTmp28->pBone[19].S = Vect(1.000000f, 1.000000f, 1.000000f);



	//------- Key Time: 29  ms: 966 ------------------ 

	Frame_Bucket *pTmp29 = new Frame_Bucket();
	pTmp29->prevBucket = pTmp28;
	pTmp29->nextBucket = 0;
	pTmp29->KeyTime = 29 * Time(TIME_NTSC_30_FRAME);
	pTmp29->pBone = new Bone[NUM_BONES];
	pTmp28->nextBucket = pTmp29;

	//     Node Name: RootNode
	//     Node Name: Bip01
	pTmp29->pBone[0].T = Vect(-1.689475f, -6.181122f, -4.240609f);
	pTmp29->pBone[0].Q = Quat(ROT_XYZ, 1.736185f, -1.478284f, 2.948182f);
	pTmp29->pBone[0].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Pelvis
	pTmp29->pBone[1].T = Vect(0.0f, 0.0f, 0.0f);
	pTmp29->pBone[1].Q = Quat(ROT_XYZ, 0.0f, 0.0f, 0.0f);
	pTmp29->pBone[1].S = Vect(1.0f, 1.0f, 1.0f);

	//     Node Name: Bip01 Spine
	pTmp29->pBone[2].T = Vect(6.016917f, -0.014115f, 0.000003f);
	pTmp29->pBone[2].Q = Quat(ROT_XYZ, 0.059397f, -0.060086f, -0.009887f);
	pTmp29->pBone[2].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Thigh
	pTmp29->pBone[3].T = Vect(-5.184851f, 0.786267f, 13.985970f);
	pTmp29->pBone[3].Q = Quat(ROT_XYZ, -3.077840f, -0.169239f, 2.214138f);
	pTmp29->pBone[3].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Calf
	pTmp29->pBone[4].T = Vect(16.961559f, -0.000002f, -0.000004f);
	pTmp29->pBone[4].Q = Quat(ROT_XYZ, -0.051451f, -0.011523f, -0.925846f);
	pTmp29->pBone[4].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Foot
	pTmp29->pBone[5].T = Vect(13.263550f, 0.000003f, 0.000002f);
	pTmp29->pBone[5].Q = Quat(ROT_XYZ, 0.008184f, -0.027720f, 0.007938f);
	pTmp29->pBone[5].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Thigh
	pTmp29->pBone[6].T = Vect(-6.826763f, -0.833956f, -13.259270f);
	pTmp29->pBone[6].Q = Quat(ROT_XYZ, -2.992566f, 0.115461f, -2.796517f);
	pTmp29->pBone[6].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Calf
	pTmp29->pBone[7].T = Vect(16.961571f, 0.000003f, -0.000003f);
	pTmp29->pBone[7].Q = Quat(ROT_XYZ, -0.300599f, 0.022476f, -0.949928f);
	pTmp29->pBone[7].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Foot
	pTmp29->pBone[8].T = Vect(13.263530f, -0.000000f, 0.000002f);
	pTmp29->pBone[8].Q = Quat(ROT_XYZ, -0.013102f, -0.097592f, 0.424806f);
	pTmp29->pBone[8].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Spine1
	pTmp29->pBone[9].T = Vect(17.730431f, -0.017629f, 0.000002f);
	pTmp29->pBone[9].Q = Quat(ROT_XYZ, 0.119765f, -0.119425f, -0.024997f);
	pTmp29->pBone[9].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Neck
	pTmp29->pBone[10].T = Vect(22.140881f, -0.008821f, 0.000003f);
	pTmp29->pBone[10].Q = Quat(ROT_XYZ, -0.085001f, 0.124360f, -0.170093f);
	pTmp29->pBone[10].S = Vect(1.000000f, 1.000000f, 1.000001f);

	//     Node Name: Bip01 L Clavicle
	pTmp29->pBone[11].T = Vect(-0.678920f, -0.451421f, 5.400275f);
	pTmp29->pBone[11].Q = Quat(ROT_XYZ, 0.365964f, -1.152682f, 2.887019f);
	pTmp29->pBone[11].S = Vect(0.999999f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L UpperArm
	pTmp29->pBone[12].T = Vect(13.067860f, 0.000004f, -0.000004f);
	pTmp29->pBone[12].Q = Quat(ROT_XYZ, 0.110321f, 0.884962f, 0.453767f);
	pTmp29->pBone[12].S = Vect(1.000000f, 1.000001f, 1.000001f);

	//     Node Name: Bip01 L Forearm
	pTmp29->pBone[13].T = Vect(15.099380f, -0.000274f, 0.000124f);
	pTmp29->pBone[13].Q = Quat(ROT_XYZ, 0.861480f, 0.016976f, -0.216383f);
	pTmp29->pBone[13].S = Vect(1.000000f, 1.000001f, 1.000001f);

	//     Node Name: Bip01 L Hand
	pTmp29->pBone[14].T = Vect(15.099410f, 0.000062f, 0.000058f);
	pTmp29->pBone[14].Q = Quat(ROT_XYZ, -1.588527f, 0.225162f, -0.119560f);
	pTmp29->pBone[14].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Clavicle
	pTmp29->pBone[15].T = Vect(0.675961f, 0.468780f, -5.399162f);
	pTmp29->pBone[15].Q = Quat(ROT_XYZ, -0.805030f, 0.886763f, 2.410198f);
	pTmp29->pBone[15].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R UpperArm
	pTmp29->pBone[16].T = Vect(13.067840f, -0.000001f, 0.000001f);
	pTmp29->pBone[16].Q = Quat(ROT_XYZ, 0.015324f, -0.665408f, 0.276541f);
	pTmp29->pBone[16].S = Vect(1.000000f, 1.000000f, 1.000001f);

	//     Node Name: Bip01 R Forearm
	pTmp29->pBone[17].T = Vect(15.099410f, -0.001394f, -0.000300f);
	pTmp29->pBone[17].Q = Quat(ROT_XYZ, -0.705116f, -0.056583f, -0.416078f);
	pTmp29->pBone[17].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Hand
	pTmp29->pBone[18].T = Vect(15.099400f, 0.000183f, -0.000110f);
	pTmp29->pBone[18].Q = Quat(ROT_XYZ, 1.538360f, 0.100554f, -0.167921f);
	pTmp29->pBone[18].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Head
	pTmp29->pBone[19].T = Vect(11.079190f, 0.000001f, 0.000001f);
	pTmp29->pBone[19].Q = Quat(ROT_XYZ, 0.034138f, 0.077502f, 0.304734f);
	pTmp29->pBone[19].S = Vect(1.000000f, 1.000000f, 1.000000f);



	//------- Key Time: 30  ms: 1000 ------------------ 

	Frame_Bucket *pTmp30 = new Frame_Bucket();
	pTmp30->prevBucket = pTmp29;
	pTmp30->nextBucket = 0;
	pTmp30->KeyTime = 30 * Time(TIME_NTSC_30_FRAME);
	pTmp30->pBone = new Bone[NUM_BONES];
	pTmp29->nextBucket = pTmp30;

	//     Node Name: RootNode
	//     Node Name: Bip01
	pTmp30->pBone[0].T = Vect(-1.309788f, -5.170105f, -4.374341f);
	pTmp30->pBone[0].Q = Quat(ROT_XYZ, 1.733123f, -1.443215f, 2.961751f);
	pTmp30->pBone[0].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Pelvis
	pTmp30->pBone[1].T = Vect(0.0f, 0.0f, 0.0f);
	pTmp30->pBone[1].Q = Quat(ROT_XYZ, 0.0f, 0.0f, 0.0f);
	pTmp30->pBone[1].S = Vect(1.0f, 1.0f, 1.0f);

	//     Node Name: Bip01 Spine
	pTmp30->pBone[2].T = Vect(6.016916f, -0.014115f, 0.000003f);
	pTmp30->pBone[2].Q = Quat(ROT_XYZ, 0.050539f, -0.055102f, -0.006441f);
	pTmp30->pBone[2].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Thigh
	pTmp30->pBone[3].T = Vect(-5.254828f, 0.681748f, 13.965370f);
	pTmp30->pBone[3].Q = Quat(ROT_XYZ, -3.091442f, -0.173603f, 2.281789f);
	pTmp30->pBone[3].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Calf
	pTmp30->pBone[4].T = Vect(16.961559f, -0.000002f, -0.000004f);
	pTmp30->pBone[4].Q = Quat(ROT_XYZ, -0.080965f, -0.012889f, -0.988951f);
	pTmp30->pBone[4].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Foot
	pTmp30->pBone[5].T = Vect(13.263550f, 0.000003f, 0.000002f);
	pTmp30->pBone[5].Q = Quat(ROT_XYZ, 0.002266f, -0.042435f, 0.120189f);
	pTmp30->pBone[5].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Thigh
	pTmp30->pBone[6].T = Vect(-6.760635f, -0.697481f, -13.301000f);
	pTmp30->pBone[6].Q = Quat(ROT_XYZ, -3.001224f, 0.181502f, -2.922607f);
	pTmp30->pBone[6].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Calf
	pTmp30->pBone[7].T = Vect(16.961571f, 0.000003f, -0.000003f);
	pTmp30->pBone[7].Q = Quat(ROT_XYZ, -0.288662f, 0.021818f, -1.154952f);
	pTmp30->pBone[7].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Foot
	pTmp30->pBone[8].T = Vect(13.263530f, -0.000000f, 0.000002f);
	pTmp30->pBone[8].Q = Quat(ROT_XYZ, -0.015615f, -0.107283f, 0.306043f);
	pTmp30->pBone[8].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Spine1
	pTmp30->pBone[9].T = Vect(17.730431f, -0.017629f, 0.000002f);
	pTmp30->pBone[9].Q = Quat(ROT_XYZ, 0.101715f, -0.109761f, -0.017298f);
	pTmp30->pBone[9].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Neck
	pTmp30->pBone[10].T = Vect(22.140881f, -0.008821f, 0.000003f);
	pTmp30->pBone[10].Q = Quat(ROT_XYZ, -0.051555f, 0.112862f, -0.150038f);
	pTmp30->pBone[10].S = Vect(1.000000f, 1.000000f, 1.000001f);

	//     Node Name: Bip01 L Clavicle
	pTmp30->pBone[11].T = Vect(-0.616378f, -0.270948f, 5.419806f);
	pTmp30->pBone[11].Q = Quat(ROT_XYZ, 0.419484f, -1.128272f, 2.816982f);
	pTmp30->pBone[11].S = Vect(0.999999f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L UpperArm
	pTmp30->pBone[12].T = Vect(13.067860f, 0.000004f, -0.000004f);
	pTmp30->pBone[12].Q = Quat(ROT_XYZ, 0.088183f, 0.868228f, 0.423008f);
	pTmp30->pBone[12].S = Vect(1.000000f, 1.000001f, 1.000001f);

	//     Node Name: Bip01 L Forearm
	pTmp30->pBone[13].T = Vect(15.099380f, -0.000476f, 0.000130f);
	pTmp30->pBone[13].Q = Quat(ROT_XYZ, 0.850498f, 0.017094f, -0.239518f);
	pTmp30->pBone[13].S = Vect(1.000000f, 1.000001f, 1.000001f);

	//     Node Name: Bip01 L Hand
	pTmp30->pBone[14].T = Vect(15.099410f, 0.000061f, 0.000057f);
	pTmp30->pBone[14].Q = Quat(ROT_XYZ, -1.587156f, 0.203103f, -0.135934f);
	pTmp30->pBone[14].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Clavicle
	pTmp30->pBone[15].T = Vect(0.613780f, 0.288389f, -5.419205f);
	pTmp30->pBone[15].Q = Quat(ROT_XYZ, -0.751162f, 0.889686f, 2.430853f);
	pTmp30->pBone[15].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R UpperArm
	pTmp30->pBone[16].T = Vect(13.067840f, -0.000001f, 0.000001f);
	pTmp30->pBone[16].Q = Quat(ROT_XYZ, -0.016487f, -0.690136f, 0.304817f);
	pTmp30->pBone[16].S = Vect(1.000000f, 1.000000f, 1.000001f);

	//     Node Name: Bip01 R Forearm
	pTmp30->pBone[17].T = Vect(15.099410f, -0.001004f, -0.000250f);
	pTmp30->pBone[17].Q = Quat(ROT_XYZ, -0.747073f, -0.052496f, -0.371939f);
	pTmp30->pBone[17].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Hand
	pTmp30->pBone[18].T = Vect(15.099400f, 0.000184f, -0.000110f);
	pTmp30->pBone[18].Q = Quat(ROT_XYZ, 1.541074f, 0.083623f, -0.166901f);
	pTmp30->pBone[18].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Head
	pTmp30->pBone[19].T = Vect(11.079190f, 0.000001f, 0.000001f);
	pTmp30->pBone[19].Q = Quat(ROT_XYZ, 0.053997f, 0.074141f, 0.291013f);
	pTmp30->pBone[19].S = Vect(1.000000f, 1.000000f, 1.000000f);



	//------- Key Time: 31  ms: 1033 ------------------ 

	Frame_Bucket *pTmp31 = new Frame_Bucket();
	pTmp31->prevBucket = pTmp30;
	pTmp31->nextBucket = 0;
	pTmp31->KeyTime = 31 * Time(TIME_NTSC_30_FRAME);
	pTmp31->pBone = new Bone[NUM_BONES];
	pTmp30->nextBucket = pTmp31;

	//     Node Name: RootNode
	//     Node Name: Bip01
	pTmp31->pBone[0].T = Vect(-1.491607f, -4.097389f, -4.153934f);
	pTmp31->pBone[0].Q = Quat(ROT_XYZ, 1.754832f, -1.421156f, 2.941626f);
	pTmp31->pBone[0].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Pelvis
	pTmp31->pBone[1].T = Vect(0.0f, 0.0f, 0.0f);
	pTmp31->pBone[1].Q = Quat(ROT_XYZ, 0.0f, 0.0f, 0.0f);
	pTmp31->pBone[1].S = Vect(1.0f, 1.0f, 1.0f);

	//     Node Name: Bip01 Spine
	pTmp31->pBone[2].T = Vect(6.016915f, -0.014115f, 0.000003f);
	pTmp31->pBone[2].Q = Quat(ROT_XYZ, 0.038513f, -0.048071f, -0.005198f);
	pTmp31->pBone[2].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Thigh
	pTmp31->pBone[3].T = Vect(-5.352951f, 0.519801f, 13.935000f);
	pTmp31->pBone[3].Q = Quat(ROT_XYZ, -3.106316f, -0.177966f, 2.359972f);
	pTmp31->pBone[3].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Calf
	pTmp31->pBone[4].T = Vect(16.961559f, -0.000002f, -0.000004f);
	pTmp31->pBone[4].Q = Quat(ROT_XYZ, -0.102995f, -0.013554f, -1.030864f);
	pTmp31->pBone[4].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Foot
	pTmp31->pBone[5].T = Vect(13.263550f, 0.000003f, 0.000002f);
	pTmp31->pBone[5].Q = Quat(ROT_XYZ, -0.005901f, -0.058720f, 0.226969f);
	pTmp31->pBone[5].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Thigh
	pTmp31->pBone[6].T = Vect(-6.666756f, -0.531821f, -13.355890f);
	pTmp31->pBone[6].Q = Quat(ROT_XYZ, -3.033370f, 0.209431f, -3.032059f);
	pTmp31->pBone[6].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Calf
	pTmp31->pBone[7].T = Vect(16.961571f, 0.000003f, -0.000003f);
	pTmp31->pBone[7].Q = Quat(ROT_XYZ, -0.223543f, 0.019728f, -1.291813f);
	pTmp31->pBone[7].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Foot
	pTmp31->pBone[8].T = Vect(13.263530f, -0.000000f, 0.000002f);
	pTmp31->pBone[8].Q = Quat(ROT_XYZ, -0.007776f, -0.054620f, 0.178058f);
	pTmp31->pBone[8].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Spine1
	pTmp31->pBone[9].T = Vect(17.730431f, -0.017629f, 0.000002f);
	pTmp31->pBone[9].Q = Quat(ROT_XYZ, 0.077457f, -0.095869f, -0.013864f);
	pTmp31->pBone[9].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Neck
	pTmp31->pBone[10].T = Vect(22.140881f, -0.008821f, 0.000003f);
	pTmp31->pBone[10].Q = Quat(ROT_XYZ, -0.016084f, 0.098982f, -0.123297f);
	pTmp31->pBone[10].S = Vect(1.000000f, 1.000000f, 1.000001f);

	//     Node Name: Bip01 L Clavicle
	pTmp31->pBone[11].T = Vect(-0.540769f, -0.078664f, 5.434061f);
	pTmp31->pBone[11].Q = Quat(ROT_XYZ, 0.492952f, -1.103971f, 2.720254f);
	pTmp31->pBone[11].S = Vect(0.999999f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L UpperArm
	pTmp31->pBone[12].T = Vect(13.067860f, 0.000004f, -0.000004f);
	pTmp31->pBone[12].Q = Quat(ROT_XYZ, 0.072962f, 0.847302f, 0.400044f);
	pTmp31->pBone[12].S = Vect(1.000000f, 1.000001f, 1.000001f);

	//     Node Name: Bip01 L Forearm
	pTmp31->pBone[13].T = Vect(15.099380f, -0.000363f, 0.000142f);
	pTmp31->pBone[13].Q = Quat(ROT_XYZ, 0.836793f, 0.018614f, -0.268207f);
	pTmp31->pBone[13].S = Vect(1.000000f, 1.000001f, 1.000001f);

	//     Node Name: Bip01 L Hand
	pTmp31->pBone[14].T = Vect(15.099410f, 0.000061f, 0.000056f);
	pTmp31->pBone[14].Q = Quat(ROT_XYZ, -1.584078f, 0.180307f, -0.126919f);
	pTmp31->pBone[14].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Clavicle
	pTmp31->pBone[15].T = Vect(0.538635f, 0.096178f, -5.433993f);
	pTmp31->pBone[15].Q = Quat(ROT_XYZ, -0.684684f, 0.896344f, 2.458082f);
	pTmp31->pBone[15].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R UpperArm
	pTmp31->pBone[16].T = Vect(13.067840f, -0.000001f, 0.000001f);
	pTmp31->pBone[16].Q = Quat(ROT_XYZ, -0.048067f, -0.709674f, 0.342874f);
	pTmp31->pBone[16].S = Vect(1.000000f, 1.000000f, 1.000001f);

	//     Node Name: Bip01 R Forearm
	pTmp31->pBone[17].T = Vect(15.099410f, -0.002401f, -0.000545f);
	pTmp31->pBone[17].Q = Quat(ROT_XYZ, -0.807794f, -0.048164f, -0.333875f);
	pTmp31->pBone[17].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Hand
	pTmp31->pBone[18].T = Vect(15.099400f, 0.000184f, -0.000110f);
	pTmp31->pBone[18].Q = Quat(ROT_XYZ, 1.545958f, 0.056473f, -0.164813f);
	pTmp31->pBone[18].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Head
	pTmp31->pBone[19].T = Vect(11.079190f, 0.000001f, 0.000001f);
	pTmp31->pBone[19].Q = Quat(ROT_XYZ, 0.067415f, 0.074307f, 0.288056f);
	pTmp31->pBone[19].S = Vect(1.000000f, 1.000000f, 1.000000f);



	//------- Key Time: 32  ms: 1066 ------------------ 

	Frame_Bucket *pTmp32 = new Frame_Bucket();
	pTmp32->prevBucket = pTmp31;
	pTmp32->nextBucket = 0;
	pTmp32->KeyTime = 32 * Time(TIME_NTSC_30_FRAME);
	pTmp32->pBone = new Bone[NUM_BONES];
	pTmp31->nextBucket = pTmp32;

	//     Node Name: RootNode
	//     Node Name: Bip01
	pTmp32->pBone[0].T = Vect(-1.768452f, -2.863916f, -3.751906f);
	pTmp32->pBone[0].Q = Quat(ROT_XYZ, 1.746380f, -1.394423f, 2.947267f);
	pTmp32->pBone[0].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Pelvis
	pTmp32->pBone[1].T = Vect(0.0f, 0.0f, 0.0f);
	pTmp32->pBone[1].Q = Quat(ROT_XYZ, 0.0f, 0.0f, 0.0f);
	pTmp32->pBone[1].S = Vect(1.0f, 1.0f, 1.0f);

	//     Node Name: Bip01 Spine
	pTmp32->pBone[2].T = Vect(6.016914f, -0.014115f, 0.000003f);
	pTmp32->pBone[2].Q = Quat(ROT_XYZ, 0.026490f, -0.041093f, -0.001944f);
	pTmp32->pBone[2].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Thigh
	pTmp32->pBone[3].T = Vect(-5.450202f, 0.370791f, 13.902060f);
	pTmp32->pBone[3].Q = Quat(ROT_XYZ, -3.128134f, -0.181790f, 2.451159f);
	pTmp32->pBone[3].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Calf
	pTmp32->pBone[4].T = Vect(16.961559f, -0.000002f, -0.000004f);
	pTmp32->pBone[4].Q = Quat(ROT_XYZ, -0.129006f, -0.015424f, -1.048686f);
	pTmp32->pBone[4].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Foot
	pTmp32->pBone[5].T = Vect(13.263550f, 0.000003f, 0.000002f);
	pTmp32->pBone[5].Q = Quat(ROT_XYZ, -0.015059f, -0.070458f, 0.328477f);
	pTmp32->pBone[5].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Thigh
	pTmp32->pBone[6].T = Vect(-6.573470f, -0.352862f, -13.408010f);
	pTmp32->pBone[6].Q = Quat(ROT_XYZ, -3.072727f, 0.217908f, 3.125959f);
	pTmp32->pBone[6].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Calf
	pTmp32->pBone[7].T = Vect(16.961571f, 0.000003f, -0.000003f);
	pTmp32->pBone[7].Q = Quat(ROT_XYZ, -0.176086f, 0.014638f, -1.420145f);
	pTmp32->pBone[7].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Foot
	pTmp32->pBone[8].T = Vect(13.263530f, -0.000000f, 0.000002f);
	pTmp32->pBone[8].Q = Quat(ROT_XYZ, -0.006560f, -0.020714f, 0.232759f);
	pTmp32->pBone[8].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Spine1
	pTmp32->pBone[9].T = Vect(17.730431f, -0.017629f, 0.000002f);
	pTmp32->pBone[9].Q = Quat(ROT_XYZ, 0.053169f, -0.082097f, -0.006580f);
	pTmp32->pBone[9].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Neck
	pTmp32->pBone[10].T = Vect(22.140881f, -0.008821f, 0.000003f);
	pTmp32->pBone[10].Q = Quat(ROT_XYZ, 0.018807f, 0.100721f, -0.106785f);
	pTmp32->pBone[10].S = Vect(1.000000f, 1.000000f, 1.000001f);

	//     Node Name: Bip01 L Clavicle
	pTmp32->pBone[11].T = Vect(-0.550076f, 0.110944f, 5.432564f);
	pTmp32->pBone[11].Q = Quat(ROT_XYZ, 0.555732f, -1.074434f, 2.645163f);
	pTmp32->pBone[11].S = Vect(0.999999f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L UpperArm
	pTmp32->pBone[12].T = Vect(13.067860f, 0.000004f, -0.000004f);
	pTmp32->pBone[12].Q = Quat(ROT_XYZ, 0.046482f, 0.829120f, 0.360154f);
	pTmp32->pBone[12].S = Vect(1.000000f, 1.000001f, 1.000001f);

	//     Node Name: Bip01 L Forearm
	pTmp32->pBone[13].T = Vect(15.099380f, -0.000761f, 0.000157f);
	pTmp32->pBone[13].Q = Quat(ROT_XYZ, 0.831772f, 0.020346f, -0.295937f);
	pTmp32->pBone[13].S = Vect(1.000000f, 1.000001f, 1.000001f);

	//     Node Name: Bip01 L Hand
	pTmp32->pBone[14].T = Vect(15.099410f, 0.000060f, 0.000056f);
	pTmp32->pBone[14].Q = Quat(ROT_XYZ, -1.580891f, 0.157203f, -0.116644f);
	pTmp32->pBone[14].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Clavicle
	pTmp32->pBone[15].T = Vect(0.548229f, -0.093404f, -5.433086f);
	pTmp32->pBone[15].Q = Quat(ROT_XYZ, -0.637131f, 0.915058f, 2.479417f);
	pTmp32->pBone[15].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R UpperArm
	pTmp32->pBone[16].T = Vect(13.067840f, -0.000001f, 0.000001f);
	pTmp32->pBone[16].Q = Quat(ROT_XYZ, -0.078448f, -0.726272f, 0.377541f);
	pTmp32->pBone[16].S = Vect(1.000000f, 1.000000f, 1.000001f);

	//     Node Name: Bip01 R Forearm
	pTmp32->pBone[17].T = Vect(15.099410f, -0.008379f, -0.001972f);
	pTmp32->pBone[17].Q = Quat(ROT_XYZ, -0.857770f, -0.043887f, -0.306694f);
	pTmp32->pBone[17].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Hand
	pTmp32->pBone[18].T = Vect(15.099400f, 0.000184f, -0.000110f);
	pTmp32->pBone[18].Q = Quat(ROT_XYZ, 1.549017f, 0.041888f, -0.167490f);
	pTmp32->pBone[18].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Head
	pTmp32->pBone[19].T = Vect(11.079190f, 0.000001f, 0.000001f);
	pTmp32->pBone[19].Q = Quat(ROT_XYZ, 0.083209f, 0.058493f, 0.286709f);
	pTmp32->pBone[19].S = Vect(1.000000f, 1.000000f, 1.000000f);



	//------- Key Time: 33  ms: 1100 ------------------ 

	Frame_Bucket *pTmp33 = new Frame_Bucket();
	pTmp33->prevBucket = pTmp32;
	pTmp33->nextBucket = 0;
	pTmp33->KeyTime = 33 * Time(TIME_NTSC_30_FRAME);
	pTmp33->pBone = new Bone[NUM_BONES];
	pTmp32->nextBucket = pTmp33;

	//     Node Name: RootNode
	//     Node Name: Bip01
	pTmp33->pBone[0].T = Vect(-1.787555f, -1.620269f, -3.004836f);
	pTmp33->pBone[0].Q = Quat(ROT_XYZ, 1.739038f, -1.370613f, 2.953707f);
	pTmp33->pBone[0].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Pelvis
	pTmp33->pBone[1].T = Vect(0.0f, 0.0f, 0.0f);
	pTmp33->pBone[1].Q = Quat(ROT_XYZ, 0.0f, 0.0f, 0.0f);
	pTmp33->pBone[1].S = Vect(1.0f, 1.0f, 1.0f);

	//     Node Name: Bip01 Spine
	pTmp33->pBone[2].T = Vect(6.016913f, -0.014115f, 0.000003f);
	pTmp33->pBone[2].Q = Quat(ROT_XYZ, 0.012938f, -0.032060f, 0.003325f);
	pTmp33->pBone[2].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Thigh
	pTmp33->pBone[3].T = Vect(-5.575471f, 0.213420f, 13.855600f);
	pTmp33->pBone[3].Q = Quat(ROT_XYZ, 3.125967f, -0.170187f, 2.557315f);
	pTmp33->pBone[3].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Calf
	pTmp33->pBone[4].T = Vect(16.961559f, -0.000002f, -0.000004f);
	pTmp33->pBone[4].Q = Quat(ROT_XYZ, -0.157630f, -0.017157f, -1.034461f);
	pTmp33->pBone[4].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Foot
	pTmp33->pBone[5].T = Vect(13.263550f, 0.000003f, 0.000002f);
	pTmp33->pBone[5].Q = Quat(ROT_XYZ, -0.025233f, -0.085360f, 0.418156f);
	pTmp33->pBone[5].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Thigh
	pTmp33->pBone[6].T = Vect(-6.451900f, -0.140185f, -13.470800f);
	pTmp33->pBone[6].Q = Quat(ROT_XYZ, -3.106206f, 0.212750f, 2.968772f);
	pTmp33->pBone[6].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Calf
	pTmp33->pBone[7].T = Vect(16.961571f, 0.000003f, -0.000003f);
	pTmp33->pBone[7].Q = Quat(ROT_XYZ, -0.158119f, 0.009592f, -1.519150f);
	pTmp33->pBone[7].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Foot
	pTmp33->pBone[8].T = Vect(13.263530f, -0.000000f, 0.000002f);
	pTmp33->pBone[8].Q = Quat(ROT_XYZ, -0.012880f, -0.053082f, 0.329332f);
	pTmp33->pBone[8].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Spine1
	pTmp33->pBone[9].T = Vect(17.730431f, -0.017629f, 0.000002f);
	pTmp33->pBone[9].Q = Quat(ROT_XYZ, 0.025823f, -0.064148f, 0.004646f);
	pTmp33->pBone[9].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Neck
	pTmp33->pBone[10].T = Vect(22.140881f, -0.008821f, 0.000003f);
	pTmp33->pBone[10].Q = Quat(ROT_XYZ, 0.060890f, 0.090168f, -0.100479f);
	pTmp33->pBone[10].S = Vect(1.000000f, 1.000000f, 1.000001f);

	//     Node Name: Bip01 L Clavicle
	pTmp33->pBone[11].T = Vect(-0.492654f, 0.339737f, 5.428581f);
	pTmp33->pBone[11].Q = Quat(ROT_XYZ, 0.638275f, -1.048542f, 2.563627f);
	pTmp33->pBone[11].S = Vect(0.999999f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L UpperArm
	pTmp33->pBone[12].T = Vect(13.067860f, 0.000004f, -0.000004f);
	pTmp33->pBone[12].Q = Quat(ROT_XYZ, 0.018515f, 0.811953f, 0.316460f);
	pTmp33->pBone[12].S = Vect(1.000000f, 1.000001f, 1.000001f);

	//     Node Name: Bip01 L Forearm
	pTmp33->pBone[13].T = Vect(15.099380f, -0.000642f, 0.000170f);
	pTmp33->pBone[13].Q = Quat(ROT_XYZ, 0.812013f, 0.022736f, -0.325992f);
	pTmp33->pBone[13].S = Vect(1.000000f, 1.000001f, 1.000001f);

	//     Node Name: Bip01 L Hand
	pTmp33->pBone[14].T = Vect(15.099410f, 0.000059f, 0.000055f);
	pTmp33->pBone[14].Q = Quat(ROT_XYZ, -1.578326f, 0.136105f, -0.117409f);
	pTmp33->pBone[14].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Clavicle
	pTmp33->pBone[15].T = Vect(0.490896f, -0.322224f, -5.429809f);
	pTmp33->pBone[15].Q = Quat(ROT_XYZ, -0.584216f, 0.939086f, 2.515243f);
	pTmp33->pBone[15].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R UpperArm
	pTmp33->pBone[16].T = Vect(13.067840f, -0.000001f, 0.000001f);
	pTmp33->pBone[16].Q = Quat(ROT_XYZ, -0.100192f, -0.738822f, 0.397581f);
	pTmp33->pBone[16].S = Vect(1.000000f, 1.000000f, 1.000001f);

	//     Node Name: Bip01 R Forearm
	pTmp33->pBone[17].T = Vect(15.099410f, -0.000474f, -0.000108f);
	pTmp33->pBone[17].Q = Quat(ROT_XYZ, -0.910158f, -0.040220f, -0.280332f);
	pTmp33->pBone[17].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Hand
	pTmp33->pBone[18].T = Vect(15.099400f, 0.000184f, -0.000110f);
	pTmp33->pBone[18].Q = Quat(ROT_XYZ, 1.553119f, 0.019269f, -0.172687f);
	pTmp33->pBone[18].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Head
	pTmp33->pBone[19].T = Vect(11.079190f, 0.000001f, 0.000001f);
	pTmp33->pBone[19].Q = Quat(ROT_XYZ, 0.100951f, 0.049828f, 0.284101f);
	pTmp33->pBone[19].S = Vect(1.000000f, 1.000000f, 1.000000f);



	//------- Key Time: 34  ms: 1133 ------------------ 

	Frame_Bucket *pTmp34 = new Frame_Bucket();
	pTmp34->prevBucket = pTmp33;
	pTmp34->nextBucket = 0;
	pTmp34->KeyTime = 34 * Time(TIME_NTSC_30_FRAME);
	pTmp34->pBone = new Bone[NUM_BONES];
	pTmp33->nextBucket = pTmp34;

	//     Node Name: RootNode
	//     Node Name: Bip01
	pTmp34->pBone[0].T = Vect(-1.518071f, -0.778720f, -2.439757f);
	pTmp34->pBone[0].Q = Quat(ROT_XYZ, 1.720037f, -1.356736f, 2.977251f);
	pTmp34->pBone[0].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Pelvis
	pTmp34->pBone[1].T = Vect(0.0f, 0.0f, 0.0f);
	pTmp34->pBone[1].Q = Quat(ROT_XYZ, 0.0f, 0.0f, 0.0f);
	pTmp34->pBone[1].S = Vect(1.0f, 1.0f, 1.0f);

	//     Node Name: Bip01 Spine
	pTmp34->pBone[2].T = Vect(6.016912f, -0.014115f, 0.000003f);
	pTmp34->pBone[2].Q = Quat(ROT_XYZ, -0.002809f, -0.023459f, 0.006974f);
	pTmp34->pBone[2].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Thigh
	pTmp34->pBone[3].T = Vect(-5.694271f, 0.017305f, 13.808830f);
	pTmp34->pBone[3].Q = Quat(ROT_XYZ, 3.099893f, -0.142895f, 2.654406f);
	pTmp34->pBone[3].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Calf
	pTmp34->pBone[4].T = Vect(16.961559f, -0.000002f, -0.000004f);
	pTmp34->pBone[4].Q = Quat(ROT_XYZ, -0.191553f, -0.018539f, -1.006362f);
	pTmp34->pBone[4].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Foot
	pTmp34->pBone[5].T = Vect(13.263550f, 0.000003f, 0.000002f);
	pTmp34->pBone[5].Q = Quat(ROT_XYZ, -0.033026f, -0.091176f, 0.485714f);
	pTmp34->pBone[5].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Thigh
	pTmp34->pBone[6].T = Vect(-6.335595f, 0.094063f, -13.526270f);
	pTmp34->pBone[6].Q = Quat(ROT_XYZ, -3.129562f, 0.199753f, 2.819612f);
	pTmp34->pBone[6].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Calf
	pTmp34->pBone[7].T = Vect(16.961571f, 0.000003f, -0.000003f);
	pTmp34->pBone[7].Q = Quat(ROT_XYZ, -0.144741f, 0.007846f, -1.545690f);
	pTmp34->pBone[7].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Foot
	pTmp34->pBone[8].T = Vect(13.263530f, -0.000000f, 0.000002f);
	pTmp34->pBone[8].Q = Quat(ROT_XYZ, -0.015827f, -0.063584f, 0.374588f);
	pTmp34->pBone[8].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Spine1
	pTmp34->pBone[9].T = Vect(17.730431f, -0.017629f, 0.000002f);
	pTmp34->pBone[9].Q = Quat(ROT_XYZ, -0.005758f, -0.046898f, 0.012426f);
	pTmp34->pBone[9].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Neck
	pTmp34->pBone[10].T = Vect(22.140881f, -0.008821f, 0.000003f);
	pTmp34->pBone[10].Q = Quat(ROT_XYZ, 0.099715f, 0.080085f, -0.093891f);
	pTmp34->pBone[10].S = Vect(1.000000f, 1.000000f, 1.000001f);

	//     Node Name: Bip01 L Clavicle
	pTmp34->pBone[11].T = Vect(-0.437736f, 0.550662f, 5.415977f);
	pTmp34->pBone[11].Q = Quat(ROT_XYZ, 0.719853f, -1.026053f, 2.482422f);
	pTmp34->pBone[11].S = Vect(0.999999f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L UpperArm
	pTmp34->pBone[12].T = Vect(13.067860f, 0.000004f, -0.000004f);
	pTmp34->pBone[12].Q = Quat(ROT_XYZ, -0.010097f, 0.794173f, 0.273800f);
	pTmp34->pBone[12].S = Vect(1.000000f, 1.000001f, 1.000001f);

	//     Node Name: Bip01 L Forearm
	pTmp34->pBone[13].T = Vect(15.099380f, -0.000754f, 0.000175f);
	pTmp34->pBone[13].Q = Quat(ROT_XYZ, 0.794663f, 0.025354f, -0.357895f);
	pTmp34->pBone[13].S = Vect(1.000000f, 1.000001f, 1.000001f);

	//     Node Name: Bip01 L Hand
	pTmp34->pBone[14].T = Vect(15.099410f, 0.000058f, 0.000055f);
	pTmp34->pBone[14].Q = Quat(ROT_XYZ, -1.578469f, 0.136855f, -0.122758f);
	pTmp34->pBone[14].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Clavicle
	pTmp34->pBone[15].T = Vect(0.436082f, -0.533196f, -5.417858f);
	pTmp34->pBone[15].Q = Quat(ROT_XYZ, -0.543234f, 0.971996f, 2.544615f);
	pTmp34->pBone[15].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R UpperArm
	pTmp34->pBone[16].T = Vect(13.067840f, -0.000001f, 0.000001f);
	pTmp34->pBone[16].Q = Quat(ROT_XYZ, -0.117776f, -0.753729f, 0.414224f);
	pTmp34->pBone[16].S = Vect(1.000000f, 1.000000f, 1.000001f);

	//     Node Name: Bip01 R Forearm
	pTmp34->pBone[17].T = Vect(15.099410f, -0.025220f, -0.006940f);
	pTmp34->pBone[17].Q = Quat(ROT_XYZ, -0.960760f, -0.034712f, -0.259805f);
	pTmp34->pBone[17].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Hand
	pTmp34->pBone[18].T = Vect(15.099400f, 0.000184f, -0.000110f);
	pTmp34->pBone[18].Q = Quat(ROT_XYZ, 1.556102f, -0.000650f, -0.183286f);
	pTmp34->pBone[18].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Head
	pTmp34->pBone[19].T = Vect(11.079190f, 0.000001f, 0.000001f);
	pTmp34->pBone[19].Q = Quat(ROT_XYZ, 0.118432f, 0.036183f, 0.279520f);
	pTmp34->pBone[19].S = Vect(1.000000f, 1.000000f, 1.000000f);



	//------- Key Time: 35  ms: 1166 ------------------ 

	Frame_Bucket *pTmp35 = new Frame_Bucket();
	pTmp35->prevBucket = pTmp34;
	pTmp35->nextBucket = 0;
	pTmp35->KeyTime = 35 * Time(TIME_NTSC_30_FRAME);
	pTmp35->pBone = new Bone[NUM_BONES];
	pTmp34->nextBucket = pTmp35;

	//     Node Name: RootNode
	//     Node Name: Bip01
	pTmp35->pBone[0].T = Vect(-1.743190f, -0.423828f, -1.576679f);
	pTmp35->pBone[0].Q = Quat(ROT_XYZ, 1.720291f, -1.349081f, 2.975647f);
	pTmp35->pBone[0].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Pelvis
	pTmp35->pBone[1].T = Vect(0.0f, 0.0f, 0.0f);
	pTmp35->pBone[1].Q = Quat(ROT_XYZ, 0.0f, 0.0f, 0.0f);
	pTmp35->pBone[1].S = Vect(1.0f, 1.0f, 1.0f);

	//     Node Name: Bip01 Spine
	pTmp35->pBone[2].T = Vect(6.016912f, -0.014115f, 0.000003f);
	pTmp35->pBone[2].Q = Quat(ROT_XYZ, -0.018790f, -0.012725f, 0.011367f);
	pTmp35->pBone[2].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Thigh
	pTmp35->pBone[3].T = Vect(-5.841920f, -0.175780f, 13.745930f);
	pTmp35->pBone[3].Q = Quat(ROT_XYZ, 3.073161f, -0.117379f, 2.736919f);
	pTmp35->pBone[3].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Calf
	pTmp35->pBone[4].T = Vect(16.961559f, -0.000002f, -0.000004f);
	pTmp35->pBone[4].Q = Quat(ROT_XYZ, -0.217510f, -0.020214f, -0.992974f);
	pTmp35->pBone[4].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Foot
	pTmp35->pBone[5].T = Vect(13.263550f, 0.000003f, 0.000002f);
	pTmp35->pBone[5].Q = Quat(ROT_XYZ, -0.035641f, -0.075892f, 0.547680f);
	pTmp35->pBone[5].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Thigh
	pTmp35->pBone[6].T = Vect(-6.189781f, 0.337890f, -13.589760f);
	pTmp35->pBone[6].Q = Quat(ROT_XYZ, 3.137179f, 0.173454f, 2.701841f);
	pTmp35->pBone[6].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Calf
	pTmp35->pBone[7].T = Vect(16.961571f, 0.000003f, -0.000003f);
	pTmp35->pBone[7].Q = Quat(ROT_XYZ, -0.115390f, 0.009399f, -1.501093f);
	pTmp35->pBone[7].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Foot
	pTmp35->pBone[8].T = Vect(13.263530f, -0.000000f, 0.000002f);
	pTmp35->pBone[8].Q = Quat(ROT_XYZ, -0.008936f, -0.030732f, 0.380502f);
	pTmp35->pBone[8].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Spine1
	pTmp35->pBone[9].T = Vect(17.730431f, -0.017629f, 0.000002f);
	pTmp35->pBone[9].Q = Quat(ROT_XYZ, -0.037711f, -0.025246f, 0.021381f);
	pTmp35->pBone[9].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Neck
	pTmp35->pBone[10].T = Vect(22.140881f, -0.008821f, 0.000003f);
	pTmp35->pBone[10].Q = Quat(ROT_XYZ, 0.132547f, 0.072374f, -0.088049f);
	pTmp35->pBone[10].S = Vect(1.000000f, 1.000000f, 1.000001f);

	//     Node Name: Bip01 L Clavicle
	pTmp35->pBone[11].T = Vect(-0.395692f, 0.728581f, 5.398170f);
	pTmp35->pBone[11].Q = Quat(ROT_XYZ, 0.798327f, -1.006933f, 2.403643f);
	pTmp35->pBone[11].S = Vect(0.999999f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L UpperArm
	pTmp35->pBone[12].T = Vect(13.067860f, 0.000004f, -0.000004f);
	pTmp35->pBone[12].Q = Quat(ROT_XYZ, -0.032819f, 0.782153f, 0.233908f);
	pTmp35->pBone[12].S = Vect(1.000000f, 1.000001f, 1.000001f);

	//     Node Name: Bip01 L Forearm
	pTmp35->pBone[13].T = Vect(15.099380f, -0.000549f, 0.000112f);
	pTmp35->pBone[13].Q = Quat(ROT_XYZ, 0.771123f, 0.027996f, -0.390859f);
	pTmp35->pBone[13].S = Vect(1.000000f, 1.000001f, 1.000001f);

	//     Node Name: Bip01 L Hand
	pTmp35->pBone[14].T = Vect(15.099410f, 0.000058f, 0.000054f);
	pTmp35->pBone[14].Q = Quat(ROT_XYZ, -1.576220f, 0.123043f, -0.128343f);
	pTmp35->pBone[14].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Clavicle
	pTmp35->pBone[15].T = Vect(0.394143f, -0.711174f, -5.400606f);
	pTmp35->pBone[15].Q = Quat(ROT_XYZ, -0.514079f, 1.006188f, 2.566810f);
	pTmp35->pBone[15].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R UpperArm
	pTmp35->pBone[16].T = Vect(13.067840f, -0.000001f, 0.000001f);
	pTmp35->pBone[16].Q = Quat(ROT_XYZ, -0.122532f, -0.765876f, 0.417622f);
	pTmp35->pBone[16].S = Vect(1.000000f, 1.000000f, 1.000001f);

	//     Node Name: Bip01 R Forearm
	pTmp35->pBone[17].T = Vect(15.099410f, 0.000452f, 0.000201f);
	pTmp35->pBone[17].Q = Quat(ROT_XYZ, -0.977760f, -0.032223f, -0.239256f);
	pTmp35->pBone[17].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Hand
	pTmp35->pBone[18].T = Vect(15.099400f, 0.000184f, -0.000110f);
	pTmp35->pBone[18].Q = Quat(ROT_XYZ, 1.557878f, -0.014141f, -0.188850f);
	pTmp35->pBone[18].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Head
	pTmp35->pBone[19].T = Vect(11.079190f, 0.000001f, 0.000001f);
	pTmp35->pBone[19].Q = Quat(ROT_XYZ, 0.135169f, 0.020157f, 0.272915f);
	pTmp35->pBone[19].S = Vect(1.000000f, 1.000000f, 1.000000f);



	//------- Key Time: 36  ms: 1200 ------------------ 

	Frame_Bucket *pTmp36 = new Frame_Bucket();
	pTmp36->prevBucket = pTmp35;
	pTmp36->nextBucket = 0;
	pTmp36->KeyTime = 36 * Time(TIME_NTSC_30_FRAME);
	pTmp36->pBone = new Bone[NUM_BONES];
	pTmp35->nextBucket = pTmp36;

	//     Node Name: RootNode
	//     Node Name: Bip01
	pTmp36->pBone[0].T = Vect(-1.533935f, -0.445911f, -0.680915f);
	pTmp36->pBone[0].Q = Quat(ROT_XYZ, 1.709089f, -1.334329f, 2.992805f);
	pTmp36->pBone[0].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Pelvis
	pTmp36->pBone[1].T = Vect(0.0f, 0.0f, 0.0f);
	pTmp36->pBone[1].Q = Quat(ROT_XYZ, 0.0f, 0.0f, 0.0f);
	pTmp36->pBone[1].S = Vect(1.0f, 1.0f, 1.0f);

	//     Node Name: Bip01 Spine
	pTmp36->pBone[2].T = Vect(6.016911f, -0.014115f, 0.000003f);
	pTmp36->pBone[2].Q = Quat(ROT_XYZ, -0.029792f, -0.003091f, 0.015317f);
	pTmp36->pBone[2].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Thigh
	pTmp36->pBone[3].T = Vect(-5.973720f, -0.301548f, 13.686980f);
	pTmp36->pBone[3].Q = Quat(ROT_XYZ, 3.052751f, -0.079660f, 2.825991f);
	pTmp36->pBone[3].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Calf
	pTmp36->pBone[4].T = Vect(16.961559f, -0.000002f, -0.000004f);
	pTmp36->pBone[4].Q = Quat(ROT_XYZ, -0.253079f, -0.022534f, -0.963704f);
	pTmp36->pBone[4].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Foot
	pTmp36->pBone[5].T = Vect(13.263550f, 0.000003f, 0.000002f);
	pTmp36->pBone[5].Q = Quat(ROT_XYZ, -0.044815f, -0.087486f, 0.610945f);
	pTmp36->pBone[5].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Thigh
	pTmp36->pBone[6].T = Vect(-6.058175f, 0.512885f, -13.643500f);
	pTmp36->pBone[6].Q = Quat(ROT_XYZ, 3.128632f, 0.152905f, 2.578042f);
	pTmp36->pBone[6].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Calf
	pTmp36->pBone[7].T = Vect(16.961571f, 0.000003f, -0.000003f);
	pTmp36->pBone[7].Q = Quat(ROT_XYZ, -0.099497f, 0.009806f, -1.423186f);
	pTmp36->pBone[7].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Foot
	pTmp36->pBone[8].T = Vect(13.263530f, -0.000000f, 0.000002f);
	pTmp36->pBone[8].Q = Quat(ROT_XYZ, -0.006365f, -0.019536f, 0.369756f);
	pTmp36->pBone[8].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Spine1
	pTmp36->pBone[9].T = Vect(17.730431f, -0.017629f, 0.000002f);
	pTmp36->pBone[9].Q = Quat(ROT_XYZ, -0.059618f, -0.005745f, 0.029126f);
	pTmp36->pBone[9].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Neck
	pTmp36->pBone[10].T = Vect(22.140881f, -0.008821f, 0.000003f);
	pTmp36->pBone[10].Q = Quat(ROT_XYZ, 0.159313f, 0.059960f, -0.090601f);
	pTmp36->pBone[10].S = Vect(1.000000f, 1.000000f, 1.000001f);

	//     Node Name: Bip01 L Clavicle
	pTmp36->pBone[11].T = Vect(-0.328077f, 0.873503f, 5.381174f);
	pTmp36->pBone[11].Q = Quat(ROT_XYZ, 0.865077f, -0.994274f, 2.344579f);
	pTmp36->pBone[11].S = Vect(0.999999f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L UpperArm
	pTmp36->pBone[12].T = Vect(13.067860f, 0.000004f, -0.000004f);
	pTmp36->pBone[12].Q = Quat(ROT_XYZ, -0.057868f, 0.758340f, 0.186030f);
	pTmp36->pBone[12].S = Vect(1.000000f, 1.000001f, 1.000001f);

	//     Node Name: Bip01 L Forearm
	pTmp36->pBone[13].T = Vect(15.099380f, -0.001435f, 0.000392f);
	pTmp36->pBone[13].Q = Quat(ROT_XYZ, 0.750340f, 0.030648f, -0.424212f);
	pTmp36->pBone[13].S = Vect(1.000000f, 1.000001f, 1.000001f);

	//     Node Name: Bip01 L Hand
	pTmp36->pBone[14].T = Vect(15.099410f, 0.000057f, 0.000054f);
	pTmp36->pBone[14].Q = Quat(ROT_XYZ, -1.574819f, 0.114954f, -0.138394f);
	pTmp36->pBone[14].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Clavicle
	pTmp36->pBone[15].T = Vect(0.326487f, -0.856169f, -5.384058f);
	pTmp36->pBone[15].Q = Quat(ROT_XYZ, -0.480838f, 1.034937f, 2.602965f);
	pTmp36->pBone[15].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R UpperArm
	pTmp36->pBone[16].T = Vect(13.067840f, -0.000001f, 0.000001f);
	pTmp36->pBone[16].Q = Quat(ROT_XYZ, -0.118233f, -0.776683f, 0.418033f);
	pTmp36->pBone[16].S = Vect(1.000000f, 1.000000f, 1.000001f);

	//     Node Name: Bip01 R Forearm
	pTmp36->pBone[17].T = Vect(15.099410f, -0.000956f, -0.000211f);
	pTmp36->pBone[17].Q = Quat(ROT_XYZ, -0.965337f, -0.029971f, -0.229715f);
	pTmp36->pBone[17].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Hand
	pTmp36->pBone[18].T = Vect(15.099400f, 0.000184f, -0.000110f);
	pTmp36->pBone[18].Q = Quat(ROT_XYZ, 1.559185f, -0.023492f, -0.180150f);
	pTmp36->pBone[18].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Head
	pTmp36->pBone[19].T = Vect(11.079190f, 0.000001f, 0.000001f);
	pTmp36->pBone[19].Q = Quat(ROT_XYZ, 0.149430f, 0.007336f, 0.269055f);
	pTmp36->pBone[19].S = Vect(1.000000f, 1.000000f, 1.000000f);



	//------- Key Time: 37  ms: 1233 ------------------ 

	Frame_Bucket *pTmp37 = new Frame_Bucket();
	pTmp37->prevBucket = pTmp36;
	pTmp37->nextBucket = 0;
	pTmp37->KeyTime = 37 * Time(TIME_NTSC_30_FRAME);
	pTmp37->pBone = new Bone[NUM_BONES];
	pTmp36->nextBucket = pTmp37;

	//     Node Name: RootNode
	//     Node Name: Bip01
	pTmp37->pBone[0].T = Vect(-1.023884f, -0.720733f, -0.051928f);
	pTmp37->pBone[0].Q = Quat(ROT_XYZ, 1.673177f, -1.312122f, 3.038853f);
	pTmp37->pBone[0].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Pelvis
	pTmp37->pBone[1].T = Vect(0.0f, 0.0f, 0.0f);
	pTmp37->pBone[1].Q = Quat(ROT_XYZ, 0.0f, 0.0f, 0.0f);
	pTmp37->pBone[1].S = Vect(1.0f, 1.0f, 1.0f);

	//     Node Name: Bip01 Spine
	pTmp37->pBone[2].T = Vect(6.016910f, -0.014115f, 0.000003f);
	pTmp37->pBone[2].Q = Quat(ROT_XYZ, -0.037280f, 0.006549f, 0.016807f);
	pTmp37->pBone[2].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Thigh
	pTmp37->pBone[3].T = Vect(-6.105210f, -0.392888f, 13.626490f);
	pTmp37->pBone[3].Q = Quat(ROT_XYZ, 3.038000f, -0.035989f, 2.914043f);
	pTmp37->pBone[3].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Calf
	pTmp37->pBone[4].T = Vect(16.961559f, -0.000002f, -0.000004f);
	pTmp37->pBone[4].Q = Quat(ROT_XYZ, -0.284711f, -0.024103f, -0.921893f);
	pTmp37->pBone[4].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Foot
	pTmp37->pBone[5].T = Vect(13.263550f, 0.000003f, 0.000002f);
	pTmp37->pBone[5].Q = Quat(ROT_XYZ, -0.053243f, -0.099607f, 0.656362f);
	pTmp37->pBone[5].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Thigh
	pTmp37->pBone[6].T = Vect(-5.926094f, 0.626139f, -13.696660f);
	pTmp37->pBone[6].Q = Quat(ROT_XYZ, 3.122138f, 0.129636f, 2.461617f);
	pTmp37->pBone[6].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Calf
	pTmp37->pBone[7].T = Vect(16.961571f, 0.000003f, -0.000003f);
	pTmp37->pBone[7].Q = Quat(ROT_XYZ, -0.087002f, 0.008974f, -1.298073f);
	pTmp37->pBone[7].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Foot
	pTmp37->pBone[8].T = Vect(13.263530f, -0.000000f, 0.000002f);
	pTmp37->pBone[8].Q = Quat(ROT_XYZ, -0.005294f, -0.011948f, 0.337588f);
	pTmp37->pBone[8].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Spine1
	pTmp37->pBone[9].T = Vect(17.730431f, -0.017629f, 0.000002f);
	pTmp37->pBone[9].Q = Quat(ROT_XYZ, -0.074444f, 0.013692f, 0.031767f);
	pTmp37->pBone[9].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Neck
	pTmp37->pBone[10].T = Vect(22.140881f, -0.008821f, 0.000003f);
	pTmp37->pBone[10].Q = Quat(ROT_XYZ, 0.183285f, 0.038452f, -0.096242f);
	pTmp37->pBone[10].S = Vect(1.000000f, 1.000000f, 1.000001f);

	//     Node Name: Bip01 L Clavicle
	pTmp37->pBone[11].T = Vect(-0.210796f, 1.003287f, 5.364385f);
	pTmp37->pBone[11].Q = Quat(ROT_XYZ, 0.919372f, -0.992072f, 2.302192f);
	pTmp37->pBone[11].S = Vect(0.999999f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L UpperArm
	pTmp37->pBone[12].T = Vect(13.067860f, 0.000004f, -0.000004f);
	pTmp37->pBone[12].Q = Quat(ROT_XYZ, -0.085674f, 0.725114f, 0.135611f);
	pTmp37->pBone[12].S = Vect(1.000000f, 1.000001f, 1.000001f);

	//     Node Name: Bip01 L Forearm
	pTmp37->pBone[13].T = Vect(15.099380f, -0.001741f, 0.000471f);
	pTmp37->pBone[13].Q = Quat(ROT_XYZ, 0.742681f, 0.033051f, -0.453541f);
	pTmp37->pBone[13].S = Vect(1.000000f, 1.000001f, 1.000001f);

	//     Node Name: Bip01 L Hand
	pTmp37->pBone[14].T = Vect(15.099410f, 0.000057f, 0.000054f);
	pTmp37->pBone[14].Q = Quat(ROT_XYZ, -1.571077f, 0.085883f, -0.150606f);
	pTmp37->pBone[14].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Clavicle
	pTmp37->pBone[15].T = Vect(0.209100f, -0.986031f, -5.367652f);
	pTmp37->pBone[15].Q = Quat(ROT_XYZ, -0.442709f, 1.053168f, 2.644949f);
	pTmp37->pBone[15].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R UpperArm
	pTmp37->pBone[16].T = Vect(13.067840f, -0.000001f, 0.000001f);
	pTmp37->pBone[16].Q = Quat(ROT_XYZ, -0.107519f, -0.782806f, 0.418028f);
	pTmp37->pBone[16].S = Vect(1.000000f, 1.000000f, 1.000001f);

	//     Node Name: Bip01 R Forearm
	pTmp37->pBone[17].T = Vect(15.099410f, -0.001086f, -0.000248f);
	pTmp37->pBone[17].Q = Quat(ROT_XYZ, -0.935779f, -0.028962f, -0.225675f);
	pTmp37->pBone[17].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Hand
	pTmp37->pBone[18].T = Vect(15.099400f, 0.000184f, -0.000110f);
	pTmp37->pBone[18].Q = Quat(ROT_XYZ, 1.560712f, -0.034618f, -0.165270f);
	pTmp37->pBone[18].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Head
	pTmp37->pBone[19].T = Vect(11.079190f, 0.000001f, 0.000001f);
	pTmp37->pBone[19].Q = Quat(ROT_XYZ, 0.161885f, 0.003402f, 0.266034f);
	pTmp37->pBone[19].S = Vect(1.000000f, 1.000000f, 1.000000f);



	//------- Key Time: 38  ms: 1266 ------------------ 

	Frame_Bucket *pTmp38 = new Frame_Bucket();
	pTmp38->prevBucket = pTmp37;
	pTmp38->nextBucket = 0;
	pTmp38->KeyTime = 38 * Time(TIME_NTSC_30_FRAME);
	pTmp38->pBone = new Bone[NUM_BONES];
	pTmp37->nextBucket = pTmp38;

	//     Node Name: RootNode
	//     Node Name: Bip01
	pTmp38->pBone[0].T = Vect(-0.461423f, -1.187930f, 0.644831f);
	pTmp38->pBone[0].Q = Quat(ROT_XYZ, 1.648096f, -1.293243f, 3.074077f);
	pTmp38->pBone[0].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Pelvis
	pTmp38->pBone[1].T = Vect(0.0f, 0.0f, 0.0f);
	pTmp38->pBone[1].Q = Quat(ROT_XYZ, 0.0f, 0.0f, 0.0f);
	pTmp38->pBone[1].S = Vect(1.0f, 1.0f, 1.0f);

	//     Node Name: Bip01 Spine
	pTmp38->pBone[2].T = Vect(6.016910f, -0.014115f, 0.000003f);
	pTmp38->pBone[2].Q = Quat(ROT_XYZ, -0.042836f, 0.013710f, 0.016118f);
	pTmp38->pBone[2].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Thigh
	pTmp38->pBone[3].T = Vect(-6.202670f, -0.470864f, 13.579880f);
	pTmp38->pBone[3].Q = Quat(ROT_XYZ, 3.033513f, 0.003939f, 3.017046f);
	pTmp38->pBone[3].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Calf
	pTmp38->pBone[4].T = Vect(16.961559f, -0.000002f, -0.000004f);
	pTmp38->pBone[4].Q = Quat(ROT_XYZ, -0.301272f, -0.024523f, -0.860519f);
	pTmp38->pBone[4].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Foot
	pTmp38->pBone[5].T = Vect(13.263550f, 0.000003f, 0.000002f);
	pTmp38->pBone[5].Q = Quat(ROT_XYZ, -0.071549f, -0.136296f, 0.708503f);
	pTmp38->pBone[5].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Thigh
	pTmp38->pBone[6].T = Vect(-5.827764f, 0.699903f, -13.735180f);
	pTmp38->pBone[6].Q = Quat(ROT_XYZ, 3.119226f, 0.102400f, 2.379181f);
	pTmp38->pBone[6].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Calf
	pTmp38->pBone[7].T = Vect(16.961571f, 0.000003f, -0.000003f);
	pTmp38->pBone[7].Q = Quat(ROT_XYZ, -0.064340f, 0.007450f, -1.126059f);
	pTmp38->pBone[7].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Foot
	pTmp38->pBone[8].T = Vect(13.263530f, -0.000000f, 0.000002f);
	pTmp38->pBone[8].Q = Quat(ROT_XYZ, -0.004022f, 0.002626f, 0.298489f);
	pTmp38->pBone[8].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Spine1
	pTmp38->pBone[9].T = Vect(17.730431f, -0.017629f, 0.000002f);
	pTmp38->pBone[9].Q = Quat(ROT_XYZ, -0.085462f, 0.028064f, 0.030046f);
	pTmp38->pBone[9].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Neck
	pTmp38->pBone[10].T = Vect(22.140881f, -0.008821f, 0.000003f);
	pTmp38->pBone[10].Q = Quat(ROT_XYZ, 0.197206f, 0.026330f, -0.085534f);
	pTmp38->pBone[10].S = Vect(1.000000f, 1.000000f, 1.000001f);

	//     Node Name: Bip01 L Clavicle
	pTmp38->pBone[11].T = Vect(-0.144530f, 1.078298f, 5.352011f);
	pTmp38->pBone[11].Q = Quat(ROT_XYZ, 0.960103f, -0.995841f, 2.254461f);
	pTmp38->pBone[11].S = Vect(0.999999f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L UpperArm
	pTmp38->pBone[12].T = Vect(13.067860f, 0.000004f, -0.000004f);
	pTmp38->pBone[12].Q = Quat(ROT_XYZ, -0.106164f, 0.698521f, 0.102402f);
	pTmp38->pBone[12].S = Vect(1.000000f, 1.000001f, 1.000001f);

	//     Node Name: Bip01 L Forearm
	pTmp38->pBone[13].T = Vect(15.099380f, -0.030474f, 0.008233f);
	pTmp38->pBone[13].Q = Quat(ROT_XYZ, 0.725080f, 0.035416f, -0.490745f);
	pTmp38->pBone[13].S = Vect(1.000000f, 1.000001f, 1.000001f);

	//     Node Name: Bip01 L Hand
	pTmp38->pBone[14].T = Vect(15.099410f, 0.000057f, 0.000054f);
	pTmp38->pBone[14].Q = Quat(ROT_XYZ, -1.571149f, 0.082935f, -0.149433f);
	pTmp38->pBone[14].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Clavicle
	pTmp38->pBone[15].T = Vect(0.143023f, -1.061067f, -5.355498f);
	pTmp38->pBone[15].Q = Quat(ROT_XYZ, -0.414556f, 1.077618f, 2.667696f);
	pTmp38->pBone[15].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R UpperArm
	pTmp38->pBone[16].T = Vect(13.067840f, -0.000001f, 0.000001f);
	pTmp38->pBone[16].Q = Quat(ROT_XYZ, -0.093496f, -0.792985f, 0.414746f);
	pTmp38->pBone[16].S = Vect(1.000000f, 1.000000f, 1.000001f);

	//     Node Name: Bip01 R Forearm
	pTmp38->pBone[17].T = Vect(15.099410f, -0.000233f, 0.000015f);
	pTmp38->pBone[17].Q = Quat(ROT_XYZ, -0.894768f, -0.028313f, -0.220364f);
	pTmp38->pBone[17].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Hand
	pTmp38->pBone[18].T = Vect(15.099400f, 0.000184f, -0.000110f);
	pTmp38->pBone[18].Q = Quat(ROT_XYZ, 1.562571f, -0.045140f, -0.136568f);
	pTmp38->pBone[18].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Head
	pTmp38->pBone[19].T = Vect(11.079190f, 0.000001f, 0.000001f);
	pTmp38->pBone[19].Q = Quat(ROT_XYZ, 0.173032f, -0.009444f, 0.256315f);
	pTmp38->pBone[19].S = Vect(1.000000f, 1.000000f, 1.000000f);



	//------- Key Time: 39  ms: 1300 ------------------ 

	Frame_Bucket *pTmp39 = new Frame_Bucket();
	pTmp39->prevBucket = pTmp38;
	pTmp39->nextBucket = 0;
	pTmp39->KeyTime = 39 * Time(TIME_NTSC_30_FRAME);
	pTmp39->pBone = new Bone[NUM_BONES];
	pTmp38->nextBucket = pTmp39;

	//     Node Name: RootNode
	//     Node Name: Bip01
	pTmp39->pBone[0].T = Vect(0.279301f, -1.899856f, 1.491280f);
	pTmp39->pBone[0].Q = Quat(ROT_XYZ, 1.640464f, -1.275859f, 3.094093f);
	pTmp39->pBone[0].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Pelvis
	pTmp39->pBone[1].T = Vect(0.0f, 0.0f, 0.0f);
	pTmp39->pBone[1].Q = Quat(ROT_XYZ, 0.0f, 0.0f, 0.0f);
	pTmp39->pBone[1].S = Vect(1.0f, 1.0f, 1.0f);

	//     Node Name: Bip01 Spine
	pTmp39->pBone[2].T = Vect(6.016909f, -0.014115f, 0.000003f);
	pTmp39->pBone[2].Q = Quat(ROT_XYZ, -0.045650f, 0.017967f, 0.014005f);
	pTmp39->pBone[2].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Thigh
	pTmp39->pBone[3].T = Vect(-6.260730f, -0.520559f, 13.551420f);
	pTmp39->pBone[3].Q = Quat(ROT_XYZ, 3.044093f, 0.046785f, -3.141425f);
	pTmp39->pBone[3].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Calf
	pTmp39->pBone[4].T = Vect(16.961559f, -0.000002f, -0.000004f);
	pTmp39->pBone[4].Q = Quat(ROT_XYZ, -0.318205f, -0.023600f, -0.766581f);
	pTmp39->pBone[4].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Foot
	pTmp39->pBone[5].T = Vect(13.263550f, 0.000003f, 0.000002f);
	pTmp39->pBone[5].Q = Quat(ROT_XYZ, -0.098327f, -0.194069f, 0.748375f);
	pTmp39->pBone[5].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Thigh
	pTmp39->pBone[6].T = Vect(-5.769438f, 0.726976f, -13.758400f);
	pTmp39->pBone[6].Q = Quat(ROT_XYZ, 3.118444f, 0.079091f, 2.302287f);
	pTmp39->pBone[6].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Calf
	pTmp39->pBone[7].T = Vect(16.961571f, 0.000003f, -0.000003f);
	pTmp39->pBone[7].Q = Quat(ROT_XYZ, -0.047983f, 0.005107f, -0.967395f);
	pTmp39->pBone[7].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Foot
	pTmp39->pBone[8].T = Vect(13.263530f, -0.000000f, 0.000002f);
	pTmp39->pBone[8].Q = Quat(ROT_XYZ, -0.003638f, 0.015435f, 0.263338f);
	pTmp39->pBone[8].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Spine1
	pTmp39->pBone[9].T = Vect(17.730431f, -0.017629f, 0.000002f);
	pTmp39->pBone[9].Q = Quat(ROT_XYZ, -0.091074f, 0.036519f, 0.025590f);
	pTmp39->pBone[9].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Neck
	pTmp39->pBone[10].T = Vect(22.140881f, -0.008821f, 0.000003f);
	pTmp39->pBone[10].Q = Quat(ROT_XYZ, 0.204691f, 0.014372f, -0.074199f);
	pTmp39->pBone[10].S = Vect(1.000000f, 1.000000f, 1.000001f);

	//     Node Name: Bip01 L Clavicle
	pTmp39->pBone[11].T = Vect(-0.079138f, 1.118607f, 5.345102f);
	pTmp39->pBone[11].Q = Quat(ROT_XYZ, 1.000570f, -1.011123f, 2.205862f);
	pTmp39->pBone[11].S = Vect(0.999999f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L UpperArm
	pTmp39->pBone[12].T = Vect(13.067860f, 0.000004f, -0.000004f);
	pTmp39->pBone[12].Q = Quat(ROT_XYZ, -0.126164f, 0.680293f, 0.075574f);
	pTmp39->pBone[12].S = Vect(1.000000f, 1.000001f, 1.000001f);

	//     Node Name: Bip01 L Forearm
	pTmp39->pBone[13].T = Vect(15.099380f, -0.000646f, 0.000193f);
	pTmp39->pBone[13].Q = Quat(ROT_XYZ, 0.673361f, 0.039967f, -0.541411f);
	pTmp39->pBone[13].S = Vect(1.000000f, 1.000001f, 1.000001f);

	//     Node Name: Bip01 L Hand
	pTmp39->pBone[14].T = Vect(15.099410f, 0.000056f, 0.000054f);
	pTmp39->pBone[14].Q = Quat(ROT_XYZ, -1.576521f, 0.115601f, -0.140151f);
	pTmp39->pBone[14].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Clavicle
	pTmp39->pBone[15].T = Vect(0.077839f, -1.101382f, -5.348701f);
	pTmp39->pBone[15].Q = Quat(ROT_XYZ, -0.394932f, 1.101013f, 2.683398f);
	pTmp39->pBone[15].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R UpperArm
	pTmp39->pBone[16].T = Vect(13.067840f, -0.000001f, 0.000001f);
	pTmp39->pBone[16].Q = Quat(ROT_XYZ, -0.093041f, -0.805085f, 0.425593f);
	pTmp39->pBone[16].S = Vect(1.000000f, 1.000000f, 1.000001f);

	//     Node Name: Bip01 R Forearm
	pTmp39->pBone[17].T = Vect(15.099410f, -0.000695f, -0.000114f);
	pTmp39->pBone[17].Q = Quat(ROT_XYZ, -0.830764f, -0.028726f, -0.222806f);
	pTmp39->pBone[17].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Hand
	pTmp39->pBone[18].T = Vect(15.099400f, 0.000184f, -0.000110f);
	pTmp39->pBone[18].Q = Quat(ROT_XYZ, 1.564804f, -0.057982f, -0.114890f);
	pTmp39->pBone[18].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Head
	pTmp39->pBone[19].T = Vect(11.079190f, 0.000001f, 0.000001f);
	pTmp39->pBone[19].Q = Quat(ROT_XYZ, 0.182380f, -0.020001f, 0.256845f);
	pTmp39->pBone[19].S = Vect(1.000000f, 1.000000f, 1.000000f);



	//------- Key Time: 40  ms: 1333 ------------------ 

	Frame_Bucket *pTmp40 = new Frame_Bucket();
	pTmp40->prevBucket = pTmp39;
	pTmp40->nextBucket = 0;
	pTmp40->KeyTime = 40 * Time(TIME_NTSC_30_FRAME);
	pTmp40->pBone = new Bone[NUM_BONES];
	pTmp39->nextBucket = pTmp40;

	//     Node Name: RootNode
	//     Node Name: Bip01
	pTmp40->pBone[0].T = Vect(0.970241f, -2.711164f, 1.899865f);
	pTmp40->pBone[0].Q = Quat(ROT_XYZ, 1.624573f, -1.264373f, 3.121366f);
	pTmp40->pBone[0].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Pelvis
	pTmp40->pBone[1].T = Vect(0.0f, 0.0f, 0.0f);
	pTmp40->pBone[1].Q = Quat(ROT_XYZ, 0.0f, 0.0f, 0.0f);
	pTmp40->pBone[1].S = Vect(1.0f, 1.0f, 1.0f);

	//     Node Name: Bip01 Spine
	pTmp40->pBone[2].T = Vect(6.016909f, -0.014115f, 0.000003f);
	pTmp40->pBone[2].Q = Quat(ROT_XYZ, -0.047765f, 0.020883f, 0.010792f);
	pTmp40->pBone[2].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Thigh
	pTmp40->pBone[3].T = Vect(-6.300597f, -0.567644f, 13.531080f);
	pTmp40->pBone[3].Q = Quat(ROT_XYZ, 3.060995f, 0.095672f, -3.015694f);
	pTmp40->pBone[3].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Calf
	pTmp40->pBone[4].T = Vect(16.961559f, -0.000002f, -0.000004f);
	pTmp40->pBone[4].Q = Quat(ROT_XYZ, -0.348719f, -0.021303f, -0.650498f);
	pTmp40->pBone[4].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Foot
	pTmp40->pBone[5].T = Vect(13.263550f, 0.000003f, 0.000002f);
	pTmp40->pBone[5].Q = Quat(ROT_XYZ, -0.103779f, -0.201807f, 0.767062f);
	pTmp40->pBone[5].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Thigh
	pTmp40->pBone[6].T = Vect(-5.729619f, 0.737561f, -13.774520f);
	pTmp40->pBone[6].Q = Quat(ROT_XYZ, 3.118021f, 0.054987f, 2.213237f);
	pTmp40->pBone[6].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Calf
	pTmp40->pBone[7].T = Vect(16.961571f, 0.000003f, -0.000003f);
	pTmp40->pBone[7].Q = Quat(ROT_XYZ, -0.023962f, 0.004592f, -0.865712f);
	pTmp40->pBone[7].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Foot
	pTmp40->pBone[8].T = Vect(13.263530f, -0.000000f, 0.000002f);
	pTmp40->pBone[8].Q = Quat(ROT_XYZ, -0.001414f, 0.038321f, 0.268113f);
	pTmp40->pBone[8].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Spine1
	pTmp40->pBone[9].T = Vect(17.730431f, -0.017629f, 0.000002f);
	pTmp40->pBone[9].Q = Quat(ROT_XYZ, -0.095342f, 0.042221f, 0.018989f);
	pTmp40->pBone[9].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Neck
	pTmp40->pBone[10].T = Vect(22.140881f, -0.008821f, 0.000003f);
	pTmp40->pBone[10].Q = Quat(ROT_XYZ, 0.204811f, 0.010239f, -0.060075f);
	pTmp40->pBone[10].S = Vect(1.000000f, 1.000000f, 1.000001f);

	//     Node Name: Bip01 L Clavicle
	pTmp40->pBone[11].T = Vect(-0.056449f, 1.119313f, 5.345238f);
	pTmp40->pBone[11].Q = Quat(ROT_XYZ, 1.012052f, -1.036376f, 2.183302f);
	pTmp40->pBone[11].S = Vect(0.999999f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L UpperArm
	pTmp40->pBone[12].T = Vect(13.067860f, 0.000004f, -0.000004f);
	pTmp40->pBone[12].Q = Quat(ROT_XYZ, -0.141168f, 0.660791f, 0.057310f);
	pTmp40->pBone[12].S = Vect(1.000000f, 1.000001f, 1.000001f);

	//     Node Name: Bip01 L Forearm
	pTmp40->pBone[13].T = Vect(15.099380f, -0.012282f, 0.002727f);
	pTmp40->pBone[13].Q = Quat(ROT_XYZ, 0.645436f, 0.044599f, -0.613739f);
	pTmp40->pBone[13].S = Vect(1.000000f, 1.000001f, 1.000001f);

	//     Node Name: Bip01 L Hand
	pTmp40->pBone[14].T = Vect(15.099410f, 0.000056f, 0.000054f);
	pTmp40->pBone[14].Q = Quat(ROT_XYZ, -1.583675f, 0.166629f, -0.123963f);
	pTmp40->pBone[14].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Clavicle
	pTmp40->pBone[15].T = Vect(0.055393f, -1.102071f, -5.348833f);
	pTmp40->pBone[15].Q = Quat(ROT_XYZ, -0.402010f, 1.128657f, 2.673232f);
	pTmp40->pBone[15].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R UpperArm
	pTmp40->pBone[16].T = Vect(13.067840f, -0.000001f, 0.000001f);
	pTmp40->pBone[16].Q = Quat(ROT_XYZ, -0.103290f, -0.809167f, 0.449671f);
	pTmp40->pBone[16].S = Vect(1.000000f, 1.000000f, 1.000001f);

	//     Node Name: Bip01 R Forearm
	pTmp40->pBone[17].T = Vect(15.099410f, -0.000566f, -0.000081f);
	pTmp40->pBone[17].Q = Quat(ROT_XYZ, -0.818463f, -0.029294f, -0.231555f);
	pTmp40->pBone[17].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Hand
	pTmp40->pBone[18].T = Vect(15.099400f, 0.000184f, -0.000110f);
	pTmp40->pBone[18].Q = Quat(ROT_XYZ, 1.566849f, -0.070789f, -0.100040f);
	pTmp40->pBone[18].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Head
	pTmp40->pBone[19].T = Vect(11.079190f, 0.000001f, 0.000001f);
	pTmp40->pBone[19].Q = Quat(ROT_XYZ, 0.191348f, -0.036722f, 0.254347f);
	pTmp40->pBone[19].S = Vect(1.000000f, 1.000000f, 1.000000f);



	//------- Key Time: 41  ms: 1366 ------------------ 

	Frame_Bucket *pTmp41 = new Frame_Bucket();
	pTmp41->prevBucket = pTmp40;
	pTmp41->nextBucket = 0;
	pTmp41->KeyTime = 41 * Time(TIME_NTSC_30_FRAME);
	pTmp41->pBone = new Bone[NUM_BONES];
	pTmp40->nextBucket = pTmp41;

	//     Node Name: RootNode
	//     Node Name: Bip01
	pTmp41->pBone[0].T = Vect(0.970241f, -2.711164f, 1.899865f);
	pTmp41->pBone[0].Q = Quat(ROT_XYZ, 1.624573f, -1.264373f, 3.121366f);
	pTmp41->pBone[0].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Pelvis
	pTmp41->pBone[1].T = Vect(0.0f, 0.0f, 0.0f);
	pTmp41->pBone[1].Q = Quat(ROT_XYZ, 0.0f, 0.0f, 0.0f);
	pTmp41->pBone[1].S = Vect(1.0f, 1.0f, 1.0f);

	//     Node Name: Bip01 Spine
	pTmp41->pBone[2].T = Vect(6.016909f, -0.014115f, 0.000003f);
	pTmp41->pBone[2].Q = Quat(ROT_XYZ, -0.047765f, 0.020883f, 0.010792f);
	pTmp41->pBone[2].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Thigh
	pTmp41->pBone[3].T = Vect(-6.300597f, -0.567644f, 13.531080f);
	pTmp41->pBone[3].Q = Quat(ROT_XYZ, 3.060995f, 0.095672f, -3.015694f);
	pTmp41->pBone[3].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Calf
	pTmp41->pBone[4].T = Vect(16.961559f, -0.000002f, -0.000004f);
	pTmp41->pBone[4].Q = Quat(ROT_XYZ, -0.348719f, -0.021303f, -0.650498f);
	pTmp41->pBone[4].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Foot
	pTmp41->pBone[5].T = Vect(13.263550f, 0.000003f, 0.000002f);
	pTmp41->pBone[5].Q = Quat(ROT_XYZ, -0.103779f, -0.201807f, 0.767062f);
	pTmp41->pBone[5].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Thigh
	pTmp41->pBone[6].T = Vect(-5.729619f, 0.737561f, -13.774520f);
	pTmp41->pBone[6].Q = Quat(ROT_XYZ, 3.118021f, 0.054987f, 2.213237f);
	pTmp41->pBone[6].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Calf
	pTmp41->pBone[7].T = Vect(16.961571f, 0.000003f, -0.000003f);
	pTmp41->pBone[7].Q = Quat(ROT_XYZ, -0.023962f, 0.004592f, -0.865712f);
	pTmp41->pBone[7].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Foot
	pTmp41->pBone[8].T = Vect(13.263530f, -0.000000f, 0.000002f);
	pTmp41->pBone[8].Q = Quat(ROT_XYZ, -0.001414f, 0.038321f, 0.268113f);
	pTmp41->pBone[8].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Spine1
	pTmp41->pBone[9].T = Vect(17.730431f, -0.017629f, 0.000002f);
	pTmp41->pBone[9].Q = Quat(ROT_XYZ, -0.095342f, 0.042221f, 0.018989f);
	pTmp41->pBone[9].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Neck
	pTmp41->pBone[10].T = Vect(22.140881f, -0.008821f, 0.000003f);
	pTmp41->pBone[10].Q = Quat(ROT_XYZ, 0.204811f, 0.010239f, -0.060075f);
	pTmp41->pBone[10].S = Vect(1.000000f, 1.000000f, 1.000001f);

	//     Node Name: Bip01 L Clavicle
	pTmp41->pBone[11].T = Vect(-0.056449f, 1.119313f, 5.345238f);
	pTmp41->pBone[11].Q = Quat(ROT_XYZ, 1.012052f, -1.036376f, 2.183302f);
	pTmp41->pBone[11].S = Vect(0.999999f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L UpperArm
	pTmp41->pBone[12].T = Vect(13.067860f, 0.000004f, -0.000004f);
	pTmp41->pBone[12].Q = Quat(ROT_XYZ, -0.141168f, 0.660791f, 0.057310f);
	pTmp41->pBone[12].S = Vect(1.000000f, 1.000001f, 1.000001f);

	//     Node Name: Bip01 L Forearm
	pTmp41->pBone[13].T = Vect(15.099380f, -0.012282f, 0.002727f);
	pTmp41->pBone[13].Q = Quat(ROT_XYZ, 0.645436f, 0.044599f, -0.613739f);
	pTmp41->pBone[13].S = Vect(1.000000f, 1.000001f, 1.000001f);

	//     Node Name: Bip01 L Hand
	pTmp41->pBone[14].T = Vect(15.099410f, 0.000056f, 0.000054f);
	pTmp41->pBone[14].Q = Quat(ROT_XYZ, -1.583675f, 0.166629f, -0.123963f);
	pTmp41->pBone[14].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Clavicle
	pTmp41->pBone[15].T = Vect(0.055393f, -1.102071f, -5.348833f);
	pTmp41->pBone[15].Q = Quat(ROT_XYZ, -0.402010f, 1.128657f, 2.673232f);
	pTmp41->pBone[15].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R UpperArm
	pTmp41->pBone[16].T = Vect(13.067840f, -0.000001f, 0.000001f);
	pTmp41->pBone[16].Q = Quat(ROT_XYZ, -0.103290f, -0.809167f, 0.449671f);
	pTmp41->pBone[16].S = Vect(1.000000f, 1.000000f, 1.000001f);

	//     Node Name: Bip01 R Forearm
	pTmp41->pBone[17].T = Vect(15.099410f, -0.000566f, -0.000081f);
	pTmp41->pBone[17].Q = Quat(ROT_XYZ, -0.818463f, -0.029294f, -0.231555f);
	pTmp41->pBone[17].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Hand
	pTmp41->pBone[18].T = Vect(15.099400f, 0.000184f, -0.000110f);
	pTmp41->pBone[18].Q = Quat(ROT_XYZ, 1.566849f, -0.070789f, -0.100040f);
	pTmp41->pBone[18].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Head
	pTmp41->pBone[19].T = Vect(11.079190f, 0.000001f, 0.000001f);
	pTmp41->pBone[19].Q = Quat(ROT_XYZ, 0.191348f, -0.036722f, 0.254347f);
	pTmp41->pBone[19].S = Vect(1.000000f, 1.000000f, 1.000000f);

}

void SetAnimationData()
{
	// --------  Result Frame  ----------------------

	anim[1].bucketList = new Frame_Bucket();
	anim[1].bucketList->prevBucket = 0;
	anim[1].bucketList->nextBucket = 0;
	anim[1].bucketList->KeyTime = Time(TIME_ZERO);
	anim[1].bucketList->pBone = new Bone[NUM_BONES];

	// --------  Frame 0  ----------------------------

	Frame_Bucket *pTmp = new Frame_Bucket();
	pTmp->prevBucket = anim[1].bucketList;
	pTmp->nextBucket = 0;
	pTmp->KeyTime = 0 * Time(TIME_NTSC_30_FRAME);
	pTmp->pBone = new Bone[NUM_BONES];
	anim[1].bucketList->nextBucket = pTmp;

	//------- Key Time: 0  ms: 0 ------------------ 

	//     Node Name: RootNode
	//     Node Name: Bip01
	pTmp->pBone[0].T = Vect(0.970236f, -2.711161f, 1.899864f);
	pTmp->pBone[0].Q = Quat(ROT_XYZ, 1.624573f, -1.264373f, 3.121366f);
	pTmp->pBone[0].S = Vect(0.100000f, 0.100000f, 0.100000f);
	pTmp->pBone[0].S = Vect(1.0f, 1.0f, 1.0f);

	//     Node Name: Bip01 Pelvis
	pTmp->pBone[1].T = Vect(0.0f, 0.0f, 0.0f);
	pTmp->pBone[1].Q = Quat(ROT_XYZ, 0.0f, 0.0f, 0.0f);
	pTmp->pBone[1].S = Vect(1.0f, 1.0f, 1.0f);

	//     Node Name: Bip01 Spine
	pTmp->pBone[2].T = Vect(6.016934f, -0.014115f, -0.000028f);
	pTmp->pBone[2].Q = Quat(ROT_XYZ, -0.047765f, 0.020883f, 0.010792f);
	pTmp->pBone[2].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Thigh
	pTmp->pBone[3].T = Vect(-6.300546f, -0.567644f, 13.531080f);
	pTmp->pBone[3].Q = Quat(ROT_XYZ, 3.060995f, 0.095672f, -3.015697f);
	pTmp->pBone[3].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Calf
	pTmp->pBone[4].T = Vect(16.961559f, -0.000002f, -0.000004f);
	pTmp->pBone[4].Q = Quat(ROT_XYZ, -0.348718f, -0.021303f, -0.650505f);
	pTmp->pBone[4].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L Foot
	pTmp->pBone[5].T = Vect(13.263550f, 0.000003f, 0.000002f);
	pTmp->pBone[5].Q = Quat(ROT_XYZ, -0.103780f, -0.201808f, 0.767066f);
	pTmp->pBone[5].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Thigh
	pTmp->pBone[6].T = Vect(-5.729567f, 0.737561f, -13.774520f);
	pTmp->pBone[6].Q = Quat(ROT_XYZ, 3.118021f, 0.054987f, 2.213239f);
	pTmp->pBone[6].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Calf
	pTmp->pBone[7].T = Vect(16.961571f, 0.000003f, -0.000003f);
	pTmp->pBone[7].Q = Quat(ROT_XYZ, -0.023962f, 0.004592f, -0.865707f);
	pTmp->pBone[7].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Foot
	pTmp->pBone[8].T = Vect(13.263530f, -0.000000f, 0.000002f);
	pTmp->pBone[8].Q = Quat(ROT_XYZ, -0.001414f, 0.038321f, 0.268111f);
	pTmp->pBone[8].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Spine1
	pTmp->pBone[9].T = Vect(17.730431f, -0.017629f, 0.000002f);
	pTmp->pBone[9].Q = Quat(ROT_XYZ, -0.095342f, 0.042221f, 0.018989f);
	pTmp->pBone[9].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Neck
	pTmp->pBone[10].T = Vect(22.140881f, -0.008821f, 0.000003f);
	pTmp->pBone[10].Q = Quat(ROT_XYZ, 0.204811f, 0.010239f, -0.060075f);
	pTmp->pBone[10].S = Vect(1.000000f, 1.000000f, 1.000001f);

	//     Node Name: Bip01 L Clavicle
	pTmp->pBone[11].T = Vect(-0.056449f, 1.119313f, 5.345238f);
	pTmp->pBone[11].Q = Quat(ROT_XYZ, 1.012052f, -1.036376f, 2.183302f);
	pTmp->pBone[11].S = Vect(0.999999f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 L UpperArm
	pTmp->pBone[12].T = Vect(13.067860f, 0.000004f, -0.000004f);
	pTmp->pBone[12].Q = Quat(ROT_XYZ, -0.141163f, 0.660794f, 0.057318f);
	pTmp->pBone[12].S = Vect(1.000000f, 1.000001f, 1.000001f);

	//     Node Name: Bip01 L Forearm
	pTmp->pBone[13].T = Vect(15.099380f, -0.012400f, 0.002757f);
	pTmp->pBone[13].Q = Quat(ROT_XYZ, 0.645437f, 0.044597f, -0.613745f);
	pTmp->pBone[13].S = Vect(1.000000f, 1.000001f, 1.000001f);

	//     Node Name: Bip01 L Hand
	pTmp->pBone[14].T = Vect(15.099410f, 0.000082f, 0.000072f);
	pTmp->pBone[14].Q = Quat(ROT_XYZ, -1.583676f, 0.166629f, -0.123964f);
	pTmp->pBone[14].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Clavicle
	pTmp->pBone[15].T = Vect(0.055393f, -1.102072f, -5.348833f);
	pTmp->pBone[15].Q = Quat(ROT_XYZ, -0.402011f, 1.128657f, 2.673232f);
	pTmp->pBone[15].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R UpperArm
	pTmp->pBone[16].T = Vect(13.067840f, -0.000001f, 0.000001f);
	pTmp->pBone[16].Q = Quat(ROT_XYZ, -0.103288f, -0.809167f, 0.449669f);
	pTmp->pBone[16].S = Vect(1.000000f, 1.000000f, 1.000001f);

	//     Node Name: Bip01 R Forearm
	pTmp->pBone[17].T = Vect(15.099410f, -0.000577f, -0.000088f);
	pTmp->pBone[17].Q = Quat(ROT_XYZ, -0.818463f, -0.029294f, -0.231554f);
	pTmp->pBone[17].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 R Hand
	pTmp->pBone[18].T = Vect(15.099400f, 0.000180f, -0.000104f);
	pTmp->pBone[18].Q = Quat(ROT_XYZ, 1.566849f, -0.070789f, -0.100040f);
	pTmp->pBone[18].S = Vect(1.000000f, 1.000000f, 1.000000f);

	//     Node Name: Bip01 Head
	pTmp->pBone[19].T = Vect(11.079190f, 0.000001f, 0.000001f);
	pTmp->pBone[19].Q = Quat(ROT_XYZ, 0.191348f, -0.036722f, 0.254348f);
	pTmp->pBone[19].S = Vect(1.000000f, 1.000000f, 1.000000f);
}

