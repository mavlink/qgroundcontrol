
/* Copyright (c) Mark J. Kilgard, 1998.  */

/* This program is freely distributable without licensing fees
   and is provided without guarantee or warrantee expressed or
   implied. This program is -not- in the public domain. */

#include "glsmapint.h"

void
smapGetSphereMapTexDim(SphereMap *smap, GLsizei *texdim)
{
	*texdim = smap->smapTexDim;
}

void
smapGetViewTexDim(SphereMap *smap, GLsizei *texdim)
{
	*texdim = smap->viewTexDim;
}

