#include <math.h>
#include <assert.h>

#include "Game.h"
#include "MathEngine.h"
#include "GameObject.h"
#include "Anim.h"
#include "FrameBucket.h"

extern Frame_Bucket *pHead;

GameObject::GameObject(GraphicsObject  *graphicsObject)
	: pGraphicsObject(graphicsObject)
{
	assert(graphicsObject != 0);

	this->pWorld = new Matrix(IDENTITY);
	assert(pWorld);

	this->pDof = new Vect(0.0f, 0.0f, 0.0f);
	assert(pDof);

	this->pUp = new Vect(0.0f, 1.0f, 0.0f);
	assert(pUp);

	this->pScale = new Vect(1.0f, 1.0f, 1.0f);
	assert(pScale);

	this->pPos = new Vect(0.0f, 0.0f, 0.0f);
	assert(pPos);

	this->pLocal = new Matrix(IDENTITY);
	assert(pLocal);

	this->pBoneOrientation = new Matrix(IDENTITY);
	assert(pBoneOrientation);


//	this->pBoneParent = 0;
	this->indexBoneArray = 0;

}

void GameObject::setIndex(int val)
{
	this->indexBoneArray = val;
}

GraphicsObject *GameObject::getGraphicsObject()
{
	return this->pGraphicsObject;
}

GameObject::~GameObject()
{
	delete this->pWorld;
}

Matrix *GameObject::getWorld()
{
	return this->pWorld;
}

void GameObject::setBoneOrientation(const Matrix &tmp)
{
	*this->pBoneOrientation = tmp;
}

Matrix GameObject::getBoneOrientation(void) const
{
	return Matrix(*this->pBoneOrientation);
}

void GameObject::Update(Time currentTime)
{
	GameObject *pBoneParent = (GameObject *) this->getParent();
	assert(pBoneParent != 0);

	Matrix ParentWorld = *pBoneParent->getWorld();

	// REMEMBER this is for Animation and hierachy, you need to handle models differently
	// Get the result bone array, from there make the matrix
	Bone *bResult = pHead->pBone;

	Matrix T = Matrix(TRANS, bResult[indexBoneArray].T);
	Matrix S = Matrix(SCALE, bResult[indexBoneArray].S);
	Quat   Q = bResult[indexBoneArray].Q;

	// Isn't awesome that we can multiply Quat with matrices!
	Matrix M = S * Q * T;
	*this->pLocal = M;

	// Goal: update the world matrix
	*this->pWorld = *this->pLocal * ParentWorld;
}

void GameObject::Process(Time currentTime)
{
	// Goal: update the world matrix
	this->Update(currentTime);

	// push to graphics object
	Matrix mTmp = *this->pBoneOrientation;

	// push to graphics object
	this->pGraphicsObject->SetWorld(mTmp);
}

void GameObject::setWorld(Matrix *world)
{
	assert(world);
	*this->pWorld = *world;
}


void GameObject::Draw()
{
	assert(this->pGraphicsObject);
	this->pGraphicsObject->Render();
}






