
/* Copyright (c) Mark J. Kilgard, 1998.  */

/* This program is freely distributable without licensing fees
   and is provided without guarantee or warrantee expressed or
   implied. This program is -not- in the public domain. */

/* drawmesh.c - draws mesh for constructing a sphere map */

#include <GL/glut.h>

#include "smapmesh.h"

#if defined(GL_EXT_texture_object) && !defined(GL_VERSION_1_1)
#define glBindTexture(A,B)     glBindTextureEXT(A,B)
#endif

void
drawSphereMapMesh(GLuint texobj[6])
{
	int side, i, j;

	/* five front and side faces */
	for (side=0; side<5; side++) {
		/* bind to texture for given face of cube map */
		glBindTexture(GL_TEXTURE_2D, texobj[side]);
		for (i=0; i<YSTEPS-1; i++) {
			glBegin(GL_QUAD_STRIP);
			for (j=0; j<XSTEPS; j++) {
				glTexCoord2fv (&face[side][i][j].s);
				glVertex2fv   (&face[side][i][j].x);
				glTexCoord2fv (&face[side][i+1][j].s);
				glVertex2fv   (&face[side][i+1][j].x);
			}
			glEnd();
		}
	}

	/* Back face specially rendered for its singularity! */

	/* Bind to texture for back face of cube map. */
	glBindTexture(GL_TEXTURE_2D, texobj[side]);
	for (side=0; side<4; side++) {
		for (j=0; j<RINGS-1+EDGE_EXTEND; j++) {
			glBegin(GL_QUAD_STRIP);
			for (i=0; i<SPOKES; i++) {
				glTexCoord2fv (&back[side][j][i].s);
				glVertex2fv   (&back[side][j][i].x);
				glTexCoord2fv (&back[side][j+1][i].s);
				glVertex2fv   (&back[side][j+1][i].x);
			}
			glEnd();
		}
	}
}
