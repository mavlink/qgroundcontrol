
/* Copyright (c) Mark J. Kilgard, 1998.  */

/* This program is freely distributable without licensing fees
   and is provided without guarantee or warrantee expressed or
   implied. This program is -not- in the public domain. */

#include <stdio.h>  /* SunOS multithreaded assert() needs <stdio.h>.  Lame. */
#include <assert.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glsmap.h>

#include "glsmapint.h"

static struct {
	int xl;
	int yl;
	int zl;
	int swap;  /* swap final (s,t) */
	int flip;  /* flip final s or t, ie. [0..1] --> [1..0] */
	float dir;
} faceInfo[5] = {
	{ 0, 1, 2, 0,  1.0,  1.0 },  /* front */
	{ 0, 2, 1, 0, -1.0,  1.0 },  /* top */
	{ 0, 2, 1, 0,  1.0, -1.0 },  /* bottom */
	{ 1, 2, 0, 1, -1.0,  1.0 },  /* left */
	{ 1, 2, 0, 1,  1.0, -1.0 },  /* right */
};

static struct {
	int xl;
	int yl;
	float dir;
} edgeInfo[4] = {
	{ 0, 1, -1.0 },
	{ 0, 1,  1.0 },
	{ 1, 0, -1.0 },
	{ 1, 0,  1.0 }
};

