
/* 
 * cone drawing demo 
 *
 * FUNCTION:
 * Baisc demo illustrating how to write code to draw
 * the most basic cone shape.
 *
 * HISTORY:
 * Linas Vepstas March 1995
 */

/* required include files */
#include <GL/glut.h>
#include <GL/tube.h>

/* the arrays in which we will store out polyline */
#define NPTS 6
double radii [NPTS];
double points [NPTS][3];
float colors [NPTS][3];
int idx = 0;

/* some utilities for filling that array */
#define PNT(x,y,z) { 			\
   points[idx][0] = x; 			\
   points[idx][1] = y; 			\
   points[idx][2] = z;			\
   idx ++;				\
}

#define COL(r,g,b) { 			\
   colors[idx][0] = r; 			\
   colors[idx][1] = g; 			\
   colors[idx][2] = b;			\
}

#define RAD(r) {			\
   radii[idx] = r;			\
}

/* 
 * Initialize a bent shape with three segments. 
 * The data format is a polyline.
 *
 * NOTE that neither the first, nor the last segment are drawn.
 * The first & last segment serve only to determine that angle 
 * at which the endcaps are drawn.
 */

void InitStuff (void) {

   /* initialize the join style here */
   gleSetJoinStyle (TUBE_NORM_EDGE | TUBE_JN_ANGLE | TUBE_JN_CAP);

   RAD (1.0);
   COL (0.0, 0.0, 0.0);
   PNT (-6.0, 6.0, 0.0);

   RAD (1.0);
   COL (0.0, 0.8, 0.3);
   PNT (6.0, 6.0, 0.0);

   RAD (3.0);
   COL (0.8, 0.3, 0.0);
   PNT (6.0, -6.0, 0.0);

   RAD (0.5);
   COL (0.2, 0.3, 0.9);
   PNT (-6.0, -6.0, 0.0);

   RAD (2.0);
   COL (0.2, 0.8, 0.5);
   PNT (-6.0, 6.0, 0.0);

   RAD (1.0);
   COL (0.0, 0.0, 0.0);
   PNT (6.0, 6.0, 0.0);
}

extern float lastx;
extern float lasty;

/* draw the polycone shape */
void DrawStuff (void) {

   glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   /* set up some matrices so that the object spins with the mouse */
   glPushMatrix ();
   glTranslatef (0.0, 0.0, -80.0);
   glRotatef (lastx, 0.0, 1.0, 0.0);
   glRotatef (lasty, 1.0, 0.0, 0.0);

   /* Phew. FINALLY, Draw the polycone  -- */
   glePolyCone (idx, points, colors, radii);

   glPopMatrix ();

   glutSwapBuffers ();
}

/* --------------------------- end of file ------------------- */
