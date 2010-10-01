
/* Copyright (c) Mark J. Kilgard, 1998.  */

/* This program is freely distributable without licensing fees
   and is provided without guarantee or warrantee expressed or
   implied. This program is -not- in the public domain. */

#include "glsmapint.h"

void
smapSetEyeVector(SphereMap *smap, GLfloat *eye)
{
	smap->eye[X] = eye[X];
	smap->eye[Y] = eye[Y];
	smap->eye[Z] = eye[Z];
}

void
smapSetUpVector(SphereMap *smap, GLfloat *up)
{
	smap->up[X] = up[X];
	smap->up[Y] = up[Y];
	smap->up[Z] = up[Z];
}

void
smapSetObjectVector(SphereMap *smap, GLfloat *obj)
{
	smap->obj[X] = obj[X];
	smap->obj[Y] = obj[Y];
	smap->obj[Z] = obj[Z];
}