void
__smapValidateSphereMapMesh(SphereMapMesh *mesh)
{
	/* setup some local variables for variable array size indexing */
	INITFACE(mesh);
	INITBACK(mesh);

	float st[2];     /* (s,t) coordinate  */
				     /* range=[0..1,0..1] */
	float v[3];      /* (x,y,z) location on cube map */
	                 /* range=[-1..1,-1..1,-1..1] */
	float rv[3];     /* reflection vector, ie. cube map location */
	                 /* normalized onto unit sphere */
	float len;       /* distance from v[3] to origin */
	                 /* for converting to rv[3] */
	int side;        /* which of 5 faces (all but back face) */
	int i, j;
	int xl, yl, zl;  /* renamed X, Y, Z index */
	int swap;
	int flip;
	int edge;        /* which edge of back face */
	float sc, tc;    /* singularity (s,t) on back face circle */

	if (mesh->face) {
		assert(mesh->back == &(mesh->face[5*sqsteps]));
		return;
	}
	assert(mesh->back == NULL);

	mesh->face = (STXY*)
		malloc((5*sqsteps+4*ringedspokes) * sizeof(STXY));
	mesh->back = &(mesh->face[5*sqsteps]);

	/* for the front and four side faces */
	for (side=0; side<5; side++) {
		/* use faceInfo to parameterize face construction */
		xl  = faceInfo[side].xl;
		yl  = faceInfo[side].yl;
		zl  = faceInfo[side].zl;
		swap = faceInfo[side].swap;
		flip = faceInfo[side].flip;
		/* cube map "Z" coordinate */
		v[zl] = faceInfo[side].dir;

		for (i=0; i<mesh->steps; i++) {
			/* cube map "Y" coordinate */
			v[yl] = 2.0/(mesh->steps-1) * i - 1.0;
			for (j=0; j<mesh->steps; j++) {
				/* cube map "X" coordinate */
				v[xl] = 2.0/(mesh->steps-1) * j - 1.0;

				/* normalize cube map location to construct */
				/* reflection vector */
				len = sqrt(1.0 + v[xl]*v[xl] + v[yl]*v[yl]);
				rv[0] = v[0]/len;
				rv[1] = v[1]/len;
				rv[2] = v[2]/len;

				/* map reflection vector to sphere map (s,t) */
				/* NOTE: face[side][i][j] (x,y) gets updated */
				smapRvecToSt(rv, FACExy(side,i,j));

				/* update texture coordinate, */
				/* normalize [-1..1,-1..1] to [0..1,0..1] */
				if (!swap) {
				    FACE(side,i,j).s = (-v[xl] + 1.0)/2.0;
					FACE(side,i,j).t = (flip*v[yl] + 1.0)/2.0;
				} else {
					FACE(side,i,j).s = (flip*-v[yl] + 1.0)/2.0;
					FACE(side,i,j).t = (v[xl] + 1.0)/2.0;
				}
			}
		}
	}


	/* The back face must be specially handled.  The center
	   point in the back face of a cube map becomes a
	   a singularity around the circular edge of a sphere map. */

	/* Carefully work from each edge of the back face to center
	   of back face mapped to the outside of the sphere map. */

	/* cube map "Z" coordinate, always -1 since backface */
	v[2] = -1;

	/* for each edge */
	/*   [x=-1, y=-1..1, z=-1] */
	/*   [x= 1, y=-1..1, z=-1] */
	/*   [x=-1..1, y=-1, z=-1] */
	/*   [x=-1..1, y= 1, z=-1] */
	for (edge=0; edge<4; edge++) {
		/* cube map "X" coordinate */
		v[edgeInfo[edge].xl] = edgeInfo[edge].dir;
		for (j=0; j<mesh->steps; j++) {			
			/* cube map "Y" coordinate */
			v[edgeInfo[edge].yl] = 2.0/(mesh->steps-1) * j - 1.0;

			/* normalize cube map location to construct */
			/* reflection vector */
			len = sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
			rv[0] = v[0]/len;
			rv[1] = v[1]/len;
			rv[2] = v[2]/len;

			/* Map reflection vector to sphere map (s,t). */
			smapRvecToSt(rv, st);

			/* Determine distinance from the center of sphere */
			/* map (0.5,0.5) to (s,t) */
			len = sqrt((st[0]-0.5)*(st[0]-0.5) + (st[1]-0.5)*(st[1]-0.5));

			/* Calculate (s,t) location extended to the singularity */
			/* at the center of the back face (ie, extend to */
			/* circle edge of the sphere map). */
			sc = (st[0]-0.5)/len * 0.5 + 0.5;
			tc = (st[1]-0.5)/len * 0.5 + 0.5;

			/* (s,t) at back face edge. */
			BACK(edge,0,j).s = (-v[0] + 1.0)/2.0;
			BACK(edge,0,j).t = (-v[1] + 1.0)/2.0;
			BACK(edge,0,j).x = st[0];
			BACK(edge,0,j).y = st[1];

			/* If just two rings, we just generate a back face edge
			   vertex and a center vertex (2 rings), but if there
			   are more rings, we carefully interpolate between the
			   edge and center vertices.  Notice how smapStToRvec is used
			   to map the interpolated (s,t) into a reflection vector
			   that must then be extended to the back cube face (it is
			   not correct to just interpolate the texture
			   coordinates!). */
			if (mesh->rings > 2) {
				float ist[2];  /* interpolated (s,t) */
				float ds, dt;  /* delta s and delta t */

				/* Start interpolating from the edge. */
				ist[0] = st[0];
				ist[1] = st[1];

				/* Calculate delta s and delta t for interpolation. */
				ds = (sc - ist[0]) / (mesh->rings-1);
				dt = (tc - ist[1]) / (mesh->rings-1);

				for (i=1; i<mesh->rings-1; i++) {
					/* Incremental interpolation of (s,t). */
					ist[0] = ist[0] + ds;
					ist[1] = ist[1] + dt;

					/* Calculate reflection vector from interpolated (s,t). */
					smapStToRvec(ist, rv);
					/* Assert that z must be on the back cube face. */
					assert(rv[2] <= -sqrt(1.0/3.0));
					/* Extend reflection vector out of back cube face. */
					/* Note: z is negative value so negate z to avoid */
					/* inverting x and y! */
					rv[0] = rv[0] / -rv[2];
					rv[1] = rv[1] / -rv[2];

					BACK(edge,i,j).s = (-rv[0] + 1.0)/2.0;
					BACK(edge,i,j).t = (-rv[1] + 1.0)/2.0;
					BACK(edge,i,j).x = ist[0];
					BACK(edge,i,j).y = ist[1];
				}
			}

			/* (s,t) at circle edge of the sphere map is ALWAYS */
			/* at center of back cube map face */
			BACK(edge,mesh->rings-1,j).s = 0.5;
			BACK(edge,mesh->rings-1,j).t = 0.5;
			/* Location of singularity at the edge of the sphere map. */
			BACK(edge,mesh->rings-1,j).x = sc;
			BACK(edge,mesh->rings-1,j).y = tc;

			if (mesh->edgeExtend) {
				/* Add an extra ring to avoid seeing the */
				/* tessellation boundary of the sphere map's sphere. */
				BACK(edge,mesh->rings,j).s = 0.5;
				BACK(edge,mesh->rings,j).t = 0.5;
				/* 0.33 below is a fudge factor. */
				BACK(edge,mesh->rings,j).x = sc + 0.33*(sc - st[0]);
				BACK(edge,mesh->rings,j).y = tc + 0.33*(tc - st[1]);
			}
		}
	}
	for (edge=0; edge<4; edge++) {
		for (j=0; j<mesh->steps; j++) {			
			for (i=1; i<mesh->rings-1; i++) {
			}
		}
	}
}

void
smapConfigureSphereMapMesh(SphereMap *smap,
	int steps, int rings, int edgeExtend)
{
	SphereMapMesh *mesh = smap->mesh;

	if (steps == mesh->steps &&
		rings == mesh->rings &&
		edgeExtend == mesh->edgeExtend) {
		return;
	}

	mesh->steps = steps;
	mesh->rings = rings;
	mesh->edgeExtend = edgeExtend;

	mesh->face = NULL;
	mesh->back = NULL;
}
