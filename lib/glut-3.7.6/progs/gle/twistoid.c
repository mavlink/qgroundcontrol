/* 
 * twistoid.c
 *
 * FUNCTION:
 * Show extrusion of open contours. Also, show how torsion is applied.
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

/* Some <math.h> files do not define M_PI... */
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

extern float lastx;
extern float lasty;

#define OPENGL_10
/* =========================================================== */

#define NUM_TOID1_PTS 5
double toid1_points[NUM_TOID1_PTS][3];
float toid1_colors [NUM_TOID1_PTS][3];
double toid1_twists [NUM_TOID1_PTS];

#define TSCALE (6.0)

#define TPTS(x,y) {				\
   toid1_points[i][0] = TSCALE * (x);		\
   toid1_points[i][1] = TSCALE * (y);		\
   toid1_points[i][2] = TSCALE * (0.0);		\
   i++;						\
}

#define TCOLS(r,g,b) {				\
   toid1_colors[i][0] = (r);			\
   toid1_colors[i][1] = (g);			\
   toid1_colors[i][2] = (b);			\
   i++;						\
}

#define TXZERO() {				\
   toid1_twists[i] = 0.0;			\
   i++;						\
}

void init_toid1_line (void)
{
   int i;

   i=0;
   TPTS (-1.1, 0.0);
   TPTS (-1.0, 0.0);
   TPTS (0.0, 0.0);
   TPTS (1.0, 0.0);
   TPTS (1.1, 0.0);

   i=0;
   TCOLS (0.8, 0.8, 0.5);
   TCOLS (0.8, 0.4, 0.5);
   TCOLS (0.8, 0.8, 0.3);
   TCOLS (0.4, 0.4, 0.5);
   TCOLS (0.8, 0.8, 0.5);

   i=0;
   TXZERO ();
   TXZERO ();
   TXZERO ();
   TXZERO ();
   TXZERO ();
}

/* =========================================================== */

#define SCALE 0.6
#define TWIST(x,y) {						\
   double ax, ay, alen;						\
   twistation[i][0] = SCALE * (x);				\
   twistation[i][1] = SCALE * (y);				\
   if (i!=0) {							\
      ax = twistation[i][0] - twistation[i-1][0];		\
      ay = twistation[i][1] - twistation[i-1][1];		\
      alen = 1.0 / sqrt (ax*ax + ay*ay);			\
      ax *= alen;   ay *= alen;					\
      twist_normal [i-1][0] = - ay;				\
      twist_normal [i-1][1] = ax;				\
   }								\
   i++;								\
}

#define NUM_TWIS_PTS (20)

double twistation [NUM_TWIS_PTS][2];
double twist_normal [NUM_TWIS_PTS][2];

void init_tripples (void)
{
   int i;
   double angle;
   double co, si;

   /* outline of extrusion */
   i=0;
   /* first, draw a semi-curcular "hump" */
   while (i< 11) {
      angle = M_PI * ((double) i) / 10.0;
      co = cos (angle);
      si = sin (angle);
      TWIST ((-7.0 -3.0*co), 1.8*si);
   }

   /* now, a zig-zag corrugation */
   while (i< NUM_TWIS_PTS) {
      TWIST ((-10.0 +(double) i), 0.0);
      TWIST ((-9.5 +(double) i), 1.0);
   }
}

   
/* =========================================================== */

#define V3F(x,y,z) {					\
	float vvv[3]; 					\
	vvv[0] = x; vvv[1] = y; vvv[2] = z; v3f (vvv); 	\
}

#define N3F(x,y,z) {					\
	float nnn[3]; 					\
	nnn[0] = x; nnn[1] = y; nnn[2] = z; n3f (nnn); 	\
}

/* =========================================================== */

void DrawStuff (void) {
   int i;

   toid1_twists[2] = (lastx-121.0) / 8.0;

   i=3;
/*
   TPTS (1.0, lasty /400.0);
   TPTS (1.1, 1.1 * lasty / 400.0);
*/
   TPTS (1.0, -(lasty-121.0) /200.0);
   TPTS (1.1, -1.1 * (lasty-121.0) / 200.0);

#ifdef IBM_GL_32
   rotate (230, 'x');
   rotate (230, 'y');
   scale (1.8, 1.8, 1.8);

   if (mono_color) {
      RGBcolor (178, 178, 204);
      twist_extrusion (NUM_TWIS_PTS, twistation, twist_normal, 
                NULL, NUM_TOID1_PTS, toid1_points, NULL, toid1_twists);
   } else {
      twist_extrusion (NUM_TWIS_PTS, twistation, twist_normal, 
              NULL, NUM_TOID1_PTS, toid1_points, toid1_colors, toid1_twists);
   }
#endif

#ifdef OPENGL_10
   glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   /* set up some matrices so that the object spins with the mouse */
   glPushMatrix ();
   glTranslatef (0.0, 0.0, -80.0);
   glRotated (43.0, 1.0, 0.0, 0.0);
   glRotated (43.0, 0.0, 1.0, 0.0);
   glScaled (1.8, 1.8, 1.8);
   gleTwistExtrusion (NUM_TWIS_PTS, twistation, twist_normal, 
              NULL, NUM_TOID1_PTS, toid1_points, NULL, toid1_twists);
   glPopMatrix ();
   glutSwapBuffers ();
#endif

}

/* =========================================================== */

void InitStuff (void) {
   int js;

   init_toid1_line ();
   init_tripples ();

#ifdef IBM_GL_32
   js = getjoinstyle ();
   js &= ~TUBE_CONTOUR_CLOSED;
   setjoinstyle (js);
#endif

#ifdef OPENGL_10
   js = gleGetJoinStyle ();
   js &= ~TUBE_CONTOUR_CLOSED;
   gleSetJoinStyle (js);
#endif

}

/* ------------------ end of file -------------------- */
