#include "CameraDemoInput.h"
#include "InputMan.h"
#include "Keyboard.h"
#include "CameraMan.h"


void CameraDemoInput()
{
	Keyboard *key = InputMan::GetKeyboard();

	if (key->GetKeyState(AZUL_KEY::KEY_0))
	{
		CameraMan::SetCurrent(Camera::Name::CAMERA_0);
	}

	if (key->GetKeyState(AZUL_KEY::KEY_1))
	{
		CameraMan::SetCurrent(Camera::Name::CAMERA_1);
	}

	if (key->GetKeyState(AZUL_KEY::KEY_ARROW_UP))
	{
		Camera *pCam = CameraMan::Find(Camera::Name::CAMERA_0);
		
		Vect pos;
		Vect tar;
		Vect up;
		Vect upNorm;
		Vect forwardNorm;
		Vect rightNorm;

		pCam->getHelper(up,tar, pos, upNorm, forwardNorm, rightNorm);

		// OK, now 3 points... pos, tar, up
		//     now 3 normals... upNorm, forwardNorm, rightNorm

		// Let's Roll

		float delta = 0.01f;
		up += delta * upNorm;
		pos += delta * upNorm;
		tar += delta * upNorm;

		pCam->setHelper(up, tar, pos);

	}

	if (key->GetKeyState(AZUL_KEY::KEY_ARROW_DOWN))
	{
		Camera *pCam = CameraMan::Find(Camera::Name::CAMERA_0);

		Vect pos;
		Vect tar;
		Vect up;
		Vect upNorm;
		Vect forwardNorm;
		Vect rightNorm;

		pCam->getHelper(up, tar, pos, upNorm, forwardNorm, rightNorm);

		// OK, now 3 points... pos, tar, up
		//     now 3 normals... upNorm, forwardNorm, rightNorm

		// Let's Roll

		float delta = -0.01f;
		up += delta * upNorm;
		pos += delta * upNorm;
		tar += delta * upNorm;

		pCam->setHelper(up, tar, pos);

	}

	if (key->GetKeyState(AZUL_KEY::KEY_ARROW_RIGHT))
	{
		Camera *pCam = CameraMan::Find(Camera::Name::CAMERA_0);

		Vect pos;
		Vect tar;
		Vect up;
		Vect upNorm;
		Vect forwardNorm;
		Vect rightNorm;

		pCam->getHelper(up, tar, pos, upNorm, forwardNorm, rightNorm);

		// OK, now 3 points... pos, tar, up
		//     now 3 normals... upNorm, forwardNorm, rightNorm

		// Let's Roll

		float delta = 0.01f;
		up += delta * rightNorm;
		pos += delta * rightNorm;
		tar += delta * rightNorm;

		pCam->setHelper(up, tar, pos);

	}

	if (key->GetKeyState(AZUL_KEY::KEY_ARROW_LEFT))
	{
		Camera *pCam = CameraMan::Find(Camera::Name::CAMERA_0);

		Vect pos;
		Vect tar;
		Vect up;
		Vect upNorm;
		Vect forwardNorm;
		Vect rightNorm;

		pCam->getHelper(up, tar, pos, upNorm, forwardNorm, rightNorm);

		// OK, now 3 points... pos, tar, up
		//     now 3 normals... upNorm, forwardNorm, rightNorm

		// Let's Roll

		float delta = -0.01f;
		up += delta * rightNorm;
		pos += delta * rightNorm;
		tar += delta * rightNorm;

		pCam->setHelper(up, tar, pos);

	}

}