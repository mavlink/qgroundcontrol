
/* Copyright (c) Mark J. Kilgard, 1998.  */

/* This program is freely distributable without licensing fees
   and is provided without guarantee or warrantee expressed or
   implied. This program is -not- in the public domain. */

#include "glsmapint.h"

void
smapSetNearFar(SphereMap *smap,
			   GLfloat viewNear, GLfloat viewFar)
{
	/* Curse Intel for "near" and "far" keywords. */
	smap->viewNear = viewNear;
	smap->viewFar = viewFar;
}

void
smapGetNearFar(SphereMap *smap,
			   GLfloat *viewNear, GLfloat *viewFar)
{
	/* Curse Intel for "near" and "far" keywords. */
        *viewNear = smap->viewNear;
	*viewFar = smap->viewFar;
}
