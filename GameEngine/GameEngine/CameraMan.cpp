#include <assert.h>

#include "Camera.h"
#include "CameraNode.h"
#include "CameraMan.h"

CameraMan::CameraMan()
{
	this->active = 0;
	this->currCamera = 0;
}

CameraMan * CameraMan::privGetInstance()
{
	// This is where its actually stored (BSS section)
	static CameraMan cameraMan;
	return &cameraMan;
}


void CameraMan::Add(Camera::Name name, Camera *pCamera)
{
	// Get the instance to the manager
	CameraMan *pCameraMan = CameraMan::privGetInstance();
	assert(pCameraMan);

	// Create a TextureNode
	CameraNode *pNode = new CameraNode();
	assert(pNode);

	// initialize it
	assert(pCamera);
	pCamera->setName(name);
	pNode->set( pCamera );

	// Now add it to the manager
	pCameraMan->privAddToFront( pNode, pCameraMan->active );
}

Camera * CameraMan::Find( Camera::Name _name)
{
	// Get the instance to the manager
	CameraMan *pCameraMan = CameraMan::privGetInstance();
	assert(pCameraMan);

	CameraNode *pNode = (CameraNode *)pCameraMan->active;
	while( pNode != 0 )
	{
		if( pNode->pCamera->getName() == _name )
		{
			// found it
			break;
		}

		pNode = (CameraNode *)pNode->next;
	}
	assert(pNode);
	return pNode->pCamera;
}

void CameraMan::SetCurrent( const Camera::Name name)
{
	CameraMan *pCameraMan = CameraMan::privGetInstance();
	assert(pCameraMan);

	Camera *pCam = CameraMan::Find(name);
	assert(pCam);
	
	pCameraMan->currCamera = pCam;
}

Camera * CameraMan::GetCurrent( )
{
	CameraMan *pCameraMan = CameraMan::privGetInstance();
	assert(pCameraMan);

	assert(pCameraMan->currCamera);
	return pCameraMan->currCamera;
}

void CameraMan::privAddToFront(CameraLink *node, CameraLink *&head)
{
    assert(node != 0);

    if (head == 0)
    {
        head = node;
        node->next = 0;
        node->prev = 0;
    }
    else
    {
        node->next = head;
        head->prev = node;
        head = node;
    }
}






