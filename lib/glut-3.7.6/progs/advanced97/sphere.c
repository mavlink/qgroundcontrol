#include <stdlib.h>
#include <GL/glut.h>
#include <math.h>

#ifndef __sgi
/* Most math.h's do not define float versions of the math functions. */
#define sqrtf(x) ((float)sqrt((x)))
#endif

/* sphere tessellation code based on code originally
 * written by Jon Leech (leech@cs.unc.edu) 3/24/89
 */

typedef struct {
    float x, y, z;
} point;

typedef struct {
    point pt[3];
} triangle;

/* six equidistant points lying on the unit sphere */
#define XPLUS {  1,  0,  0 }	/*  X */
#define XMIN  { -1,  0,  0 }	/* -X */
#define YPLUS {  0,  1,  0 }	/*  Y */
#define YMIN  {  0, -1,  0 }	/* -Y */
#define ZPLUS {  0,  0,  1 }	/*  Z */
#define ZMIN  {  0,  0, -1 }	/* -Z */

/* for icosahedron */
#define CZ (0.866025403)	/*  cos(30) */
#define SZ (0.5)		/*  sin(30) */
#define C1 (0.951056516)	/* cos(18),  */
#define S1 (0.309016994)	/* sin(18) */
#define C2 (0.587785252)	/* cos(54),  */
#define S2 (0.809016994)	/* sin(54) */
#define X1 (C1*CZ)
#define Y1 (S1*CZ)
#define X2 (C2*CZ)
#define Y2 (S2*CZ)

#define Ip0 	{0., 	0., 	1.}
#define Ip1	{-X2, 	-Y2, 	SZ}
#define Ip2	{X2, 	-Y2, 	SZ}
#define Ip3	{X1, 	Y1, 	SZ}
#define Ip4	{0, 	CZ, 	SZ}
#define Ip5	{-X1, 	Y1, 	SZ}

#define Im0	{-X1, 	-Y1, 	-SZ}
#define Im1	{0, 	-CZ, 	-SZ}
#define Im2	{X1, 	-Y1, 	-SZ}
#define Im3	{X2, 	Y2, 	-SZ}
#define Im4	{-X2, 	Y2, 	-SZ}
#define Im5	{0., 	0., 	-1.}

/* vertices of a unit icosahedron */
static triangle icosahedron[20]= {
	/* front pole */
	{ {Ip0, Ip1, Ip2}, },
	{ {Ip0, Ip5, Ip1}, },
	{ {Ip0, Ip4, Ip5}, },
	{ {Ip0, Ip3, Ip4}, },
	{ {Ip0, Ip2, Ip3}, },

	/* mid */
	{ {Ip1, Im0, Im1}, },
	{ {Im0, Ip1, Ip5}, },
	{ {Ip5, Im4, Im0}, },
	{ {Im4, Ip5, Ip4}, },
	{ {Ip4, Im3, Im4}, },
	{ {Im3, Ip4, Ip3}, },
	{ {Ip3, Im2, Im3}, },
	{ {Im2, Ip3, Ip2}, },
	{ {Ip2, Im1, Im2}, },
	{ {Im1, Ip2, Ip1}, },

	/* back pole */
	{ {Im3, Im2, Im5}, },
	{ {Im4, Im3, Im5}, },
	{ {Im0, Im4, Im5}, },
	{ {Im1, Im0, Im5}, },
	{ {Im2, Im1, Im5}, },
};

/* normalize point r */
static void
normalize(point *r) {
    float mag;

    mag = r->x * r->x + r->y * r->y + r->z * r->z;
    if (mag != 0.0f) {
	mag = 1.0f / sqrtf(mag);
	r->x *= mag;
	r->y *= mag;
	r->z *= mag;
    }
}

/* linearly interpolate between a & b, by fraction f */
static void
lerp(point *a, point *b, float f, point *r) {
    r->x = a->x + f*(b->x-a->x);
    r->y = a->y + f*(b->y-a->y);
    r->z = a->z + f*(b->z-a->z);
}

void
sphere(int maxlevel) {
    int nrows = 1 << maxlevel;
    int s;

    /* iterate over the 20 sides of the icosahedron */
    for(s = 0; s < 20; s++) {
	int i;
	triangle *t = &icosahedron[s];
	for(i = 0; i < nrows; i++) {
	    /* create a tstrip for each row */
	    /* number of triangles in this row is number in previous +2 */
	    /* strip the ith trapezoid block */
	    point v0, v1, v2, v3, va, vb;
	    int j;
	    lerp(&t->pt[1], &t->pt[0], (float)(i+1)/nrows, &v0);
	    lerp(&t->pt[1], &t->pt[0], (float)i/nrows, &v1);
	    lerp(&t->pt[1], &t->pt[2], (float)(i+1)/nrows, &v2);
	    lerp(&t->pt[1], &t->pt[2], (float)i/nrows, &v3);
	    glBegin(GL_TRIANGLE_STRIP);
#define V(v)	{ point x; x = v; normalize(&x); glNormal3fv(&x.x); glVertex3fv(&x.x); }
	    V(v0);
	    V(v1);
	    for(j = 0; j < i; j++) {
		/* calculate 2 more vertices at a time */
		lerp(&v0, &v2, (float)(j+1)/(i+1), &va);
		lerp(&v1, &v3, (float)(j+1)/i, &vb);
		V(va);
		V(vb);
	    }
	    V(v2);
#undef V
	    glEnd();
	}
    }
}

float *
sphere_tris(int maxlevel) {
    int nrows = 1 << maxlevel;
    int s, n;
    float *buf, *b;

    n = 20*(1 << (maxlevel * 2));
    b = buf = (float *)malloc(n*3*3*sizeof(float));

    /* iterate over the 20 sides of the icosahedron */
    for(s = 0; s < 20; s++) {
	int i;
	triangle *t = &icosahedron[s];
	for(i = 0; i < nrows; i++) {
	    /* create a tstrip for each row */
	    /* number of triangles in this row is number in previous +2 */
	    /* strip the ith trapezoid block */
	    point v0, v1, v2, v3, va, vb, x1, x2;
	    int j;
	    lerp(&t->pt[1], &t->pt[0], (float)(i+1)/nrows, &v0);
	    lerp(&t->pt[1], &t->pt[0], (float)i/nrows, &v1);
	    lerp(&t->pt[1], &t->pt[2], (float)(i+1)/nrows, &v2);
	    lerp(&t->pt[1], &t->pt[2], (float)i/nrows, &v3);
#define V(a, c, v)	{ point x = v; normalize(&a); normalize(&c); normalize(&x); \
			b[0] = a.x; b[1] = a.y; b[2] = a.z; \
			b[3] = c.x; b[4] = c.y; b[5] = c.z; \
			b[6] = x.x; b[7] = x.y; b[8] = x.z; b+=9; }
	    x1 = v0;
	    x2 = v1;
	    for(j = 0; j < i; j++) {
		/* calculate 2 more vertices at a time */
		lerp(&v0, &v2, (float)(j+1)/(i+1), &va);
		lerp(&v1, &v3, (float)(j+1)/i, &vb);
		V(x1,x2,va); x1 = x2; x2 = va;
		V(vb,x2,x1); x1 = x2; x2 = vb;
	    }
	    V(x1, x2, v2);
#undef V
	}
    }
    return buf;
}
