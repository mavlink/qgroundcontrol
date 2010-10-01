/*
 * MODULE NAME: ex_raw.c
 *
 * FUNCTION: 
 * This module contains code that draws extrusions with square
 * ("raw join style") end styles.  It also inserts colors and normals 
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
#include <stdio.h>      /* to get stderr defined */
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
 * The following routine is, in principle, very simple:
 * all that it does is normalize the up vector, and makes
 * sure that it is perpendicular to the initial polyline segment.
 *
 * In fact, this routine gets awfully complicated because:
 * a) the first few segements of the polyline might be degenerate, 
 * b) up vecotr may be parallel to first few segments of polyline,
 * c) etc.
 *
 */

void up_sanity_check (gleDouble up[3],	/* up vector for contour */
                      int npoints,		/* numpoints in poly-line */
                      gleDouble point_array[][3])	/* polyline */
{
   int i;
   double len;
   double diff[3];

   /* now, right off the bat, we should make sure that the up vector 
    * is in fact perpendicular to the polyline direction */
   VEC_DIFF (diff, point_array[1], point_array[0]);
   VEC_LENGTH (len, diff);
   if (len == 0.0) {
      /* This error message should go through "official" error interface */
/*
      fprintf (stderr, "Extrusion: Warning: initial segment zero length \n");
*/

      /* loop till we find something that ain't of zero length */
      for (i=1; i<npoints-2; i++) {
         VEC_DIFF (diff, point_array[i+1], point_array[i]);
         VEC_LENGTH (len, diff);
         if (len != 0.0) break;
      }
   }

   /* normalize diff to be of unit length */
   len = 1.0 / len;
   VEC_SCALE (diff, len, diff);

   /* we want to take only perpendicular component of up w.r.t. the
    * initial segment */
   VEC_PERP (up, up, diff);

   VEC_LENGTH (len, up);
   if (len == 0.0) {
      /* This error message should go through "official" error interface */
      fprintf (stderr, "Extrusion: Warning: ");
      fprintf (stderr, "contour up vector parallel to tubing direction \n");

      /* do like the error message says ... */
      VEC_COPY (up, diff);
   }

}

/* ============================================================ */
/* 
 * This routine does what it says: It draws the end-caps for the
 * "raw" join style.
 */

void draw_raw_style_end_cap (int ncp,		/* number of contour points */
                             gleDouble contour[][2],	/* 2D contour */
                             gleDouble zval,		/* where to draw cap */
                             int frontwards)	/* front or back cap */
{
   int j;

#ifdef OPENGL_10
   GLUtriangulatorObj *tobj;
   double *pts;
#endif /* OPENGL_10 */


#ifdef GL_32
   /* old-style gl handles concave polygons no problem, so the code is
    * simple.  New-style gl is a lot more tricky. */
   point [2] = zval;
   BGNPOLYGON ();
      /* draw the loop counter clockwise for the front cap */
      if (frontwards) {
         for (j=0; j<ncp; j++) {
            point [0] = contour[j][0];
            point [1] = contour[j][1];
            V3F (point, j, FRONT_CAP);
         }

      } else {
         /* the sense of the loop is reversed for backfacing culling */
         for (j=ncp-1; j>-1; j--) {
            point [0] = contour[j][0];
            point [1] = contour[j][1];
            V3F (point, j, BACK_CAP);
         }
      }
   ENDPOLYGON ();
#endif /* GL_32 */

#ifdef OPENGL_10
   /* malloc the @#$%^&* array that OpenGL wants ! */
   pts = (double *) malloc (3*ncp*sizeof(double));
   tobj = gluNewTess ();
   gluTessCallback (tobj, GLU_BEGIN, glBegin);
   gluTessCallback (tobj, GLU_VERTEX, glVertex3dv);
   gluTessCallback (tobj, GLU_END, glEnd);
   gluBeginPolygon (tobj);

      /* draw the loop counter clockwise for the front cap */
      if (frontwards) {
         for (j=1; j<ncp; j++) {
            pts [3*j] = contour[j][0];
            pts [3*j+1] = contour[j][1];
            pts [3*j+2] = zval;
            gluTessVertex (tobj, &pts[3*j], &pts[3*j]);
         }

      } else {
         /* the sense of the loop is reversed for backfacing culling */
         for (j=ncp-2; j>-1; j--) {
            pts [3*j] = contour[j][0];
            pts [3*j+1] = contour[j][1];
            pts [3*j+2] = zval;
            gluTessVertex (tobj, &pts[3*j], &pts[3*j]);
         }
      }

   gluEndPolygon (tobj);
   free (pts);
   gluDeleteTess (tobj);
#endif /* OPENGL_10 */
}


