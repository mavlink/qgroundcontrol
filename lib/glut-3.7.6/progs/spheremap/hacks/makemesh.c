
/* Copyright (c) Mark J. Kilgard, 1998.  */

/* This program is freely distributable without licensing fees
   and is provided without guarantee or warrantee expressed or
   implied. This program is -not- in the public domain. */

/* makemesh.c - generates mesh geometry for sphere map construction */

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <GL/glut.h>
#include <math.h>

#include "smapmesh.h"

/* (x,y,z) reflection vector --> (s,t) sphere map coordinates */
void
rvec2st(float v[3], float st[2])
{
	double m;

	/* In Section 2.10.4 ("Generating texture coordinates")
	   of the OpenGL 1.1 specification, you will find the
	   GL_SPHERE_MAP equations:

          n' = normal after transformation to eye coordinates
		  u  = unit vector from origin to vertex in eye coordinates

		  (rx, ry, rz) = u - 2 * n' * transpose(n') * u

		  m  = 2 * sqrt(rx^2 + ry^2 + (rz + 1)^2))

		  s = rx/m + 0.5
		  t = ry/m + 0.5

        The equation for calculating (rx, ry, rz) is the
		equation for calculating the reflection vector for
		a surface and observer.  The explanation and
		derivation for this equation is found in Roger's
		"Procedural Elements for Computer Graphics" 2nd ed.
		in Section 5-5 ("Determining the Reflection Vector").
		Note that Roger's convention has the Z axis in
		the opposite direction from the OpenGL convention. */

	m = 2 * sqrt(v[0]*v[0] + v[1]*v[1] + (v[2]+1)*(v[2]+1));

	st[0] = v[0]/m + 0.5;
	st[1] = v[1]/m + 0.5;
}

/* (s,t) sphere map coordinate --> reflection verctor (x,y,z) */
void
st2rvec(float s, float t, float *xp, float *yp, float *zp)
{
	double rx, ry, rz;
	double tmp1, tmp2;

	/* Using algebra to invert the sphere mapping equations
	   shown above in rvec2st, you get:

         rx = 2*sqrt(-4*s^2 + 4*s - 4*t^2 + 4*t - 1)*(2*s-1)
		 ry = 2*sqrt(-4*s^2 + 4*s - 4*t^2 + 4*t - 1)*(2*t-1)
		 rz = -8*s^2 + 8*s - 8*t^2 + 8*t - 3

       The C code below eliminates common subexpressions. */

	tmp1 = s*(1-s) + t*(1-t);
	tmp2 = 2 * sqrt(4*tmp1 - 1);

	rx = tmp2 * (2*s-1);
	ry = tmp2 * (2*t-1);
	rz = 8 * tmp1 - 3;

	*xp = (float) rx;
	*yp = (float) ry;
	*zp = (float) rz;
}

/* For best results (ie, to avoid cracks in the
   sphere map construction, XSTEPS, YSTEPS, and
   SPOKES should all be equal. */

/* Increasing the nSTEPS and RINGS constants below
   will give you a better approximation to the
   sphere map image warp at the cost of more polygons
   to render the image warp.  My bet is that no
   will be able to the improved quality of a higher
   level of tessellation. */

