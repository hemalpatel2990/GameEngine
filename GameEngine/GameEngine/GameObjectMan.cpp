#include <assert.h>

#include "NullModel.h"
#include "GraphicsObject_Null.h"

#include "GameObject.h"
#include "GameObjectMan.h"
#include "PCSTreeForwardIterator.h"

void GameObjectMan::Add(GameObject *pObj, GameObject *pParent)
{
	assert(pObj != 0);
	assert(pParent != 0);

	// Get singleton
	GameObjectMan *pGOM = GameObjectMan::privGetInstance();

	// insert object to root
	pGOM->pRootTree->insert(pObj, pParent);

}

PCSTree *GameObjectMan::GetPCSTree()
{
	// Get singleton
	GameObjectMan *pGOM = GameObjectMan::privGetInstance();
	assert(pGOM);

	// Get root node
	return pGOM->pRootTree;
}


GameObject * GameObjectMan::GetRoot()
{
	// Get singleton
	GameObjectMan *pGOM = GameObjectMan::privGetInstance();
	assert(pGOM);

	GameObject *pGameObj = (GameObject *)pGOM->pRootTree->getRoot();
	assert(pGameObj);

	return pGameObj;
}



void GameObjectMan::Update(Time currentTime)
{
	GameObjectMan *pGOM = GameObjectMan::privGetInstance();
	assert( pGOM );

	PCSNode *pRootNode = pGOM->pRootTree->getRoot();
	assert( pRootNode );

	PCSTreeForwardIterator pIter(pRootNode->getChild());

	PCSNode *pNode = pIter.First();

	GameObject *pGameObj = 0;

	while (!pIter.IsDone())
	{
		assert(pNode);
		// Update the game object
		pGameObj = (GameObject *)pNode;
		pGameObj->Process(currentTime);

		pNode = pIter.Next();
	}
}

void GameObjectMan::Draw()
{
	GameObjectMan *pGOM = GameObjectMan::privGetInstance();
	assert(pGOM);

	PCSNode *pRootNode = pGOM->pRootTree->getRoot();
	assert(pRootNode);

	PCSTreeForwardIterator pIter(pRootNode->getChild());

	PCSNode *pNode = pIter.First();

	GameObject *pGameObj = 0;

	while (!pIter.IsDone())
	{
		assert(pNode);
		// Update the game object
		pGameObj = (GameObject *)pNode;
		pGameObj->Draw();

		pNode = pIter.Next();
	}
}

GameObjectMan::GameObjectMan( )
{
	// Create the root node (null object)
	NullModel *pModel = new NullModel(0);
	ShaderObject *pShader = new ShaderObject(ShaderName::NULL_SHADER, "nullRender");
	GraphicsObject_Null *pGraphicsObject = new GraphicsObject_Null(pModel, pShader);
	GameObject *pGameRoot = new GameObject(pGraphicsObject);
	pGameRoot->setName("GameObject_Root");

	// Create the tree
	this->pRootTree = new PCSTree();
	assert(this->pRootTree);

	// Attach the root node
	this->pRootTree->insert(pGameRoot, this->pRootTree->getRoot());


}
	
GameObjectMan * GameObjectMan::privGetInstance( void )
{
	// This is where its actually stored (BSS section)
	static GameObjectMan gom;
	return &gom;
}
