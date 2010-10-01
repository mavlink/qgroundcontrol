/* 
 * taper.c
 * 
 * FUNCTION:
 * Draws a tapered screw shape.
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

/* Some <math.h> files do not define M_PI... */
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* =========================================================== */

#define SCALE 3.33333
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

#define PSIZE 40
double path[PSIZE][3];
double twist[PSIZE];
double taper[PSIZE];

void init_taper (void) {
   int j;
   double z, deltaz;
   double ang, dang;

   z = -10.0;
   deltaz = 0.5;

   ang = 0.0;
   dang = 20.0;
   for (j=0; j<40; j++) {
      path[j][0] = 0x0;
      path[j][1] = 0x0;
      path[j][2] = z;

      twist[j] = ang;
      ang += dang;

      taper[j] = 0.1 * sqrt (9.51*9.51 - z*z);

      z += deltaz;
   }

   taper[0] = taper[1];
   taper[39] = taper[38];

}

/* =========================================================== */

extern float lastx;
extern float lasty;

void InitStuff (void)
{
   int style;

   /* configure the pipeline */
   style = TUBE_JN_CAP;
   style |= TUBE_CONTOUR_CLOSED;
   style |= TUBE_NORM_FACET;
   style |= TUBE_JN_ANGLE;
   gleSetJoinStyle (style);

   lastx = 121.0;
   lasty = 121.0;

   init_contour();
   init_taper();
}

/* =========================================================== */

void gleTaper (int ncp, 
               gleDouble contour[][2], 
               gleDouble cont_normal[][2], 
               gleDouble up[3],
               int npoints,
               gleDouble point_array[][3],
               float color_array[][3],
               gleDouble taper[],
               gleDouble twist[])
{
   int j;
   gleAffine *xforms;
   double co, si, angle;

   /* malloc the extrusion array and the twist array */
   xforms = (gleAffine *) malloc (npoints * sizeof(gleAffine));

   for (j=0; j<npoints; j++) {
      angle = (M_PI/180.0) * twist[j];
      si = sin (angle);
      co = cos (angle);
      xforms[j][0][0] = taper[j] * co;
      xforms[j][0][1] = - taper[j] * si;
      xforms[j][0][2] = 0.0;
      xforms[j][1][0] = taper[j] * si;
      xforms[j][1][1] = taper[j] * co;
      xforms[j][1][2] = 0.0;
   }

   gleSuperExtrusion (ncp,               /* number of contour points */
                contour,    /* 2D contour */
                cont_normal, /* 2D contour normals */
                up,           /* up vector for contour */
                npoints,           /* numpoints in poly-line */
                point_array,        /* polyline */
                color_array,        /* color of polyline */
                xforms);

   free (xforms);
}

/* =========================================================== */

void DrawStuff (void) {
   int j;
   double ang, dang;
   double z, deltaz;
   double ponent;
   z=-1.0;
   deltaz = 1.999/38;
   ang = 0.0;
   dang = lasty/40.0;
   ponent = fabs (lastx/540.0);
   for (j=1; j<39; j++) {
      twist[j] = ang;
      ang += dang;

      taper[j] = pow ((1.0 - pow (fabs(z), 1.0/ponent)), ponent);
      z += deltaz;
   }

   glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   glColor3f (0.5, 0.6, 0.6);

   /* set up some matrices so that the object spins with the mouse */
   glPushMatrix ();
   glTranslatef (0.0, 0.0, -80.0);
   glRotatef (130.0, 0.0, 1.0, 0.0);
   glRotatef (65.0, 1.0, 0.0, 0.0);

   /* draw the brand and the handle */
   gleTaper (20, contour, norms,  NULL, 40, path, NULL, taper, twist);

   glPopMatrix ();
   glutSwapBuffers ();
}

/* ===================== END OF FILE ================== */
