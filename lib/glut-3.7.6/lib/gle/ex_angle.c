
/*
 * MODULE NAME: ex_angle.c
 *
 * FUNCTION: 
 * This module contains code that draws extrusions with angled
 * joins ("angle join style").  It also inserts colors and normals 
 * where necessary, if appropriate.
 *
 * HISTORY:
 * written by Linas Vepstas August/September 1991
 * split into multiple compile units, Linas, October 1991
 * added normal vectors Linas, October 1991
 * "code complete" (that is, I'm done), Linas Vepstas, October 1991
 * work around OpenGL's lack of support for concave polys, June 1994
 */

#include <stdlib.h>
#include <math.h>
#include <string.h>	/* for the memcpy() subroutine */
#include <GL/tube.h>
#include "port.h"
#include "gutil.h"
#include "vvector.h"
#include "tube_gc.h"
#include "extrude.h"
#include "intersect.h"
#include "segment.h"

/* ============================================================ */
/*
 * Algorithmic trivia:
 * 
 * There is a slight bit of trivia which the super-duper exacto coder
 * needs to know about the code in this module. It is this:
 *
 * This module attempts to correctly treat contour normal vectors 
 * by applying the inverse transpose of the 2D contour affine
 * transformation to the 2D contour normals.  This is perfectly correct,
 * when applied to the "raw" join style.  However, if the affine transform
 * has a strong rotational component, AND the join style is angle or
 * cut, then the normal vectors would continue to rotate as the
 * intersect point is extrapolated. 
 * 
 * The extrapolation of the inverse-transpose matrix to the intersection
 * point is not done.  This would appear to be overkill for most
 * situations.  The viewer might possibly detect an artifact of the 
 * failure to do this correction IF all three of the following criteria 
 * were met:
 * 1) The affine xform has a strong rotational component,
 * 2) The angle between two succesive segments is sharp (greater than 15 or
 *    30 degrees).
 * 3) The join style is angle or cut.
 *
 * However, I beleive that it is highly unlikely that the viewer will
 * detect any artifacts. The reason I beleive this is that a strong
 * rotational component will twist a segment so strongly that the more
 * visible artifact will be that a segment is composed of triangle strips.
 * As the user attempts to minimize the tesselation artifacts by shortening
 * segments, then the rotational component will decrease in proportion,
 * and the lighting artifact will fall away.
 *
 * To summarize, there is a slight inexactness in this code.  The author
 * of the code beleives that this inexactness results in miniscule
 * errors in every situation.
 *
 * Linas Vepstas March 1993
 */

/* ============================================================ */

void draw_angle_style_front_cap (int ncp,	/* number of contour points */
                           gleDouble bi[3],		/* biscetor */
                           gleDouble point_array[][3])	/* polyline */
{
   int j;
#ifdef OPENGL_10
   GLUtriangulatorObj *tobj;
#endif /* OPENGL_10 */

   if (bi[2] < 0.0) {
      VEC_SCALE (bi, -1.0, bi); 
   }

#ifdef GL_32
   /* old-style gl handles concave polygons no problem, so the code is
    * simple.  New-style gl is a lot more tricky. */
   /* draw the end cap */
   BGNPOLYGON ();

   N3F (bi);
   for (j=0; j<ncp; j++) {
      V3F (point_array[j], j, FRONT_CAP);
   }
   ENDPOLYGON ();
#endif /* GL_32 */

#ifdef OPENGL_10
   N3F(bi);

   tobj = gluNewTess ();
   gluTessCallback (tobj, GLU_BEGIN, glBegin);
   gluTessCallback (tobj, GLU_VERTEX, glVertex3dv);
   gluTessCallback (tobj, GLU_END, glEnd);
   gluBeginPolygon (tobj);

   for (j=0; j<ncp; j++) {
      gluTessVertex (tobj, point_array[j], point_array[j]);
   }
   gluEndPolygon (tobj);
   gluDeleteTess (tobj);
#endif /* OPENGL_10 */
}

/* ============================================================ */

void draw_angle_style_back_cap (int ncp,	/* number of contour points */
                           gleDouble bi[3],		/* biscetor */
                           gleDouble point_array[][3])	/* polyline */
{
   int j;
#ifdef OPENGL_10
   GLUtriangulatorObj *tobj;
#endif /* OPENGL_10 */

   if (bi[2] > 0.0) {
      VEC_SCALE (bi, -1.0, bi); 
   }

#ifdef GL_32
   /* old-style gl handles concave polygons no problem, so the code is
    * simple.  New-style gl is a lot more tricky. */
   /* draw the end cap */
   BGNPOLYGON ();

   N3F (bi);
   for (j=ncp-1; j>=0; j--) {
      V3F (point_array[j], j, BACK_CAP);
   }
   ENDPOLYGON ();
#endif /* GL_32 */

#ifdef OPENGL_10
   N3F (bi);

   tobj = gluNewTess ();
   gluTessCallback (tobj, GLU_BEGIN, glBegin);
   gluTessCallback (tobj, GLU_VERTEX, glVertex3dv);
   gluTessCallback (tobj, GLU_END, glEnd);
   gluBeginPolygon (tobj);

   for (j=ncp-1; j>=0; j--) {
      gluTessVertex (tobj, point_array[j], point_array[j]);
   }
   gluEndPolygon (tobj);
   gluDeleteTess (tobj);
#endif /* OPENGL_10 */
}

