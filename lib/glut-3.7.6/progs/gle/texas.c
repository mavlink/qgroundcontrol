/* 
 * texas.c
 * 
 * FUNCTION:
 * Draws a brand in the shape of Texas.  Both the handle, and the
 * cross-section of the brand are in the shape of Texas.
 *
 * Note that the contours are specified in clockwise order. 
 * Thus, enabling backfacing polygon removal will cause the front
 * polygons to disappear.
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

#define HNUM 4
double brand_points[HNUM][3];
float brand_colors [HNUM][3];

#define TSCALE  4.0

#define BPTS(x,y,z) {				\
   brand_points[i][0] = TSCALE * (x);		\
   brand_points[i][1] = TSCALE * (y);		\
   brand_points[i][2] = TSCALE * (z);		\
   i++;						\
}

#define BCOLS(r,g,b) {				\
   brand_colors[i][0] = (r);		 	\
   brand_colors[i][1] = (g);			\
   brand_colors[i][2] = (b);			\
   i++;						\
}

#define NUMPOINTS 18
double tspine[NUMPOINTS][3];
float tcolors [NUMPOINTS][3];

#define TPTS(x,y) {				\
   tspine[i][0] = TSCALE * (x);		\
   tspine[i][1] = TSCALE * (y);		\
   tspine[i][2] = TSCALE * (0.0);		\
   i++;						\
}

#define TCOLS(r,g,b) {				\
   tcolors[i][0] = (r);				\
   tcolors[i][1] = (g);				\
   tcolors[i][2] = (b);				\
   i++;						\
}

/* =========================================================== */

void init_spine (void)
{
   int i;
   int ir, ig, ib;
   float r, g, b;

   i=0;
   TPTS (-1.5, 2.0);	/* panhandle */
   TPTS (-0.75, 2.0);
   TPTS (-0.75, 1.38);
   TPTS (-0.5, 1.25);
   TPTS (0.88, 1.12);
   TPTS (1.0, 0.62);
   TPTS (1.12, 0.1);
   TPTS (0.5, -0.5);
   TPTS (0.2, -1.12);	/* corpus */
   TPTS (0.3, -1.5);	/* brownsville */
   TPTS (-0.25, -1.45);
   TPTS (-1.06, -0.3);
   TPTS (-1.38, -0.3);
   TPTS (-1.65, -0.6);
   TPTS (-2.5, 0.5);   /* midland */
   TPTS (-1.5, 0.5);
   TPTS (-1.5, 2.0);	/* panhandle */
   TPTS (-0.75, 2.0);

   ir = ig = ib = 0;
   for (i=0; i<NUMPOINTS; i++) {
      ir += 33; ig +=47; ib +=89;
      ir %= 255; ig %= 255; ib %= 255;
      r = ((float) ir) / 255.0;
      g = ((float) ig) / 255.0;
      b = ((float) ib) / 255.0;
      
      tcolors[i][0] = (r);
      tcolors[i][1] = (g);
      tcolors[i][2] = (b);
   }

   i=0;
   BPTS (0.0, 0.0, 0.1);
   BPTS (0.0, 0.0, 0.0);
   BPTS (0.0, 0.0, -5.0);
   BPTS (0.0, 0.0, -5.1);

   i=0;
   BCOLS (1.0, 0.3, 0.0);
   BCOLS (1.0, 0.3, 0.0);
   BCOLS (1.0, 0.3, 0.0);
   BCOLS (1.0, 0.3, 0.0);
}

/* =========================================================== */

#define SCALE 0.8
#define BORDER(x,y) {						\
   double ax, ay, alen;						\
   texas_xsection[i][0] = SCALE * (x);				\
   texas_xsection[i][1] = SCALE * (y);				\
   if (i!=0) {							\
      ax = texas_xsection[i][0] - texas_xsection[i-1][0];		\
      ay = texas_xsection[i][1] - texas_xsection[i-1][1];		\
      alen = 1.0 / sqrt (ax*ax + ay*ay);			\
      ax *= alen;   ay *= alen;					\
      texas_normal [i-1][0] = - ay;				\
      texas_normal [i-1][1] = ax;				\
   }								\
   i++;								\
}

#define NUM_TEXAS_PTS (17)

double texas_xsection [NUM_TEXAS_PTS][2];
double texas_normal [NUM_TEXAS_PTS][2];

void init_xsection (void)
{
   int i;

   /* outline of extrusion */
   i=0;
   BORDER (-0.75, 2.0);
   BORDER (-0.75, 1.38);
   BORDER (-0.5, 1.25);
   BORDER (0.88, 1.12);
   BORDER (1.0, 0.62);
   BORDER (1.12, 0.1);
   BORDER (0.5, -0.5);
   BORDER (0.2, -1.12);	/* corpus */
   BORDER (0.3, -1.5);	/* brownsville */
   BORDER (-0.25, -1.45);
   BORDER (-1.06, -0.3);
   BORDER (-1.38, -0.3);
   BORDER (-1.65, -0.6);
   BORDER (-2.5, 0.5);   /* midland */
   BORDER (-1.5, 0.5);
   BORDER (-1.5, 2.0);	/* panhandle */
   BORDER (-0.75, 2.0);
}

   
/* =========================================================== */

void InitStuff (void)
{
   int style;

   /* configure the pipeline */
   init_spine ();
   init_xsection ();

   style = TUBE_JN_CAP;
   style |= TUBE_CONTOUR_CLOSED;
   style |= TUBE_NORM_FACET;
   style |= TUBE_JN_ANGLE;
   gleSetJoinStyle (style);
}

/* =========================================================== */

extern float lastx;
extern float lasty;

void DrawStuff (void) {

#ifdef IBM_GL_32
   scale (1.8, 1.8, 1.8);

   lmcolor (material_mode);
   (void) extrusion (NUM_TEXAS_PTS-1, texas_xsection, texas_normal, 
                    NULL, NUMPOINTS, tspine, tcolors);

   (void) extrusion (NUM_TEXAS_PTS-1, texas_xsection, texas_normal, 
                    NULL, HNUM, brand_points, brand_colors);

   lmcolor (LMC_COLOR);
#endif

#define OPENGL_10
#ifdef OPENGL_10
   glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   /* set up some matrices so that the object spins with the mouse */
   glPushMatrix ();
   glTranslatef (0.0, 0.0, -80.0);
   glRotatef (lastx, 0.0, 1.0, 0.0);
   glRotatef (lasty, 1.0, 0.0, 0.0);

   /* draw the brand and the handle */
   gleExtrusion (NUM_TEXAS_PTS-1, texas_xsection, texas_normal, 
                 NULL, NUMPOINTS, tspine, tcolors);

   gleExtrusion (NUM_TEXAS_PTS-1, texas_xsection, texas_normal, 
                 NULL, HNUM, brand_points, brand_colors);

   glPopMatrix ();
   glutSwapBuffers ();
}
#endif

/* ===================== END OF FILE ================== */

