
/* Copyright (c) Mark J. Kilgard, 1998.  */

/* This program is freely distributable without licensing fees
   and is provided without guarantee or warrantee expressed or
   implied. This program is -not- in the public domain. */

#include "glsmapint.h"

void
smapGetSphereMapTexObj(SphereMap *smap, GLuint *texobj)
{
	*texobj = smap->smapTexObj;
}

void
smapGetViewTexObj(SphereMap *smap, GLuint *texobj)
{
	*texobj = smap->viewTexObj;
}

void
smapGetViewTexObjs(SphereMap *smap, GLuint texobjs[6])
{
	int i;

	for (i=0; i<6; i++) {
		texobjs[i] = smap->viewTexObjs[i];
	}
}