/* ============================================================ */

void extrusion_angle_join (int ncp,		/* number of contour points */
                           gleDouble contour[][2],	/* 2D contour */
                           gleDouble cont_normal[][2], /* 2D normal vecs */
                           gleDouble up[3],	/* up vector for contour */
                           int npoints,		/* numpoints in poly-line */
                           gleDouble point_array[][3],	/* polyline */
                           float color_array[][3],	/* color of polyline */
                           gleDouble xform_array[][2][3])  /* 2D contour xforms */
{
   int i, j;
   int inext, inextnext;
   gleDouble m[4][4];
   gleDouble len;
   gleDouble len_seg;
   gleDouble diff[3];
   gleDouble bi_0[3], bi_1[3];		/* bisecting plane */
   gleDouble bisector_0[3], bisector_1[3];	/* bisecting plane */
   gleDouble end_point_0[3], end_point_1[3]; 
   gleDouble origin[3], neg_z[3];
   gleDouble yup[3];		/* alternate up vector */
   gleDouble *front_loop, *back_loop;   /* contours in 3D */
   char * mem_anchor;
   double *norm_loop; 
   double *front_norm, *back_norm, *tmp; /* contour normals in 3D */
   int first_time;

   /* By definition, the contour passed in has its up vector pointing in
    * the y direction */
   if (up == NULL) {
      yup[0] = 0.0;
      yup[1] = 1.0;
      yup[2] = 0.0;
   } else {
      VEC_COPY(yup, up);
   }

   /* ========== "up" vector sanity check ========== */
   (void) up_sanity_check (yup, npoints, point_array);

   /* the origin is at the origin */
   origin [0] = 0.0;
   origin [1] = 0.0;
   origin [2] = 0.0;

   /* and neg_z is at neg z */
   neg_z[0] = 0.0;
   neg_z[1] = 0.0;
   neg_z[2] = 1.0;

   /* ignore all segments of zero length */
   i = 1;
   inext = i;
   FIND_NON_DEGENERATE_POINT (inext, npoints, len, diff, point_array);
   len_seg = len;	/* store for later use */

   /* get the bisecting plane */
   bisecting_plane (bi_0, point_array[0], 
                          point_array[1], 
                          point_array[inext]);
   /* reflect the up vector in the bisecting plane */
   VEC_REFLECT (yup, yup, bi_0);

   /* malloc the storage we'll need for relaying changed contours to the
    * drawing routines. */
   mem_anchor =  malloc (2 * 3 * ncp * sizeof(double)
                      +  2 * 3 * ncp * sizeof(gleDouble));
   front_loop = (gleDouble *) mem_anchor;
   back_loop = front_loop + 3 * ncp;
   front_norm = (double *) (back_loop + 3 * ncp);
   back_norm = front_norm + 3 * ncp;
   norm_loop = front_norm;

   /* may as well get the normals set up now */
   if (cont_normal != NULL) {
      if (xform_array == NULL) {
         for (j=0; j<ncp; j++) {
            norm_loop[3*j] = cont_normal[j][0];
            norm_loop[3*j+1] = cont_normal[j][1];
            norm_loop[3*j+2] = 0.0;
         }
      } else {
         for (j=0; j<ncp; j++) {
            NORM_XFORM_2X2 ( (&front_norm[3*j]),
                              xform_array[inext-1],
                              cont_normal [j]);
            front_norm[3*j+2] = 0.0;
            back_norm[3*j+2] = 0.0;
         }
      }
   }

   first_time = TRUE;
   /* draw tubing, not doing the first segment */
   while (inext<npoints-1) {

      inextnext = inext;
      /* ignore all segments of zero length */
      FIND_NON_DEGENERATE_POINT (inextnext, npoints, len, diff, point_array);

      /* get the next bisecting plane */
      bisecting_plane (bi_1, point_array[i], 
                             point_array[inext], 
                             point_array[inextnext]);  

      /* rotate so that z-axis points down v2-v1 axis, 
       * and so that origen is at v1 */
      uviewpoint (m, point_array[i], point_array[inext], yup);
      PUSHMATRIX ();
      MULTMATRIX (m);

      /* rotate the bisecting planes into the local coordinate system */
      MAT_DOT_VEC_3X3 (bisector_0, m, bi_0);
      MAT_DOT_VEC_3X3 (bisector_1, m, bi_1);

      neg_z[2] = -len_seg;

      /* draw the tube */
      /* --------- START OF TMESH GENERATION -------------- */
      for (j=0; j<ncp; j++) {

         /* if there are normals, and there are either affine xforms, OR
          * path-edge normals need to be drawn, then compute local
          * coordinate system normals. 
          */
         if (cont_normal != NULL) {
          
            /* set up the back normals. (The front normals we inherit
             * from previous pass through the loop) */
            if (xform_array != NULL) {
               /* do up the normal vectors with the inverse transpose */
               NORM_XFORM_2X2 ( (&back_norm[3*j]),
                                 xform_array[inext],
                                 cont_normal [j]);
            }

            /* Note that if the xform array is NULL, then normals are
             * constant, and are set up outside of the loop.
             */

            /* 
             * if there are normal vectors, and the style calls for it, 
             * then we want to project the normal vectors into the
             * bisecting plane. (This style is needed to make toroids, etc. 
             * look good: Without this, segmentation artifacts show up
             * under lighting.
             */
            if (__TUBE_DRAW_PATH_EDGE_NORMALS) {
               /* Hmm, if no affine xforms, then we haven't yet set
                * back vector. So do it. */
               if (xform_array == NULL) {
                  back_norm[3*j] = cont_normal[j][0];
                  back_norm[3*j+1] = cont_normal[j][1];
               }
   
               /* now, start with a fresh normal (z component equal to
                * zero), project onto bisecting plane (by computing 
                * perpendicular componenet to bisect vector, and renormalize 
                * (since projected vector is not of unit length */ 
               front_norm[3*j+2] = 0.0;
               VEC_PERP ((&front_norm[3*j]), (&front_norm[3*j]), bisector_0);
               VEC_NORMALIZE ((&front_norm[3*j]));
    
               back_norm[3*j+2] = 0.0;
               VEC_PERP ((&back_norm[3*j]), (&back_norm[3*j]), bisector_1);
               VEC_NORMALIZE ((&back_norm[3*j]));
            } 
         } 

         /* Next, we want to define segements. We find the endpoints of
          * the segments by intersecting the contour with the bisecting
          * plane.  If there is no local affine transform, this is easy.
          *
          * If there is an affine tranform, then we want to remove the
          * torsional component, so that the intersection points won't
          * get twisted out of shape.  We do this by applying the
          * local affine transform to the entire coordinate system.
          */
         if (xform_array == NULL) {
            end_point_0 [0] = contour[j][0];
            end_point_0 [1] = contour[j][1];
   
            end_point_1 [0] = contour[j][0];
            end_point_1 [1] = contour[j][1];
         } else {
            /* transform the contour points with the local xform */
            MAT_DOT_VEC_2X3 (end_point_0,
                             xform_array[inext-1], contour[j]);
            MAT_DOT_VEC_2X3 (end_point_1,
                             xform_array[inext-1], contour[j]);
         }

         end_point_0 [2] = 0.0;
         end_point_1 [2] = - len_seg;

         /* The two end-points define a line.  Intersect this line
          * against the clipping plane defined by the PREVIOUS
          * tube segment.  */

         INNERSECT ((&front_loop[3*j]), /* intersection point (returned) */
                    origin,		/* point on intersecting plane */
                    bisector_0,		/* normal vector to plane */
                    end_point_0,	/* point on line */
                    end_point_1);	/* another point on the line */	

         /* The two end-points define a line.  Intersect this line
          * against the clipping plane defined by the NEXT
          * tube segment.  */

         /* if there's an affine coordinate change, be sure to use it */
         if (xform_array != NULL) {
            /* transform the contour points with the local xform */
            MAT_DOT_VEC_2X3 (end_point_0,
                             xform_array[inext], contour[j]);
            MAT_DOT_VEC_2X3 (end_point_1,
                             xform_array[inext], contour[j]);
         }

         INNERSECT ((&back_loop[3*j]),	/* intersection point (returned) */
                    neg_z,		/* point on intersecting plane */
                    bisector_1,		/* normal vector to plane */
                    end_point_0,	/* point on line */
                    end_point_1);	/* another point on the line */	

      }

      /* --------- END OF TMESH GENERATION -------------- */

      /* v^v^v^v^v^v^v^v^v  BEGIN END CAPS v^v^v^v^v^v^v^v^v^v^v^v */

      /* if end caps are required, draw them. But don't draw any 
       * but the very first and last caps */
      if (__TUBE_DRAW_CAP) {
         if (first_time) {
            if (color_array != NULL) C3F (color_array[inext-1]);
            first_time = FALSE;
            draw_angle_style_front_cap (ncp, bisector_0, (gleVector *) front_loop);
         }
         if (inext == npoints-2) {
            if (color_array != NULL) C3F (color_array[inext]);
            draw_angle_style_back_cap (ncp, bisector_1, (gleVector *) back_loop);
         }
      }
      /* v^v^v^v^v^v^v^v^v  END END CAPS v^v^v^v^v^v^v^v^v^v^v^v */

      /* |||||||||||||||||| START SEGMENT DRAW |||||||||||||||||||| */
      /* There are six different cases we can have for presence and/or
       * absecnce of colors and normals, and for interpretation of
       * normals. The blechy set of nested if statements below
       * branch to each of the six cases */
      if ((xform_array == NULL) && (!__TUBE_DRAW_PATH_EDGE_NORMALS)) {
         if (color_array == NULL) {
            if (cont_normal == NULL) {
               draw_segment_plain (ncp, (gleVector *) front_loop,
                                        (gleVector *) back_loop, inext, len_seg);
            } else
            if (__TUBE_DRAW_FACET_NORMALS) {
               draw_segment_facet_n (ncp, (gleVector *) front_loop,
                                          (gleVector *) back_loop, 
                                          (gleVector *) norm_loop, inext, len_seg);
            } else {
               draw_segment_edge_n (ncp, (gleVector *) front_loop, 
                                         (gleVector *) back_loop, 
                                         (gleVector *) norm_loop, inext, len_seg);
            }
         } else {
            if (cont_normal == NULL) {
               draw_segment_color (ncp, (gleVector *) front_loop, 
                                        (gleVector *) back_loop, 
                                   color_array[inext-1],
                                   color_array[inext], inext, len_seg);
            } else
            if (__TUBE_DRAW_FACET_NORMALS) {
               draw_segment_c_and_facet_n (ncp, 
                                   (gleVector *) front_loop, 
                                   (gleVector *) back_loop, 
                                   (gleVector *) norm_loop,
                                   color_array[inext-1],
                                   color_array[inext], inext, len_seg);
            } else {
               draw_segment_c_and_edge_n (ncp, 
                                   (gleVector *) front_loop, 
                                   (gleVector *) back_loop, 
                                   (gleVector *) norm_loop,
                                   color_array[inext-1],
                                   color_array[inext], inext, len_seg);
             }
          }
      } else {
         if (color_array == NULL) {
            if (cont_normal == NULL) {
               draw_segment_plain (ncp, (gleVector *) front_loop, 
                                        (gleVector *)  back_loop, inext, len_seg);
            } else 
            if (__TUBE_DRAW_FACET_NORMALS) {
               draw_binorm_segment_facet_n (ncp, (gleVector *) front_loop, 
                                                 (gleVector *) back_loop,
                                                 (gleVector *) front_norm, 
                                                 (gleVector *) back_norm,
                                                 inext, len_seg);
            } else {
               draw_binorm_segment_edge_n (ncp, (gleVector *) front_loop, 
                                                (gleVector *) back_loop,
                                                (gleVector *) front_norm,
                                                (gleVector *) back_norm,
                                                inext, len_seg);
            }
         } else {
            if (cont_normal == NULL) {
               draw_segment_color (ncp, (gleVector *) front_loop, 
                                        (gleVector *) back_loop, 
                                   color_array[inext-1],
                                   color_array[inext], inext, len_seg);
            } else
            if (__TUBE_DRAW_FACET_NORMALS) {
               draw_binorm_segment_c_and_facet_n (ncp, 
                                   (gleVector *) front_loop, 
                                   (gleVector *) back_loop, 
                                   (gleVector *) front_norm, 
                                   (gleVector *) back_norm, 
                                   color_array[inext-1],
                                   color_array[inext], inext, len_seg);
            } else {
               draw_binorm_segment_c_and_edge_n (ncp, 
                                   (gleVector *) front_loop, 
                                   (gleVector *) back_loop, 
                                   (gleVector *) front_norm, 
                                   (gleVector *) back_norm, 
                                   color_array[inext-1],
                                   color_array[inext], inext, len_seg);
             }
          }
      }
      /* |||||||||||||||||| END SEGMENT DRAW |||||||||||||||||||| */

      /* pop this matrix, do the next set */
      POPMATRIX ();

      /* bump everything to the next vertex */
      len_seg = len;
      i = inext;
      inext = inextnext;
      VEC_COPY (bi_0, bi_1);

      /* trade norm loops */
      tmp = front_norm;
      front_norm = back_norm;
      back_norm = tmp;

      /* reflect the up vector in the bisecting plane */
      VEC_REFLECT (yup, yup, bi_0);
   }

   /* be sure to free it all up */
   free (mem_anchor);

}
   
/* ============================================================ */
