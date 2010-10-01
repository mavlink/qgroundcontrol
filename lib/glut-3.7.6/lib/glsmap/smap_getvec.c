
/* Copyright (c) Mark J. Kilgard, 1998.  */

/* This program is freely distributable without licensing fees
   and is provided without guarantee or warrantee expressed or
   implied. This program is -not- in the public domain. */

#include "glsmapint.h"

void
smapGetEyeVector(SphereMap *smap, GLfloat *eye)
{
	eye[X] = smap->eye[X];
	eye[Y] = smap->eye[Y];
	eye[Z] = smap->eye[Z];
}

void
smapGetUpVector(SphereMap *smap, GLfloat *up)
{
	up[X] = smap->up[X];
	up[Y] = smap->up[Y];
	up[Z] = smap->up[Z];
}

void
smapGetObjectVector(SphereMap *smap, GLfloat *obj)
{
	obj[X] = smap->obj[X];
	obj[Y] = smap->obj[Y];
	obj[Z] = smap->obj[Z];
}

