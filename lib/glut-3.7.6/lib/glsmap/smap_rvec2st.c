
/* Copyright (c) Mark J. Kilgard, 1998.  */

/* This program is freely distributable without licensing fees
   and is provided without guarantee or warrantee expressed or
   implied. This program is -not- in the public domain. */

#include <math.h>

#include "glsmapint.h"

/* (x,y,z) reflection vector --> (s,t) sphere map coordinates */
int
smapRvecToSt(float rvec[3], float st[2])
{
	double m, recipm;

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

	m = 2 * sqrt(rvec[0]*rvec[0] +
		         rvec[1]*rvec[1] +
				 (rvec[2]+1)*(rvec[2]+1));
	if (m == 0.0) {
	    /* Some point on the sphere map perimeter. */
	    st[0] = 0.0;
	    st[1] = 0.5;
		return 0;
	}

	recipm = 1.0/m;

	st[0] = rvec[0]*recipm + 0.5;
	st[1] = rvec[1]*recipm + 0.5;
	return 1;
}

/* (s,t) sphere map coordinate --> reflection verctor (x,y,z) */
void
smapStToRvec(float *st, float *rvec)
{
	double tmp1, tmp2;

	/* Using algebra to invert the sphere mapping equations
	   shown above in smapRvecToSt, you get:

         rx = 2*sqrt(-4*s^2 + 4*s - 4*t^2 + 4*t - 1)*(2*s-1)
		 ry = 2*sqrt(-4*s^2 + 4*s - 4*t^2 + 4*t - 1)*(2*t-1)
		 rz = -8*s^2 + 8*s - 8*t^2 + 8*t - 3

       The C code below eliminates common subexpressions. */

	tmp1 = st[0]*(1-st[0]) + st[1]*(1-st[1]);
	tmp2 = 2 * sqrt(4*tmp1 - 1);

	rvec[0] = tmp2 * (2*st[0]-1);
	rvec[1] = tmp2 * (2*st[1]-1);
	rvec[2] = 8 * tmp1 - 3;
}
