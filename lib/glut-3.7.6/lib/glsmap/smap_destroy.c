
/* Copyright (c) Mark J. Kilgard, 1998.  */

/* This program is freely distributable without licensing fees
   and is provided without guarantee or warrantee expressed or
   implied. This program is -not- in the public domain. */

#include <stdio.h>  /* SunOS multithreaded assert() needs <stdio.h>.  Lame. */
#include <assert.h>
#include <stdlib.h>

#include "glsmapint.h"

static void
derefSphereMapMesh(SphereMapMesh *mesh)
{
	assert(mesh->refcnt > 0);
	mesh->refcnt--;
	if (mesh->refcnt == 0) {
		if (mesh->face) {
			assert(mesh->back ==
				&(mesh->face[5*mesh->steps*mesh->steps]));
			free(mesh->face);
		}
		free(mesh);
	}
}

void
smapDestroySphereMap(SphereMap *smap)
{
	derefSphereMapMesh(smap->mesh);
	free(smap);
}
