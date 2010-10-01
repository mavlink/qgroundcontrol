/*
 * MODULE NAME: round_cap.c
 *
 * FUNCTION:
 * This module contains code that draws the round end-cap for round
 * join-style tubing.
 *
 * HISTORY:
 * written by Linas Vepstas August/September 1991
 * split into multiple compile units, Linas, October 1991
 * added normal vectors Linas, October 1991
 */


#include <malloc.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>	/* for the memcpy() subroutine */
#include <GL/tube.h>
#include "port.h"
#include "gutil.h"
#include "vvector.h"
#include "extrude.h"
#include "tube_gc.h"
#include "intersect.h"
#include "segment.h"


/* ============================================================ */
/* This routine does what it says: It draws the end-caps for the
 * "round" join style.
 */

/* HACK ALERT HACK ALERT HACK ALERT HACK ALERT */
/* This #define should be replaced by some adaptive thingy.
 * the adaptiveness needs to depend on relative angles and diameter of
 * extrusion relative to screen size (in pixels).
 */

#define __ROUND_TESS_PIECES 5

void draw_round_style_cap_callback (int ncp,
                                  double cap[][3],
                                  float face_color[3],
                                  gleDouble cut[3],
                                  gleDouble bi[3],
                                  double norms[][3],
                                  int frontwards)
{
   double axis[3];
   double xycut[3];
   double theta;
   double *last_contour, *next_contour;
   double *last_norm, *next_norm;
   double *cap_z;
   double *tmp;
   char *malloced_area;
   int i, j, k;
   double m[4][4];

   if (face_color != NULL) C3F (face_color);

   /* ------------ start setting up rotation matrix ------------- */
   /* if the cut vector is NULL (this should only occur in
    * a degenerate case), then we can't draw anything. return. */
   if (cut == NULL) return;

   /* make sure that the cut vector points inwards */
   if (cut[2] > 0.0) {
      VEC_SCALE (cut, -1.0, cut);
   }

   /* make sure that the bi vector points outwards */
   if (bi[2] < 0.0) {
      VEC_SCALE (bi, -1.0, bi);
   }

   /* determine the axis we are to rotate about to get bi-contour.
    * Note that the axis will always lie in the x-y plane */
   VEC_CROSS_PRODUCT (axis, cut, bi);

   /* reverse the cut vector for the back cap -- 
    * need to do this to get angle right */
   if (!frontwards) {
      VEC_SCALE (cut, -1.0, cut);
   }

   /* get angle to rotate by -- arccos of dot product of cut with cut
    * projected into the x-y plane */
   xycut [0] = 0.0;
   xycut [1] = 0.0;
   xycut [2] = 1.0;
   VEC_PERP (xycut, cut, xycut);
   VEC_NORMALIZE (xycut);
   VEC_DOT_PRODUCT (theta, xycut, cut);

   theta = acos (theta);

   /* we'll tesselate round joins into a number of teeny pieces */
   theta /= (double) __ROUND_TESS_PIECES;

   /* get the matrix */
   urot_axis_d (m, theta, axis);

   /* ------------ done setting up rotation matrix ------------- */

   /* This malloc is a fancy version of:
    * last_contour = (double *) malloc (3*ncp*sizeof(double);
    * next_contour = (double *) malloc (3*ncp*sizeof(double);
    */
   malloced_area = malloc ((4*3+1) *ncp*sizeof (double));
   last_contour = (double *) malloced_area;
   next_contour = last_contour +  3*ncp;
   cap_z = next_contour + 3*ncp;
   last_norm = cap_z + ncp;
   next_norm = last_norm + 3*ncp;

   /* make first copy of contour */
   if (frontwards) {
      for (j=0; j<ncp; j++) {
         last_contour[3*j] = cap[j][0];
         last_contour[3*j+1] = cap[j][1];
         last_contour[3*j+2] = cap_z[j] = cap[j][2];
      }

      if (norms != NULL) {
         for (j=0; j<ncp; j++) {
            VEC_COPY ((&last_norm[3*j]), norms[j]);
         }
      }
   } else {
      /* in order for backfacing polygon removal to work correctly, have
       * to have the sense in which the joins are drawn to be reversed 
       * for the back cap.  This can be done by reversing the order of
       * the contour points.  Normals are a bit trickier, since the 
       * reversal is off-by-one for facet normals as compared to edge 
       * normals. */
      for (j=0; j<ncp; j++) {
         k = ncp - j - 1;
         last_contour[3*k] = cap[j][0];
         last_contour[3*k+1] = cap[j][1];
         last_contour[3*k+2] = cap_z[k] = cap[j][2];
      }

      if (norms != NULL) {
         if (__TUBE_DRAW_FACET_NORMALS) {
            for (j=0; j<ncp-1; j++) {
               k = ncp - j - 2;
               VEC_COPY ((&last_norm[3*k]), norms[j]);
            }
         } else {
            for (j=0; j<ncp; j++) {
               k = ncp - j - 1;
               VEC_COPY ((&last_norm[3*k]), norms[j]);
            }
         }
      }
   }

   /* &&&&&&&&&&&&&& start drawing cap &&&&&&&&&&&&& */

   for (i=0; i<__ROUND_TESS_PIECES; i++) {
      for (j=0; j<ncp; j++) {
         next_contour [3*j+2] -= cap_z[j];
         last_contour [3*j+2] -= cap_z[j];
         MAT_DOT_VEC_3X3 ( (&next_contour[3*j]), m, (&last_contour[3*j]));
         next_contour [3*j+2] += cap_z[j];
         last_contour [3*j+2] += cap_z[j];
      }

      if (norms != NULL) {
         for (j=0; j<ncp; j++) {
            MAT_DOT_VEC_3X3 ( (&next_norm[3*j]), m, (&last_norm[3*j]));
         }
      }

      /* OK, now render it all */
      if (norms == NULL) {
         draw_segment_plain (ncp, (gleVector *) next_contour, 
                                  (gleVector *) last_contour, 0, 0.0);
      } else
      if (__TUBE_DRAW_FACET_NORMALS) {
         draw_binorm_segment_facet_n (ncp, 
                               (gleVector *) next_contour, 
                               (gleVector *) last_contour,
                               (gleVector *) next_norm, 
                               (gleVector *) last_norm, 0, 0.0);
      } else {
         draw_binorm_segment_edge_n (ncp,
                               (gleVector *) next_contour, 
                               (gleVector *) last_contour,
                               (gleVector *) next_norm, 
                               (gleVector *) last_norm, 0, 0.0);
     }

      /* swap contours */
      tmp = next_contour;
      next_contour = last_contour;
      last_contour = tmp;

      tmp = next_norm;
      next_norm = last_norm;
      last_norm = tmp;
   }
   /* &&&&&&&&&&&&&& end drawing cap &&&&&&&&&&&&& */

   /* Thou shalt not leak memory */
   free (malloced_area);
}

/* ==================== END OF FILE =========================== */
