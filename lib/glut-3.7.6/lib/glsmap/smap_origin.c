
/* Copyright (c) Mark J. Kilgard, 1998.  */

/* This program is freely distributable without licensing fees
   and is provided without guarantee or warrantee expressed or
   implied. This program is -not- in the public domain. */

#include <GL/glsmap.h>

#include "glsmapint.h"

void smapSetViewOrigin(SphereMap *smap, GLint x, GLint y)
{
        smap->viewOrigin[0] = x;
        smap->viewOrigin[1] = y;
}

void smapSetSphereMapOrigin(SphereMap *smap, GLint x, GLint y)
{
        smap->smapOrigin[0] = x;
        smap->smapOrigin[1] = y;
}

void smapGetViewOrigin(SphereMap *smap, GLint *x, GLint *y)
{
        *x = smap->viewOrigin[0];
        *y = smap->viewOrigin[1];
}

void smapGetSphereMapOrigin(SphereMap *smap, GLint *x, GLint *y)
{
        *x = smap->smapOrigin[0];
        *y = smap->smapOrigin[1];
}