/* ============================================================ */
/* This routine does what it says: It draws a counter-clockwise cap 
 * from a 3D contour loop list
 */

void draw_front_contour_cap (int ncp,	/* number of contour points */
                             gleDouble contour[][3])	/* 3D contour */
{
   int j;
#ifdef OPENGL_10
   GLUtriangulatorObj *tobj;
#endif /* OPENGL_10 */

#ifdef GL_32
   /* old-style gl handles concave polygons no problem, so the code is
    * simple.  New-style gl is a lot more tricky. */
   /* draw the end cap */
   BGNPOLYGON ();

   for (j=0; j<ncp; j++) {
      V3F (contour[j], j, FRONT_CAP);
   }
   ENDPOLYGON ();
#endif /* GL_32 */

#ifdef OPENGL_10
   tobj = gluNewTess ();
   gluTessCallback (tobj, GLU_BEGIN, glBegin);
   gluTessCallback (tobj, GLU_VERTEX, glVertex3dv);
   gluTessCallback (tobj, GLU_END, glEnd);
   gluBeginPolygon (tobj);

   for (j=0; j<ncp; j++) {
      gluTessVertex (tobj, contour[j], contour[j]);
   }
   gluEndPolygon (tobj);
   gluDeleteTess (tobj);
#endif /* OPENGL_10 */
}


/* ============================================================ */
/* This routine does what it says: It draws a clockwise cap 
 * from a 3D contour loop list
 */

void draw_back_contour_cap (int ncp,	/* number of contour points */
                             gleDouble contour[][3])	/* 3D contour */
{
   int j;
#ifdef OPENGL_10
   GLUtriangulatorObj *tobj;
#endif /* OPENGL_10 */

#ifdef GL_32
   /* old-style gl handles concave polygons no problem, so the code is
    * simple.  New-style gl is a lot more tricky. */

   /* draw the end cap */
   /* draw the loop clockwise for the back cap */
   /* the sense of the loop is reversed for backfacing culling */
   BGNPOLYGON ();

   for (j=ncp-1; j>-1; j--) {
      V3F (contour[j], j, BACK_CAP);
   }
   ENDPOLYGON ();
#endif /* GL_32 */

#ifdef OPENGL_10
   tobj = gluNewTess ();
   gluTessCallback (tobj, GLU_BEGIN, glBegin);
   gluTessCallback (tobj, GLU_VERTEX, glVertex3dv);
   gluTessCallback (tobj, GLU_END, glEnd);
   gluBeginPolygon (tobj);

   /* draw the end cap */
   /* draw the loop clockwise for the back cap */
   /* the sense of the loop is reversed for backfacing culling */
   for (j=ncp-1; j>-1; j--) {
      gluTessVertex (tobj, contour[j], contour[j]);
   }
   gluEndPolygon (tobj);
   gluDeleteTess (tobj);
#endif /* OPENGL_10 */
}

/* ============================================================ */
/* This routine draws a segment of raw-join-style tubing.
 * Essentially, we assume that the proper transformations have already 
 * been performed to properly orient the tube segment -- our only task
 * left is to render */
/* PLAIN - NO COLOR, NO NORMAL */

void draw_raw_segment_plain (int ncp,		/* number of contour points */
                             gleDouble contour[][2],	/* 2D contour */
                             gleDouble len,
                             int inext)

