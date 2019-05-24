#ifndef BOUNDING_SPHERE_H
#define BOUNDING_SPHERE_H

#include "Vect.h"

struct Sphere
{
	Vect cntr;
	float rad;
};

void RitterSphere(Sphere &s, Vect *pt, int numPts);

#endif