STXY face[5][YSTEPS][XSTEPS];
STXY back[4][RINGS+EDGE_EXTEND][SPOKES];

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
makeSphereMapMesh(void)
{
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

		for (i=0; i<YSTEPS; i++) {
			/* cube map "Y" coordinate */
			v[yl] = 2.0/(YSTEPS-1) * i - 1.0;
			for (j=0; j<XSTEPS; j++) {
				/* cube map "X" coordinate */
				v[xl] = 2.0/(XSTEPS-1) * j - 1.0;

				/* normalize cube map location to construct */
				/* reflection vector */
				len = sqrt(1.0 + v[xl]*v[xl] + v[yl]*v[yl]);
				rv[0] = v[0]/len;
				rv[1] = v[1]/len;
				rv[2] = v[2]/len;

				/* map reflection vector to sphere map (s,t) */
				/* NOTE: face[side][i][j] (x,y) gets updated */
				rvec2st(rv, &face[side][i][j].x);

				/* update texture coordinate, */
				/* normalize [-1..1,-1..1] to [0..1,0..1] */
				if (!swap) {
					face[side][i][j].s = (-v[xl] + 1.0)/2.0;
					face[side][i][j].t = (flip*v[yl] + 1.0)/2.0;
				} else {
					face[side][i][j].s = (flip*-v[yl] + 1.0)/2.0;
					face[side][i][j].t = (v[xl] + 1.0)/2.0;
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
		for (j=0; j<SPOKES; j++) {			
			/* cube map "Y" coordinate */
			v[edgeInfo[edge].yl] = 2.0/(SPOKES-1) * j - 1.0;

			/* normalize cube map location to construct */
			/* reflection vector */
			len = sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
			rv[0] = v[0]/len;
			rv[1] = v[1]/len;
			rv[2] = v[2]/len;

			/* Map reflection vector to sphere map (s,t). */
			rvec2st(rv, st);

			/* determine distinance from the center of sphere */
			/* map (0.5,0.5) to (s,t) */
			len = sqrt((st[0]-0.5)*(st[0]-0.5) + (st[1]-0.5)*(st[1]-0.5));

			/* calculate (s,t) location extended to the singularity */
			/* at the center of the back face (ie, extend to */
			/* circle edge of the sphere map) */
			sc = (st[0]-0.5)/len * 0.5 + 0.5;
			tc = (st[1]-0.5)/len * 0.5 + 0.5;

			/* (s,t) at back face edge */
			back[edge][0][j].s = (-v[0] + 1.0)/2.0;
			back[edge][0][j].t = (-v[1] + 1.0)/2.0;
			back[edge][0][j].x = st[0];
			back[edge][0][j].y = st[1];

			/* If just two rings, we just generate a back face edge
			   vertex and a center vertex (2 rings), but if there
			   are more rings, we carefully interpolate between the
			   edge and center vertices.  Notice how st2rvec is used
			   to map the interpolated (s,t) into a reflection vector
			   that must then be extended to the back cube face (it is
			   not correct to just interpolate the texture
			   coordinates!). */
			if (RINGS > 2) {
				float s, t;    /* interpolated (s,t) */
				float ds, dt;  /* delta s and delta t */
				float x, y, z;

				/* Start interpolating from the edge. */
				s = st[0];
				t = st[1];

				/* Calculate delta s and delta t for interpolation. */
				ds = (sc - s) / (RINGS-1);
				dt = (tc - t) / (RINGS-1);

				for (i=1; i<RINGS-1; i++) {
					/* Incremental interpolation of (s,t). */
					s = s + ds;
					t = t + dt;

					/* Calculate reflection vector from interpolated (s,t). */
					st2rvec(s, t, &x, &y, &z);
					/* Assert that z must be on the back cube face. */
					assert(z <= -sqrt(1.0/3.0));
					/* Extend reflection vector out of back cube face. */
					/* Note: z is negative value so negate z to avoid */
					/* inverting x and y! */
					x = x / -z;
					y = y / -z;

					back[edge][i][j].s = (-x + 1.0)/2.0;
					back[edge][i][j].t = (-y + 1.0)/2.0;
					back[edge][i][j].x = s;
					back[edge][i][j].y = t;
				}
			}

			/* (s,t) at circle edge of the sphere map is ALWAYS */
			/* at center of back cube map face */
			back[edge][RINGS-1][j].s = 0.5;
			back[edge][RINGS-1][j].t = 0.5;
			/* location of singularity at the edge of the sphere map */
			back[edge][RINGS-1][j].x = sc;
			back[edge][RINGS-1][j].y = tc;

#ifdef SPHERE_MAP_EDGE_EXTEND
			/* Add an extra ring to avoid seeing the */
			/* tessellation boundary of the sphere map's sphere */
			back[edge][RINGS][j].s = 0.5;
			back[edge][RINGS][j].t = 0.5;
			/* 0.33 below is a fudge factor. */
			back[edge][RINGS][j].x = sc + 0.33*(sc - st[0]);
			back[edge][RINGS][j].y = tc + 0.33*(tc - st[1]);
#endif
		}
	}
}
