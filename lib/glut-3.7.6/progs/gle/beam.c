/* 
 * beam.c
 *
 * FUNCTION:
 * Show how twisting is applied.
 *
 * HISTORY:
 * -- linas Vepstas October 1991
 * -- heavily modified to draw corrugated surface, Feb 1993, Linas
 * -- modified to demo twistoid March 1993
 * -- port to glut Linas Vepstas March 1995
 */

/* required include files */
#include <math.h>
#include <stdlib.h>
#include <GL/glut.h>
#include <GL/tube.h>

/* =========================================================== */

#define NUM_BEAM_PTS 22 
double beam_spine[NUM_BEAM_PTS][3];
double beam_twists [NUM_BEAM_PTS];

#define TSCALE (6.0)

#define TPTS(x,y,z) {				\
   beam_spine[i][0] = TSCALE * (x);		\
   beam_spine[i][1] = TSCALE * (y);		\
   beam_spine[i][2] = TSCALE * (z);		\
   i++;						\
}

#define TXZERO() {				\
   beam_twists[i] = 0.0;			\
}
/* =========================================================== */

#define SCALE 0.1
#define XSECTION(x,y) {					\
   double ax, ay, alen;					\
   xsection[i][0] = SCALE * (x);			\
   xsection[i][1] = SCALE * (y);			\
   if (i!=0) {						\
      ax = xsection[i][0] - xsection[i-1][0];		\
      ay = xsection[i][1] - xsection[i-1][1];		\
      alen = 1.0 / sqrt (ax*ax + ay*ay);		\
      ax *= alen;   ay *= alen;				\
      xnormal [i-1][0] = - ay;				\
      xnormal [i-1][1] = ax;				\
   }							\
   i++;							\
}

#define NUM_XSECTION_PTS (12)

double xsection [NUM_XSECTION_PTS][2];
double xnormal [NUM_XSECTION_PTS][2];

/* =========================================================== */

void InitStuff (void)
{
   int i;

   i=0;
   while (i<22) {
      TXZERO ();
      TPTS (-1.1 +((float) i)/10.0, 0.0, 0.0);
   }

   i=0;
   XSECTION (-6.0, 6.0);
   XSECTION (6.0, 6.0);
   XSECTION (6.0, 5.0);
   XSECTION (1.0, 5.0);
   XSECTION (1.0, -5.0);
   XSECTION (6.0, -5.0);
   XSECTION (6.0, -6.0);
   XSECTION (-6.0, -6.0);
   XSECTION (-6.0, -5.0);
   XSECTION (-1.0, -5.0);
   XSECTION (-1.0, 5.0);
   XSECTION (-6.0, 5.0);
}

void TwistBeam (double howmuch) {

   int i;
   double z;
   for (i=0; i<22; i++) {
      z = ((double) (i-14)) / 10.0;
      beam_twists[i] = howmuch * exp (-3.0 * z*z);
   }
}

/* =========================================================== */

extern float lastx;

void DrawStuff (void) {
   TwistBeam ((double) (lastx -121) / 8.0);

   glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   /* set up some matrices so that the object spins with the mouse */
   glPushMatrix ();
   glTranslatef (0.0, 0.0, -80.0);
   glRotated (43.0, 1.0, 0.0, 0.0);
   glRotated (43.0, 0.0, 1.0, 0.0);
   glScaled (1.8, 1.8, 1.8);
   gleTwistExtrusion (NUM_XSECTION_PTS, xsection, xnormal, 
              NULL, NUM_BEAM_PTS, beam_spine, NULL, beam_twists);
   glPopMatrix ();
   glutSwapBuffers ();
}
/* ------------------ end of file -------------------- */
