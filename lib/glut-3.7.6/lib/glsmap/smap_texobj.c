
/* Copyright (c) Mark J. Kilgard, 1998.  */

/* This program is freely distributable without licensing fees
   and is provided without guarantee or warrantee expressed or
   implied. This program is -not- in the public domain. */

#include "glsmapint.h"

void
smapSetSphereMapTexObj(SphereMap *smap, GLuint texobj)
{
	smap->smapTexObj = texobj;
}

void
smapSetViewTexObj(SphereMap *smap, GLuint texobj)
{
	smap->viewTexObj = texobj;
}

void
smapSetViewTexObjs(SphereMap *smap, GLuint texobjs[6])
{
	int i;

	for (i=0; i<6; i++) {
		smap->viewTexObjs[i] = texobjs[i];
	}
}

