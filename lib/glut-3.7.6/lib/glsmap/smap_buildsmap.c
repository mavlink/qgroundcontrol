
/* Copyright (c) Mark J. Kilgard, 1998.  */

/* This program is freely distributable without licensing fees
   and is provided without guarantee or warrantee expressed or
   implied. This program is -not- in the public domain. */

/* smap_buildsmap.c - automatically builds sphere map */

#include <assert.h>
#include <stdio.h>
#include <GL/glsmap.h>
#include <GL/glu.h>

#include "glsmapint.h"

#if defined(GL_EXT_texture_object) && !defined(GL_VERSION_1_1)
#define glBindTexture(A,B)     glBindTextureEXT(A,B)
#endif
#if defined(GL_EXT_copy_texture) && !defined(GL_VERSION_1_1)
#define glCopyTexImage2D(A, B, C, D, E, F, G, H)    glCopyTexImage2DEXT(A, B, C, D, E, F, G, H)
#endif

static void
copyImageToTexture(SphereMap *smap,
                   GLuint texobj, int origin[2], int texdim)
{
        int isSmapTexObj = (texobj == smap->smapTexObj) ? 1 : 0;
        int genMipmapsFlag =
                isSmapTexObj ? SMAP_GENERATE_SMAP_MIPMAPS : SMAP_GENERATE_VIEW_MIPMAPS;
        static GLubyte pixels[256][256][3];  /* XXX fix me. */

	glBindTexture(GL_TEXTURE_2D, texobj);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
		GL_LINEAR);
        if (smap->flags & genMipmapsFlag) {
        	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                        GL_LINEAR_MIPMAP_LINEAR);
        } else {
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
	  	        GL_LINEAR);
        }

        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	/* Clamp to avoid artifacts from wrap around in texture. */
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

        if (smap->flags & genMipmapsFlag) {
	        glPixelStorei(GL_PACK_ALIGNMENT, 1);
                glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
                glReadPixels(origin[X], origin[Y], texdim, texdim,
                        GL_RGB, GL_UNSIGNED_BYTE, pixels);
                gluBuild2DMipmaps(GL_TEXTURE_2D, 3, texdim, texdim,
                        GL_RGB, GL_UNSIGNED_BYTE, pixels);       
        } else {
                glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,
		        origin[X], origin[Y], texdim, texdim, 0);
        }
}

static struct {
	GLfloat angle;
	GLfloat x, y, z;
} faceInfo[6] = {
	{   0.0, +1.0,  0.0,  0.0 },  /* front */
	{  90.0, -1.0,  0.0,  0.0 },  /* top */
	{  90.0, +1.0,  0.0,  0.0 },  /* bottom */
	{  90.0,  0.0, -1.0,  0.0 },  /* left */
	{  90.0,  0.0, +1.0,  0.0 },  /* right */
	{ 180.0, -1.0,  0.0,  0.0 }   /* back */
};

static void
configFace(SphereMap *smap, int view)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glRotatef(faceInfo[view].angle,
		faceInfo[view].x, faceInfo[view].y, faceInfo[view].z);
	gluLookAt(smap->obj[X], smap->obj[Y], smap->obj[Z],  /* "eye" at object */
		      smap->eye[X], smap->eye[Y], smap->eye[Z],  /* looking at eye */
			  smap->up[X], smap->up[Y], smap->up[Z]);
}

static void
initGenViewTex(SphereMap *smap)
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(90.0, 1.0, smap->viewNear, smap->viewFar);
    glViewport(smap->viewOrigin[X], smap->viewOrigin[Y],
		smap->viewTexDim, smap->viewTexDim);
	glScissor(0, 0, smap->viewTexDim, smap->viewTexDim);
	glEnable(GL_SCISSOR_TEST);
	glEnable(GL_DEPTH_TEST);
}

static void
genViewTex(SphereMap *smap, int view)
{
	configFace(smap, view);
        assert(smap->positionLights);
	smap->positionLights(view, smap->context);
        assert(smap->drawView);
	smap->drawView(view, smap->context);
}

void
smapGenViewTex(SphereMap *smap, int view)
{
	initGenViewTex(smap);
	genViewTex(smap, view);
	copyImageToTexture(smap, smap->viewTexObjs[view],
		smap->viewOrigin, smap->viewTexDim);
}

void
smapGenViewTexs(SphereMap *smap)
{
	int view;

	initGenViewTex(smap);

	for (view=0; view<6; view++) {
		genViewTex(smap, view);
		copyImageToTexture(smap, smap->viewTexObjs[view],
			smap->viewOrigin, smap->viewTexDim);
	}
}

void
drawSphereMapMesh(SphereMap *smap)
{
	int side;

	/* Calculate sphere map mesh if needed. */
	__smapValidateSphereMapMesh(smap->mesh);

	/* Five front and side faces. */
	for (side=0; side<5; side++) {
		/* Bind to texture for given face of cube map. */
		glBindTexture(GL_TEXTURE_2D, smap->viewTexObjs[side]);
		__smapDrawSphereMapMeshSide(smap->mesh, side);
	}

	/* Bind to texture for back face of cube map. */
	glBindTexture(GL_TEXTURE_2D, smap->viewTexObjs[side]);
	__smapDrawSphereMapMeshBack(smap->mesh);
}

void
initDrawSphereMapMesh(SphereMap *smap)
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0, 1, 0, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
        glViewport(smap->smapOrigin[X], smap->smapOrigin[Y],
		smap->smapTexDim, smap->smapTexDim);

        if (smap->flags & SMAP_CLEAR_SMAP_TEXTURE) {
	        glClear(GL_COLOR_BUFFER_BIT);
        }

	glDisable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_CULL_FACE);
}

void
smapGenSphereMapFromViewTexs(SphereMap *smap)
{
	initDrawSphereMapMesh(smap);
	drawSphereMapMesh(smap);

	copyImageToTexture(smap, smap->smapTexObj,
		smap->smapOrigin, smap->smapTexDim);
}

void
smapGenSphereMap(SphereMap *smap)
{
	smapGenViewTexs(smap);
	smapGenSphereMapFromViewTexs(smap);
}

void
smapGenSphereMapWithOneViewTex(SphereMap *smap)
{
	int side;

	/* Make sure viewports are not obviously overlapping. */
	assert(smap->viewOrigin[X] != smap->smapOrigin[X]);
	assert(smap->viewOrigin[Y] != smap->smapOrigin[Y]);

	/* Calculate sphere map mesh if needed. */
	__smapValidateSphereMapMesh(smap->mesh);

	/* Five front and side faces. */
	for (side=0; side<5; side++) {
		initGenViewTex(smap);
		genViewTex(smap, side);
		copyImageToTexture(smap, smap->viewTexObj,
			smap->viewOrigin, smap->viewTexDim);

		/* Preceeding copyImageToTexture does bind to smap->viewTexObj */

		initDrawSphereMapMesh(smap);
		__smapDrawSphereMapMeshSide(smap->mesh, side);
	}

	initGenViewTex(smap);
	genViewTex(smap, SMAP_BACK);
	copyImageToTexture(smap, smap->viewTexObj,
			smap->viewOrigin, smap->viewTexDim);

	/* Preceeding copyImageToTexture does bind to smap->viewTexObj */

	initDrawSphereMapMesh(smap);
	__smapDrawSphereMapMeshBack(smap->mesh);

	copyImageToTexture(smap, smap->smapTexObj,
		smap->smapOrigin, smap->smapTexDim);
}