{
   int j;
   double point[3]; 

   /* draw the tube segment */
   BGNTMESH (inext, len);
   for (j=0; j<ncp; j++) {
      point [0] = contour[j][0];
      point [1] = contour[j][1];
      point [2] = 0.0;
      V3F (point, j, FRONT);

      point [2] = - len;
      V3F (point, j, BACK);
   }

   if (__TUBE_CLOSE_CONTOUR) {
      /* connect back up to first point of contour */
      point [0] = contour[0][0];
      point [1] = contour[0][1];
      point [2] = 0.0;
      V3F (point, 0, FRONT);
   
      point [2] = - len;
      V3F (point, 0, BACK);
   }

   ENDTMESH ();

   /* draw the endcaps, if the join style calls for it */
   if (__TUBE_DRAW_CAP) {

      /* draw the front cap */
      draw_raw_style_end_cap (ncp, contour, 0.0, TRUE);

      /* draw the back cap */
      draw_raw_style_end_cap (ncp, contour, -len, FALSE);
   }

}

/* ============================================================ */
/* This routine draws a segment of raw-join-style tubing.
 * Essentially, we assume that the proper transformations have already 
 * been performed to properly orient the tube segment -- our only task
 * left is to render */
/* COLOR -- DRAW ONLY COLOR */

void draw_raw_segment_color (int ncp,		/* number of contour points */
                             gleDouble contour[][2],	/* 2D contour */
                             float color_array[][3],	/* color of polyline */
                             gleDouble len,
                             int inext)

{
   int j;
   double point[3]; 

   /* draw the tube segment */
   BGNTMESH (inext, len);
   for (j=0; j<ncp; j++) {
      point [0] = contour[j][0];
      point [1] = contour[j][1];
      point [2] = 0.0;
      C3F (color_array[inext-1]);
      V3F (point, j, FRONT);

      point [2] = - len;
      C3F (color_array[inext]);	
      V3F (point, j, BACK);
   }

   if (__TUBE_CLOSE_CONTOUR) {
      /* connect back up to first point of contour */
      point [0] = contour[0][0];
      point [1] = contour[0][1];
      point [2] = 0.0;
   
      C3F (color_array[inext-1]);
      V3F (point, 0, FRONT);
   
      point [2] = - len;
      C3F (color_array[inext]);	
      V3F (point, 0, BACK);
   }

   ENDTMESH ();

   /* draw the endcaps, if the join style calls for it */
   if (__TUBE_DRAW_CAP) {

      /* draw the front cap */
      C3F (color_array[inext-1]);	
      draw_raw_style_end_cap (ncp, contour, 0.0, TRUE);

      /* draw the back cap */
      C3F (color_array[inext]);	
      draw_raw_style_end_cap (ncp, contour, -len, FALSE);
   }

}

/* ============================================================ */
/* This routine draws a segment of raw-join-style tubing.
 * Essentially, we assume that the proper transformations have already 
 * been performed to properly orient the tube segment -- our only task
 * left is to render */
/* EDGE_N -- DRAW ONLY EDGE NORMALS */

void draw_raw_segment_edge_n (int ncp,	/* number of contour points */
                             gleDouble contour[][2],	/* 2D contour */
                             gleDouble cont_normal[][2],/* 2D normal vecs */
                             gleDouble len,
                             int inext)

{
   int j;
   double point[3]; 
   double norm[3]; 

   /* draw the tube segment */
   norm [2] = 0.0;
   BGNTMESH (inext, len);
   for (j=0; j<ncp; j++) {
      norm [0] = cont_normal[j][0];
      norm [1] = cont_normal[j][1];
      N3F (norm);

      point [0] = contour[j][0];
      point [1] = contour[j][1];
      point [2] = 0.0;
      V3F (point, j, FRONT);

      point [2] = - len;
      V3F (point, j, BACK);
   }

   if (__TUBE_CLOSE_CONTOUR) {
      /* connect back up to first point of contour */
      norm [0] = cont_normal[0][0];
      norm [1] = cont_normal[0][1];
      norm [2] = 0.0;
      N3F (norm);
   
      point [0] = contour[0][0];
      point [1] = contour[0][1];
      point [2] = 0.0;
      V3F (point, 0, FRONT);
   
      point [2] = - len;
      V3F (point, 0, BACK);
   }

   ENDTMESH ();

   /* draw the endcaps, if the join style calls for it */
   if (__TUBE_DRAW_CAP) {

      /* draw the front cap */
      norm [0] = norm [1] = 0.0;
      norm [2] = 1.0;
      N3F (norm);
      draw_raw_style_end_cap (ncp, contour, 0.0, TRUE);

      /* draw the back cap */
      norm [2] = -1.0;
      N3F (norm);
      draw_raw_style_end_cap (ncp, contour, -len, FALSE);
   }

}

