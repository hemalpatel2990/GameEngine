#ifndef CAMERA_MANAGER_H
#define CAMERA_MANAGER_H

#include "Camera.h"
#include "CameraNode.h"



// Singleton
class CameraMan
{
public:
	static void Add(Camera::Name name, Camera *pCamera);
	static Camera * Find( Camera::Name name);
	static void SetCurrent( Camera::Name name);
	static Camera * GetCurrent( );

private:  // methods

	static CameraMan *privGetInstance();
	CameraMan();
	void privAddToFront(CameraLink *node, CameraLink *&head);


private:  // add

	CameraLink *active;
	Camera *currCamera;

};

#endif