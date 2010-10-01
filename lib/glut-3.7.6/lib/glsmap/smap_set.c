
/* Copyright (c) Mark J. Kilgard, 1998.  */

/* This program is freely distributable without licensing fees
   and is provided without guarantee or warrantee expressed or
   implied. This program is -not- in the public domain. */

#include "glsmapint.h"

void smapSetEye(SphereMap *smap,
	GLfloat eyex, GLfloat eyey,	GLfloat eyez)
{
	smap->eye[X] = eyex;
	smap->eye[Y] = eyey;
	smap->eye[Z] = eyez;
}

void smapSetUp(SphereMap *smap,
	GLfloat upx, GLfloat upy, GLfloat upz)
{
	smap->up[X] = upx;
	smap->up[Y] = upy;
	smap->up[Z] = upz;
}

void smapSetObject(SphereMap *smap,
	GLfloat objx, GLfloat objy, GLfloat objz)
{
	smap->obj[X] = objx;
	smap->obj[Y] = objy;
	smap->obj[Z] = objz;
}