/* ============================================================ */
/* This routine draws a segment of raw-join-style tubing.
 * Essentially, we assume that the proper transformations have already 
 * been performed to properly orient the tube segment -- our only task
 * left is to render */
/* C_AND_EDGE_N -- DRAW EDGE NORMALS AND COLORS */

void draw_raw_segment_c_and_edge_n (int ncp,	/* number of contour points */
                             gleDouble contour[][2],	/* 2D contour */
                             float color_array[][3],	/* color of polyline */
                             gleDouble cont_normal[][2],/* 2D normal vecs */
                             gleDouble len,
                             int inext)

{
   int j;
   double point[3]; 
   double norm[3];

   /* draw the tube segment */
   norm [2] = 0.0;
   BGNTMESH (inext, len);
   for (j=0; j<ncp; j++) {
      C3F (color_array[inext-1]);

      norm [0] = cont_normal[j][0];
      norm [1] = cont_normal[j][1];
      N3F (norm);

      point [0] = contour[j][0];
      point [1] = contour[j][1];
      point [2] = 0.0;
      V3F (point, j, FRONT);

      C3F (color_array[inext]);	
      N3F (norm);

      point [2] = - len;
      V3F (point, j, BACK);
   }

   if (__TUBE_CLOSE_CONTOUR) {
      /* connect back up to first point of contour */
      C3F (color_array[inext-1]);
   
      norm [0] = cont_normal[0][0];
      norm [1] = cont_normal[0][1];
      N3F (norm);
   
      point [0] = contour[0][0];
      point [1] = contour[0][1];
      point [2] = 0.0;
      V3F (point, 0, FRONT);
      
   
      C3F (color_array[inext]);	
      norm [0] = cont_normal[0][0];
      norm [1] = cont_normal[0][1];
      N3F (norm);
   
      point [2] = - len;
      V3F (point, 0, BACK);
   }

   ENDTMESH ();

   /* draw the endcaps, if the join style calls for it */
   if (__TUBE_DRAW_CAP) {

      /* draw the front cap */
      C3F (color_array[inext-1]);	
      norm [0] = norm [1] = 0.0;
      norm [2] = 1.0;
      N3F (norm);
      draw_raw_style_end_cap (ncp, contour, 0.0, TRUE);

      /* draw the back cap */
      C3F (color_array[inext]);	
      norm [2] = -1.0;
      N3F (norm);
      draw_raw_style_end_cap (ncp, contour, -len, FALSE);
   }

}

/* ============================================================ */
/* This routine draws a segment of raw-join-style tubing.
 * Essentially, we assume that the proper transformations have already 
 * been performed to properly orient the tube segment -- our only task
 * left is to render */
/* FACET_N -- DRAW ONLY FACET NORMALS */

void draw_raw_segment_facet_n (int ncp,	/* number of contour points */
                             gleDouble contour[][2],	/* 2D contour */
                             gleDouble cont_normal[][2],/* 2D normal vecs */
                             gleDouble len,
                             int inext)

{
   int j;
   double point[3]; 
   double norm[3]; 

   /* draw the tube segment */
   norm [2] = 0.0;
   BGNTMESH (inext, len);
   for (j=0; j<ncp-1; j++) {
      /* facet normals require one normal per four vertices */
      norm [0] = cont_normal[j][0];
      norm [1] = cont_normal[j][1];
      N3F (norm);

      point [0] = contour[j][0];
      point [1] = contour[j][1];
      point [2] = 0.0;
      V3F (point, j, FRONT);

      point [2] = - len;
      V3F (point, j, BACK);

      point [0] = contour[j+1][0];
      point [1] = contour[j+1][1];
      point [2] = 0.0;
      V3F (point, j+1, FRONT);

      point [2] = - len;
      V3F (point, j+1, BACK);
   }

   if (__TUBE_CLOSE_CONTOUR) {
      /* connect back up to first point of contour */
      norm [0] = cont_normal[ncp-1][0];
      norm [1] = cont_normal[ncp-1][1];
      N3F (norm);
   
      point [0] = contour[ncp-1][0];
      point [1] = contour[ncp-1][1];
      point [2] = 0.0;
      V3F (point, ncp-1, FRONT);
   
      point [2] = - len;
      V3F (point, ncp-1, BACK);
   
      point [0] = contour[0][0];
      point [1] = contour[0][1];
      point [2] = 0.0;
      V3F (point, 0, FRONT);
   
      point [2] = - len;
      V3F (point, 0, BACK);
   }

   ENDTMESH ();

   /* draw the endcaps, if the join style calls for it */
   if (__TUBE_DRAW_CAP) {

      /* draw the front cap */
      norm [0] = norm [1] = 0.0;
      norm [2] = 1.0;
      N3F (norm);
      draw_raw_style_end_cap (ncp, contour, 0.0, TRUE);

      /* draw the back cap */
      norm [2] = -1.0;
      N3F (norm);
      draw_raw_style_end_cap (ncp, contour, -len, FALSE);
   }

}

