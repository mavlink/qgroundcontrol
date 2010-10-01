
/* cylinder drawing demo */
/* this demo demonstrates the various join styles */

/* required include files */
#include <GL/glut.h>
#include <GL/tube.h>

/* ------------------------------------------------------- */

/* the arrays in which we will store the polyline */
#define NPTS 100
double points [NPTS][3];
float colors [NPTS][3];
int idx = 0;

/* some utilities for filling that array */
#define PSCALE 0.5
#define PNT(x,y,z) { 			\
   points[idx][0] = PSCALE * x; 	\
   points[idx][1] = PSCALE * y; 	\
   points[idx][2] = PSCALE * z;		\
   idx ++;				\
}

#define COL(r,g,b) { 			\
   colors[idx][0] = r; 			\
   colors[idx][1] = g; 			\
   colors[idx][2] = b;			\
}

/* the arrays in which we will store the contour */
#define NCONTOUR 100
double contour_points [NCONTOUR][2];
int cidx = 0;

/* some utilities for filling that array */
#define C_PNT(x,y) { 			\
   contour_points[cidx][0] = x;		\
   contour_points[cidx][1] = y; 	\
   cidx ++;				\
}


/* ------------------------------------------------------- */
/* 
 * Initialize a bent shape with three segments. 
 * The data format is a polyline.
 *
 * NOTE that neither the first, nor the last segment are drawn.
 * The first & last segment serve only to determine that angle 
 * at which the endcaps are drawn.
 */

void InitStuff (void) {

   COL (0.0, 0.0, 0.0);
   PNT (16.0, 0.0, 0.0);

   COL (0.2, 0.8, 0.5);
   PNT (0.0, -16.0, 0.0);

   COL (0.0, 0.8, 0.3);
   PNT (-16.0, 0.0, 0.0);

   COL (0.8, 0.3, 0.0);
   PNT (0.0, 16.0, 0.0);

   COL (0.2, 0.3, 0.9);
   PNT (16.0, 0.0, 0.0);

   COL (0.2, 0.8, 0.5);
   PNT (0.0, -16.0, 0.0);

   COL (0.0, 0.0, 0.0);
   PNT (-16.0, 0.0, 0.0);

   C_PNT (-0.8, -0.5);
   C_PNT (-1.8, 0.0);
   C_PNT (-1.2, 0.3);
   C_PNT (-0.7, 0.8);
   C_PNT (-0.2, 1.3);
   C_PNT (0.0, 1.6);
   C_PNT (0.2, 1.3);
   C_PNT (0.7, 0.8);
   C_PNT (1.2, 0.3);
   C_PNT (1.8, 0.0);
   C_PNT (0.8, -0.5);

   gleSetJoinStyle (TUBE_JN_ANGLE | TUBE_CONTOUR_CLOSED | TUBE_JN_CAP);
}

double up_vector[3] = {1.0, 0.0, 0.0};

extern float lastx;
extern float lasty;

/* ------------------------------------------------------- */
/* draw the extrusion */

void DrawStuff (void) {
   double moved_contour [NCONTOUR][2];
   int style, save_style;
   int i;

   for (i=0; i<cidx; i++) {
      moved_contour[i][0] = contour_points [i][0];
      moved_contour[i][1] = contour_points [i][1] + 0.05 * (lasty-200.0);
   }

   glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   /* set up some matrices so that the object spins with the mouse */
   glPushMatrix ();
   glTranslatef (0.0, 4.0, -80.0);
   glRotatef (0.5*lastx, 0.0, 1.0, 0.0);

   gleExtrusion (cidx, moved_contour, contour_points, up_vector, 
                 idx, points, colors);

   glPopMatrix ();


   /* draw a seond copy, this time with the raw style, to compare
    * things against */
   glPushMatrix ();
   glTranslatef (0.0, -4.0, -80.0);
   glRotatef (0.5*lastx, 0.0, 1.0, 0.0);

   save_style = gleGetJoinStyle ();
   style = save_style;
   style &= ~TUBE_JN_MASK;
   style |= TUBE_JN_RAW;
   gleSetJoinStyle (style);

   gleExtrusion (cidx, moved_contour, contour_points, up_vector, 
                 idx, points, colors);

   gleSetJoinStyle (save_style);
   glPopMatrix ();

   glutSwapBuffers ();
}

/* ------------------ end of file ----------------------------- */
