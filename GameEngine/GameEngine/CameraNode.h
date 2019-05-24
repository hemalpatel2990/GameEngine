#ifndef CAMERA_NODE_H
#define CAMERA_NODE_H

class Camera;



class CameraLink
{
public:
	CameraLink()
	{
		this->next = 0;
		this->prev = 0;
	}

	CameraLink *next;
	CameraLink *prev;
};

class CameraNode: public CameraLink
{
public:
	CameraNode();
	void set( Camera *pCamera);

public:
	Camera *pCamera;

};



#endif