/* ============================================================ */
/* This routine draws a segment of raw-join-style tubing.
 * Essentially, we assume that the proper transformations have already 
 * been performed to properly orient the tube segment -- our only task
 * left is to render */
/* C_AND_FACET_N -- DRAW FACET NORMALS AND COLORS */

void draw_raw_segment_c_and_facet_n (int ncp,	/* number of contour points */
                             gleDouble contour[][2],	/* 2D contour */
                             float color_array[][3],	/* color of polyline */
                             gleDouble cont_normal[][2],/* 2D normal vecs */
                             gleDouble len,
                             int inext)

{
   int j;
   double point[3]; 
   double norm[3]; 

   /* draw the tube segment */
   norm [2] = 0.0;
   BGNTMESH (inext, len);
   for (j=0; j<ncp-1; j++) {
      /* facet normals require one normal per four vertices;
       * However, we have to respecify normal each time at each vertex 
       * so that the lighting equation gets triggered.  (V3F does NOT
       * automatically trigger the lighting equations -- it only
       * triggers when there is a local light) */

      C3F (color_array[inext-1]);

      norm [0] = cont_normal[j][0];
      norm [1] = cont_normal[j][1];
      N3F (norm);

      point [0] = contour[j][0];
      point [1] = contour[j][1];
      point [2] = 0.0;
      V3F (point, j, FRONT);

      C3F (color_array[inext]);	
      N3F (norm);
      point [2] = - len;
      V3F (point, j, BACK);
      

      C3F (color_array[inext-1]);
      N3F (norm);

      point [0] = contour[j+1][0];
      point [1] = contour[j+1][1];
      point [2] = 0.0;
      V3F (point, j+1, FRONT);

      C3F (color_array[inext]);	
      N3F (norm);
      point [2] = - len;
      V3F (point, j+1, BACK);
   }

   if (__TUBE_CLOSE_CONTOUR) {
      /* connect back up to first point of contour */
      point [0] = contour[ncp-1][0];
      point [1] = contour[ncp-1][1];
      point [2] = 0.0;
      C3F (color_array[inext-1]);
   
      norm [0] = cont_normal[ncp-1][0];
      norm [1] = cont_normal[ncp-1][1];
      N3F (norm);
      V3F (point, ncp-1, FRONT);
   
      C3F (color_array[inext]);	
      N3F (norm);
   
      point [2] = - len;
      V3F (point, ncp-1, BACK);
   
      C3F (color_array[inext-1]);
   
      norm [0] = cont_normal[0][0];
      norm [1] = cont_normal[0][1];
      N3F (norm);
   
      point [0] = contour[0][0];
      point [1] = contour[0][1];
      point [2] = 0.0;
      V3F (point, 0, FRONT);
   
      C3F (color_array[inext]);	
      N3F (norm);
   
      point [2] = - len;
      V3F (point, 0, BACK);
   }
   
   ENDTMESH ();

   /* draw the endcaps, if the join style calls for it */
   if (__TUBE_DRAW_CAP) {

      /* draw the front cap */
      C3F (color_array[inext-1]);	
      norm [0] = norm [1] = 0.0;
      norm [2] = 1.0;
      N3F (norm);
      draw_raw_style_end_cap (ncp, contour, 0.0, TRUE);

      /* draw the back cap */
      C3F (color_array[inext]);	
      norm [2] = -1.0;
      N3F (norm);
      draw_raw_style_end_cap (ncp, contour, -len, FALSE);
   }

}

