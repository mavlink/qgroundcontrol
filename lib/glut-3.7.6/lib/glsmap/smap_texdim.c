
/* Copyright (c) Mark J. Kilgard, 1998.  */

/* This program is freely distributable without licensing fees
   and is provided without guarantee or warrantee expressed or
   implied. This program is -not- in the public domain. */

#include "glsmapint.h"

void
smapSetSphereMapTexDim(SphereMap *smap, GLsizei texdim)
{
	smap->smapTexDim = texdim;
}

void
smapSetViewTexDim(SphereMap *smap, GLsizei texdim)
{
	smap->viewTexDim = texdim;
}

