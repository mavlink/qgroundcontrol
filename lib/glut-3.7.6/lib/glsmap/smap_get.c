
/* Copyright (c) Mark J. Kilgard, 1998.  */

/* This program is freely distributable without licensing fees
   and is provided without guarantee or warrantee expressed or
   implied. This program is -not- in the public domain. */

#include "glsmapint.h"

void smapGetEye(SphereMap *smap,
	GLfloat *eyex, GLfloat *eyey, GLfloat *eyez)
{
	*eyex = smap->eye[X];
	*eyey = smap->eye[Y];
	*eyez = smap->eye[Z];
}

void smapGetUp(SphereMap *smap,
	GLfloat *upx, GLfloat *upy, GLfloat *upz)
{
	*upx = smap->up[X];
	*upy = smap->up[Y];
	*upz = smap->up[Z];
}

void smapGetObject(SphereMap *smap,
	GLfloat *objx, GLfloat *objy, GLfloat *objz)
{
	*objx = smap->obj[X];
	*objy = smap->obj[Y];
	*objz = smap->obj[Z];
}