/* ============================================================ */
/* This routine draws "raw" style extrusions.  By "raw" style, it is
 * meant extrusions with square ends: ends that are cut at 90 degrees to
 * the length of the extrusion.  End caps are NOT drawn, unless the end cap
 * style is indicated.
 */

void extrusion_raw_join (int ncp,		/* number of contour points */
                         gleDouble contour[][2],	/* 2D contour */
                         gleDouble cont_normal[][2],/* 2D contour normal vecs */
                         gleDouble up[3],	/* up vector for contour */
                         int npoints,		/* numpoints in poly-line */
                         gleDouble point_array[][3],	/* polyline */
                         float color_array[][3],	/* color of polyline */
                         gleDouble xform_array[][2][3])   /* 2D contour xforms */
{
   int i, j;
   int inext;
   gleDouble m[4][4];
   gleDouble len;
   gleDouble diff[3];
   gleDouble bi_0[3];		/* bisecting plane */
   gleDouble yup[3];		/* alternate up vector */
   gleDouble nrmv[3];
   short no_norm, no_cols, no_xform;     /*booleans */
   char *mem_anchor;
   gleDouble *front_loop, *back_loop;  /* countour loops */
   gleDouble *front_norm, *back_norm;  /* countour loops */
   gleDouble *tmp;
   
   nrmv[0] = nrmv[1] = 0.0;   /* used for drawing end caps */
   /* use some local variables for needed booleans */
   no_norm = (cont_normal == NULL);
   no_cols = (color_array == NULL);
   no_xform = (xform_array == NULL);

   /* alloc loop arrays if needed */
   if (! no_xform) {
      mem_anchor = malloc (4 * ncp * 3 * sizeof(gleDouble));
      front_loop = (gleDouble *) mem_anchor;
      back_loop = front_loop + 3*ncp;
      front_norm = back_loop + 3*ncp;
      back_norm = front_norm + 3*ncp;
   }

   /* By definition, the contour passed in has its up vector pointing in
    * the y direction */
   if (up == NULL) {
      yup[0] = 0.0;
      yup[1] = 1.0;
      yup[2] = 0.0;
   } else {
      VEC_COPY (yup, up);
   }

   /* ========== "up" vector sanity check ========== */
   (void) up_sanity_check (yup, npoints, point_array);

   /* ignore all segments of zero length */
   i = 1;
   inext = i;
   FIND_NON_DEGENERATE_POINT (inext, npoints, len, diff, point_array);

   /* first time through, get the loops */
   if (! no_xform) {
      for (j=0; j<ncp; j++) {
            MAT_DOT_VEC_2X3 ((&front_loop[3*j]), 
                             xform_array[inext-1], contour[j]);
            front_loop[3*j+2] = 0.0;
      }
      if (!no_norm) {
         for (j=0; j<ncp; j++) {
            NORM_XFORM_2X2 ( (&front_norm[3*j]),
                             xform_array[inext-1],
                             cont_normal [j]);
            front_norm[3*j+2] = 0.0;
            back_norm[3*j+2] = 0.0;
         }
      }
   }

   /* draw tubing, not doing the first segment */
   while (inext<npoints-1) {

      /* get the two bisecting planes */
      bisecting_plane (bi_0, point_array[i-1], 
                             point_array[i], 
                             point_array[inext]);

      /* reflect the up vector in the bisecting plane */
      VEC_REFLECT (yup, yup, bi_0);

      /* rotate so that z-axis points down v2-v1 axis, 
       * and so that origen is at v1 */
      uviewpoint (m, point_array[i], point_array[inext], yup);
      PUSHMATRIX ();
      MULTMATRIX (m);

      /* There are six different cases we can have for presence and/or
       * absecnce of colors and normals, and for interpretation of
       * normals. The blechy set of nested if statements below
       * branch to each of the six cases */
      if (no_xform) {
         if (no_cols) {
            if (no_norm) {
               draw_raw_segment_plain (ncp, contour, len, inext);
            } else 
            if (__TUBE_DRAW_FACET_NORMALS) {
               draw_raw_segment_facet_n (ncp, contour, cont_normal,
                                               len, inext);
            } else {
               draw_raw_segment_edge_n (ncp, contour, cont_normal,
                                               len, inext);
            }
         } else {
            if (no_norm) {
               draw_raw_segment_color (ncp, contour, color_array, len, inext);
            } else 
            if (__TUBE_DRAW_FACET_NORMALS) {
               draw_raw_segment_c_and_facet_n (ncp, contour, 
                                               color_array,
                                               cont_normal,
                                               len, inext);
            } else {
               draw_raw_segment_c_and_edge_n (ncp, contour, 
                                              color_array,
                                              cont_normal,
                                              len, inext);
             }
          }
      } else {

         /* else -- there are scales and offsets to deal with */
         for (j=0; j<ncp; j++) {
            MAT_DOT_VEC_2X3 ((&back_loop[3*j]), 
                             xform_array[inext], contour[j]);
            back_loop[3*j+2] = -len;
            front_loop[3*j+2] = 0.0;
         }

         if (!no_norm) {
            for (j=0; j<ncp; j++) {
               NORM_XFORM_2X2 ( (&back_norm[3*j]),
                                             xform_array[inext],
                                             cont_normal [j]);
            }
         }

         if (no_cols) {
            if (no_norm) {
               draw_segment_plain (ncp,
	         (gleDouble (*)[3]) front_loop, (gleDouble (*)[3]) back_loop, inext, len);
            } else
            if (__TUBE_DRAW_FACET_NORMALS) {
               draw_binorm_segment_facet_n (ncp, (gleDouble (*)[3]) front_loop, (gleDouble (*)[3]) back_loop, 
                                                 (gleDouble (*)[3]) front_norm, (gleDouble (*)[3]) back_norm, inext, len);
            } else {
               draw_binorm_segment_edge_n (ncp, (gleDouble (*)[3]) front_loop, (gleDouble (*)[3]) back_loop, 
                                                (gleDouble (*)[3]) front_norm, (gleDouble (*)[3]) back_norm, inext, len);
            }
            if (__TUBE_DRAW_CAP) {
                nrmv[2] = 1.0; N3F (nrmv);
                draw_front_contour_cap (ncp, (gleVector *) front_loop);
                nrmv[2] = -1.0; N3F (nrmv);
                draw_back_contour_cap (ncp, (gleVector *) back_loop);
            }
         } else {
            if (no_norm) {
               draw_segment_color (ncp, (gleDouble (*)[3]) front_loop, (gleDouble (*)[3]) back_loop,
                                   color_array[inext-1],
                                   color_array[inext], inext, len);
            } else
            if (__TUBE_DRAW_FACET_NORMALS) {
               draw_binorm_segment_c_and_facet_n (ncp,
                                   (gleDouble (*)[3]) front_loop, (gleDouble (*)[3]) back_loop,
                                   (gleDouble (*)[3]) front_norm, (gleDouble (*)[3]) back_norm,
                                   color_array[inext-1],
                                   color_array[inext], inext, len);
            } else {
               draw_binorm_segment_c_and_edge_n (ncp,
                                   (gleDouble (*)[3]) front_loop, (gleDouble (*)[3]) back_loop,
                                   (gleDouble (*)[3]) front_norm, (gleDouble (*)[3]) back_norm,
                                   color_array[inext-1],
                                   color_array[inext],
				   inext, len);
            }
            if (__TUBE_DRAW_CAP) {
                C3F (color_array[inext-1]);
                nrmv[2] = 1.0; N3F (nrmv);
                draw_front_contour_cap (ncp, (gleVector *) front_loop);

                C3F (color_array[inext]);
                nrmv[2] = -1.0; N3F (nrmv);
                draw_back_contour_cap (ncp, (gleVector *) back_loop);
            }
         }
      }

      /* pop this matrix, do the next set */
      POPMATRIX ();

      /* flop over transformed loops */
      tmp = front_loop;
      front_loop = back_loop;
      back_loop = tmp;
      tmp = front_norm;
      front_norm = back_norm;
      back_norm = tmp;

      i = inext;
      /* ignore all segments of zero length */
      FIND_NON_DEGENERATE_POINT (inext, npoints, len, diff, point_array);

   }

   /* free previously allocated memory, if any */
   if (!no_xform) {
      free (mem_anchor);
   }
}
   
/* ============================================================ */
