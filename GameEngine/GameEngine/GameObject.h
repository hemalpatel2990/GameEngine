#ifndef GAME_OBJECT_H
#define GAME_OBJECT_H

#include "MathEngine.h"
#include "ShaderObject.h"
#include "Model.h"
#include "GraphicsObject.h"
#include "PCSNode.h"
#include "Time.h"

class GameObject : public PCSNode
{
public:
	GameObject(GraphicsObject  *graphicsObject);

	// Big four
	GameObject() = delete;
	GameObject( const GameObject &) = delete;
	GameObject &operator=(GameObject &) = delete;
	~GameObject();

	void Update(Time currentTime);
    void Process(Time currentTime);

	void Draw();

	GraphicsObject *getGraphicsObject();

	void setBoneOrientation(const Matrix &);
	Matrix getBoneOrientation(void) const;

	void setIndex(int val);

	Matrix *getWorld();
	void setWorld(Matrix *world);

public:
	Vect *pScale;
	Vect *pPos;
	Vect *pDof;
	Vect *pUp;

	// Animation stuff: 	
		Matrix      *pLocal;
		Matrix		*pBoneOrientation;
		int         indexBoneArray;

protected:
	Matrix *pWorld;

private:
	GraphicsObject  *pGraphicsObject;


};


#endif