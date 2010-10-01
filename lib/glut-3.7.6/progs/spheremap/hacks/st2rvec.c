
/* Copyright (c) Mark J. Kilgard, 1998.  */

/* This program is freely distributable without licensing fees
   and is provided without guarantee or warrantee expressed or
   implied. This program is -not- in the public domain. */

/* st2rvec - converts between reflection vectors and sphere map s & t */

#include <stdio.h>
#include <math.h>

void
rvec2st(float rx, float ry, float rz, float *sp, float *tp)
{
	double s, t;
	double m;

	m = 2 * sqrt(rx*rx + ry*ry + (rz+1)*(rz+1));

	s = rx/m + 0.5;
	t = ry/m + 0.5;

	*sp = (float) s;
	*tp = (float) t;
}

double
dist(float x, float y, float z)
{
	return sqrt(x*x + y*y + z*z);
}

void
st2rvec(float s, float t, float *xp, float *yp, float *zp)
{
	double rx, ry, rz;
	double tmp1, tmp2;

	tmp1 = s*(1-s) + t*(1-t);
	tmp2 = 2 * sqrt(4*tmp1 - 1);

	rx = tmp2 * (2*s-1);
	ry = tmp2 * (2*t-1);
	rz = 8 * tmp1 - 3;

	*xp = (float) rx;
	*yp = (float) ry;
	*zp = (float) rz;
}

int
main(int argc, char **argv)
{
	float s, t;
	float x, y, z;
	int count;

	printf("\nYOU ENTER S & T SPHERE MAP TEXTURE COORDINATES...\n");
	printf("\n  I CONVERT IT TO A NORMALIZED REFLECTION VECTOR");
	printf("\n      (AND CONVERT BACK!)\n\n");

        while (!feof(stdin)) {
		printf("(s,t) > ");
		count = scanf("%f%f", &s, &t);
		if (count == EOF) {
			break;
		} else if (count == 2) {
			printf("READ s=%f, t=%f\n\n", s, t);

			st2rvec(s, t, &x, &y, &z);
			printf("CONVERTING TO REFLECTION VECTOR:\n");
			printf("  x=%f, y=%f, z=%f\n\n", x, y, z);

			printf("CONVERTING BACK TO TEXTURE COORDINATE:\n");
			rvec2st(x, y, z, &s, &t);
			printf("  s=%f, t=%f\n\n", s, t);
		} else {
			printf("BAD INPUT!\n\n");
			break;
		}
	}

	return 0;
}
