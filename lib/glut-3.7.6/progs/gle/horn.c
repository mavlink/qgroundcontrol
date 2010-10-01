
/* 
 * hron -- cone drawing demo 
 *
 * FUNCTION:
 * Baisc demo illustrating how to write code to draw
 * the a slightly fancier "polycone".
 *
 * HISTORY:
 * Linas Vepstas March 1995
 */

/* required include files */
#include <GL/glut.h>
#include <GL/tube.h>

/* the arrays in which we will store out polyline */
#define NPTS 26
double radii [NPTS];
double points [NPTS][3];
int idx = 0;

/* some utilities for filling that array */
#define PNT(x,y,z) { 			\
   points[idx][0] = x; 			\
   points[idx][1] = y; 			\
   points[idx][2] = z;			\
   idx ++;				\
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
   gleSetJoinStyle (TUBE_NORM_PATH_EDGE | TUBE_JN_ANGLE );

   RAD (0.3);
   PNT (-4.9, 6.0, 0.0);

   RAD (0.3);
   PNT (-4.8, 5.8, 0.0);

   RAD (0.3);
   PNT (-3.8, 5.8, 0.0);

   RAD (0.6);
   PNT (-3.5, 6.0, 0.0);

   RAD (0.8);
   PNT (-3.0, 7.0, 0.0);

   RAD (0.9);
   PNT (-2.4, 7.6, 0.0);

   RAD (1.0);
   PNT (-1.8, 7.6, 0.0);

   RAD (1.1);
   PNT (-1.2, 7.1, 0.0);

   RAD (1.2);
   PNT (-0.8, 5.1, 0.0);

   RAD (1.7);
   PNT (-0.3, -2.0, 0.0);

   RAD (1.8);
   PNT (-0.2, -7.0, 0.0);

   RAD (2.0);
   PNT (0.3, -7.8, 0.0);

   RAD (2.1);
   PNT (0.8, -8.2, 0.0);

   RAD (2.25);
   PNT (1.8, -8.6, 0.0);

   RAD (2.4);
   PNT (3.6, -8.6, 0.0);

   RAD (2.5);
   PNT (4.5, -8.2, 0.0);

   RAD (2.6);
   PNT (4.8, -7.5, 0.0);

   RAD (2.7);
   PNT (5.0, -6.0, 0.0);

   RAD (3.2);
   PNT (6.4, -2.0, 0.0);

   RAD (4.1);
   PNT (6.9, -1.0, 0.0);

   RAD (4.1);
   PNT (7.8, 0.5, 0.0);

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
   glColor3f (0.5, 0.5, 0.2);

   /* Phew. FINALLY, Draw the polycone  -- */
   glePolyCone (idx, points, 0x0, radii);

   glPopMatrix ();

   glutSwapBuffers ();
}

/* --------------------------- end of file ------------------- */
