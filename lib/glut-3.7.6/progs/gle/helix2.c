
/* 
 * helicoid (gernalized torus) demo 
 *
 * FUNCTION:
 * This code provides a very simple example of the helicoid primitive.
 * Most of this code is required to set up OpenGL and GLUT, and very
 * very little to set up the helix drawer. Don't blink!
 *
 * =======> MOUSE HOOKED UP TO RADIUS, DELTA-RADIUS < ========
 *
 * HISTORY:
 * Written by Linas Vepstas, March 1995
 */

/* required include files */
#include <GL/glut.h>
#include <GL/tube.h>

/*  most recent mouse postion */
extern float lastx;
extern float lasty;

void InitStuff (void) {
   lastx = 121.0;
   lasty = 121.0;
}

/* draw the helix shape */
void DrawStuff (void) {

   glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   glColor3f (0.6, 0.3, 0.8);

   /* set up some matrices so that the object spins with the mouse */
   glPushMatrix ();
   glTranslatef (0.0, 0.0, -80.0);
   glRotatef (220.0, 0.0, 1.0, 0.0);
   glRotatef (65.0, 1.0, 0.0, 0.0);

   /* Phew. FINALLY, Draw the helix  -- */
   gleSetJoinStyle (TUBE_NORM_EDGE | TUBE_JN_ANGLE | TUBE_JN_CAP);
   gleHelicoid (0.01*lastx, 
                6.0, (0.01*lasty - 2.0), 
               -3.0, 4.0, 0x0, 0x0, 0.0, 1080.0);

   glPopMatrix ();

   glutSwapBuffers ();
}
/* ------------------------- end of file ----------------- */
