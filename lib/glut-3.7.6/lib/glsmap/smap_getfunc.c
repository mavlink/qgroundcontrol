
/* Copyright (c) Mark J. Kilgard, 1998.  */

/* This program is freely distributable without licensing fees
   and is provided without guarantee or warrantee expressed or
   implied. This program is -not- in the public domain. */

#include "glsmapint.h"

void smapGetPositionLightsFunc(SphereMap *smap,
	void (**positionLights)(int view, void *context))
{
	*positionLights = smap->positionLights;
}

void smapGetDrawViewFunc(SphereMap *smap,
	void (**drawView)(int view, void *context))
{
	*drawView = smap->drawView;
}
