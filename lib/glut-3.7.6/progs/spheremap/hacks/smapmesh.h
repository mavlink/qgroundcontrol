
/* Copyright (c) Mark J. Kilgard, 1998.  */

/* This program is freely distributable without licensing fees
   and is provided without guarantee or warrantee expressed or
   implied. This program is -not- in the public domain. */

/* smapmesh.h - header for cview2smap program */

#define XSTEPS  8
#define YSTEPS  8
#define SPOKES  8
#define RINGS   3

/* #define SPHERE_MAP_EDGE_EXTEND */

#ifdef SPHERE_MAP_EDGE_EXTEND
#define EDGE_EXTEND 1
#else
#define EDGE_EXTEND 0
#endif

typedef struct _STXY {
	GLfloat s, t;
	GLfloat x, y;
} STXY;

extern STXY face[5][YSTEPS][XSTEPS];
extern STXY back[4][RINGS+EDGE_EXTEND][SPOKES];

extern void makeSphereMapMesh(void);
extern void drawSphereMapMesh(GLuint texobj[6]);

extern void buildSphereMap(GLuint spheremap, GLuint texobjs[6], int texdim,
	GLfloat eye[3], GLfloat obj[3], GLfloat up[3],
	void (*positionLights)(int view, void *context),
	void (*drawView)(int view, void *context),
	int outline, void *context);
