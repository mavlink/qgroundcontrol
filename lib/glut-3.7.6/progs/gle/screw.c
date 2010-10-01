/* 
 * screw.c
 * 
 * FUNCTION:
 * Draws a screw shape.
 *
 * HISTORY:
 * -- created by Linas Vepstas October 1991
 * -- heavily modified to draw more texas shapes, Feb 1993, Linas
 * -- converted to use GLUT -- December 1995, Linas
 *
 */

/* required include files */
#include <stdlib.h>
#include <math.h>
#include <GL/glut.h>
#include <GL/tube.h>

/* =========================================================== */

#define SCALE 1.3
#define CONTOUR(x,y) {					\
   double ax, ay, alen;					\
   contour[i][0] = SCALE * (x);				\
   contour[i][1] = SCALE * (y);				\
   if (i!=0) {						\
      ax = contour[i][0] - contour[i-1][0];		\
      ay = contour[i][1] - contour[i-1][1];		\
      alen = 1.0 / sqrt (ax*ax + ay*ay);		\
      ax *= alen;   ay *= alen;				\
      norms [i-1][0] = ay;				\
      norms [i-1][1] = -ax;				\
   }							\
   i++;							\
}

#define NUM_PTS (25)

double contour [NUM_PTS][2];
double norms [NUM_PTS][2];

void init_contour (void)
{
   int i;

   /* outline of extrusion */
   i=0;
   CONTOUR (1.0, 1.0);
   CONTOUR (1.0, 2.9);
   CONTOUR (0.9, 3.0);
   CONTOUR (-0.9, 3.0);
   CONTOUR (-1.0, 2.9);

   CONTOUR (-1.0, 1.0);
   CONTOUR (-2.9, 1.0);
   CONTOUR (-3.0, 0.9);
   CONTOUR (-3.0, -0.9);
   CONTOUR (-2.9, -1.0);

   CONTOUR (-1.0, -1.0);
   CONTOUR (-1.0, -2.9);
   CONTOUR (-0.9, -3.0);
   CONTOUR (0.9, -3.0);
   CONTOUR (1.0, -2.9);

   CONTOUR (1.0, -1.0);
   CONTOUR (2.9, -1.0);
   CONTOUR (3.0, -0.9);
   CONTOUR (3.0, 0.9);
   CONTOUR (2.9, 1.0);

   CONTOUR (1.0, 1.0);   /* repeat so that last normal is computed */
}
   
/* =========================================================== */

extern float lastx;
extern float lasty;

extern void TextureStyle (int msg);

void InitStuff (void)
{
   int style;

   /* pick model-vertex-cylinder coords for texture mapping */
   TextureStyle (509);

   /* configure the pipeline */
   style = TUBE_JN_CAP;
   style |= TUBE_CONTOUR_CLOSED;
   style |= TUBE_NORM_FACET;
   style |= TUBE_JN_ANGLE;
   gleSetJoinStyle (style);

   lastx = 121.0;
   lasty = 121.0;

   init_contour();
}

/* =========================================================== */

void DrawStuff (void) {

   glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   glColor3f (0.5, 0.6, 0.6);

   /* set up some matrices so that the object spins with the mouse */
   glPushMatrix ();
   glTranslatef (0.0, 0.0, -80.0);
   glRotatef (130.0, 0.0, 1.0, 0.0);
   glRotatef (65.0, 1.0, 0.0, 0.0);

   /* draw the brand and the handle */
   gleScrew (20, contour, norms, 
                 NULL, -6.0, 9.0, lasty);

   glPopMatrix ();
   glutSwapBuffers ();
}

/* ===================== END OF FILE ================== */
