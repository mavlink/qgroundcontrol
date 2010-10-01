
/*
 * MODULE NAME: ex_cut_round.c
 *
 * FUNCTION:
 * This module contains code that draws extrusions with cut or round
 * join styles. The cut join style is a beveled edge.
 * The code also inserts colors and normals if appropriate.
 *
 * HISTORY:
 * written by Linas Vepstas August/September 1991
 * split into multiple compile units, Linas, October 1991
 * added normal vectors Linas, October 1991
 * Fixed filleting problem, Linas February 1993
 * Modified to handle round joins as well (based on common code),
 *                           Linas, March 1993
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


#ifdef NONCONCAVE_CAPS

/* ============================================================ */
/* 
 * This subroutine draws a flat cap, to close off the cut ends 
 * of the cut-style join.  Because OpenGL doe not natively handle 
 * concave polygons, this will cause some artifacts to appear on the
 * screen.
 */

void draw_cut_style_cap_callback (int iloop,
                                  double cap[][3], 
                                  float face_color[3],
                                  gleDouble cut_vector[3],
                                  gleDouble bisect_vector[3],
                                  double norms[][3], 
                                  int frontwards)
{
   int i;

   if (face_color != NULL) C3F (face_color);

   if (frontwards) {

      /* if lighting is on, specify the endcap normal */
      if (cut_vector != NULL) {
         /* if normal pointing in wrong direction, flip it. */
         if (cut_vector[2] < 0.0) { 
            VEC_SCALE (cut_vector, -1.0, cut_vector); 
         }
         N3F_D (cut_vector);
      }
      BGNPOLYGON();
      for (i=0; i<iloop; i++) {
         V3F_D (cap[i], i, FRONT_CAP);
      }
      ENDPOLYGON();
   } else {

      /* if lighting is on, specify the endcap normal */
      if (cut_vector != NULL) {
         /* if normal pointing in wrong direction, flip it. */
         if (cut_vector[2] > 0.0) 
           { VEC_SCALE (cut_vector, -1.0, cut_vector); }
         N3F_D (cut_vector);
      }
      /* the sense of the loop is reversed for backfacing culling */
      BGNPOLYGON();
      for (i=iloop-1; i>-1; i--) {
         V3F_D (cap[i], i, BACK_CAP);
      }
      ENDPOLYGON();
   }

}

#else /* NONCONCAVE_CAPS */

/* ============================================================ */
/* 
 * This subroutine draws a flat cap, to close off the cut ends 
 * of the cut-style join.  Properly handles concave endcaps.
 */

/* ARGSUSED4 */
static void draw_cut_style_cap_callback (int iloop,
                                  double cap[][3], 
                                  float face_color[3],
                                  gleDouble cut_vector[3],
                                  gleDouble bisect_vector[3],
                                  double norms[][3], 
                                  int frontwards)
{
   int i;
#ifdef OPENGL_10
   GLUtriangulatorObj *tobj;
   tobj = gluNewTess ();
   gluTessCallback (tobj, GLU_BEGIN, glBegin);
   gluTessCallback (tobj, GLU_VERTEX, glVertex3dv);
   gluTessCallback (tobj, GLU_END, glEnd);
#endif /* OPENGL_10 */

   if (face_color != NULL) C3F (face_color);

   if (frontwards) {

      /* if lighting is on, specify the endcap normal */
      if (cut_vector != NULL) {
         /* if normal pointing in wrong direction, flip it. */
         if (cut_vector[2] < 0.0) { 
            VEC_SCALE (cut_vector, -1.0, cut_vector); 
         }
         N3F_D (cut_vector);
      }
#ifdef GL_32
      BGNPOLYGON();
      for (i=0; i<iloop; i++) {
         V3F_D (cap[i], i, FRONT_CAP);
      }
      ENDPOLYGON();
#endif /* GL_32 */
#ifdef OPENGL_10
      gluBeginPolygon (tobj);
      for (i=0; i<iloop; i++) {
         gluTessVertex (tobj, cap[i], cap[i]);
      }
      gluEndPolygon (tobj);
#endif /* OPENGL_10 */
   } else {

      /* if lighting is on, specify the endcap normal */
      if (cut_vector != NULL) {
         /* if normal pointing in wrong direction, flip it. */
         if (cut_vector[2] > 0.0) {
            VEC_SCALE (cut_vector, -1.0, cut_vector); 
         }
         N3F_D (cut_vector);
      }
      /* the sense of the loop is reversed for backfacing culling */
#ifdef GL_32
      BGNPOLYGON();
      for (i=iloop-1; i>-1; i--) {
         V3F_D (cap[i], i, BACK_CAP);
      }
      ENDPOLYGON();
#endif /* GL_32 */
#ifdef OPENGL_10
      gluBeginPolygon (tobj);
      for (i=iloop-1; i>-1; i--) {
         gluTessVertex (tobj, cap[i], cap[i]);
      }
      gluEndPolygon (tobj);
#endif /* OPENGL_10 */
   }

#ifdef OPENGL_10
   gluDeleteTess (tobj);
#endif /* OPENGL_10 */

}
#endif /* NONCONCAVE_ENDCAPS */

/* ============================================================ */
/* 
 * This subroutine matchs the cap callback template, but is a no-op
 */

/* ARGSUSED */
void null_cap_callback (int iloop,
                        double cap[][3], 
                        float face_color[3],
                        gleDouble cut_vector[3],
                        gleDouble bisect_vector[3],
                        double norms[][3], 
                        int frontwards)
{}

/* ============================================================ */
/* 
 * This little routine draws the little idd-biddy fillet triangle with
 * the right  color, normal, etc.
 *
 * HACK ALERT -- there are two aspects to this routine/interface that
 * are "unfinished".
 * 1) the third point of the triangle should get a color thats
 *    interpolated beween the front and back color.  The interpolant
 *    is not currently being computed.  The error introduced by not 
 *    doing this should be tiny and/or non-exitant in almost all 
 *    expected uses of this code.
 *
 * 2) additional normal vectors should be supplied, and these should
 *    be interpolated to fit.  Currently, this is not being done.  As
 *    above, the expected error of not doing this should be tiny and/or
 *    non-existant in almost all expected uses of this code.
 */
/* ARGSUSED6 */
static void draw_fillet_triangle_plain
                          (gleDouble va[3],
                           gleDouble vb[3],
                           gleDouble vc[3],
                           int face,
                           float front_color[3],
                           float back_color[3])
{

   if (front_color != NULL) C3F (front_color);
   BGNTMESH (-5, 0.0);
   if (face) {
      V3F (va, -1, FILLET);
      V3F (vb, -1, FILLET);
   } else {
      V3F (vb, -1, FILLET);
      V3F (va, -1, FILLET);
   }
   V3F (vc, -1, FILLET);
   ENDTMESH ();

}

/* ============================================================ */
/* 
 * This little routine draws the little idd-biddy fillet triangle with
 * the right  color, normal, etc.
 *
 * HACK ALERT -- there are two aspects to this routine/interface that
 * are "unfinished".
 * 1) the third point of the triangle should get a color thats
 *    interpolated beween the front and back color.  The interpolant
 *    is not currently being computed.  The error introduced by not 
 *    doing this should be tiny and/or non-exitant in almost all 
 *    expected uses of this code.
 *
 * 2) additional normal vectors should be supplied, and these should
 *    be interpolated to fit.  Currently, this is not being done.  As
 *    above, the expected error of not doing this should be tiny and/or
 *    non-existant in almost all expected uses of this code.
 */
/* ARGSUSED5 */
static void draw_fillet_triangle_n_norms
                          (gleDouble va[3],
                           gleDouble vb[3],
                           gleDouble vc[3],
                           int face,
                           float front_color[3],
                           float back_color[3],
                           double na[3],
                           double nb[3])
{

   if (front_color != NULL) C3F (front_color);
   BGNTMESH (-5, 0.0);
   if (__TUBE_DRAW_FACET_NORMALS) {
      N3F_D (na);
      if (face) {
         V3F (va, -1, FILLET);
         V3F (vb, -1, FILLET);
      } else {
         V3F (vb, -1, FILLET);
         V3F (va, -1, FILLET);
      }
      V3F (vc, -1, FILLET);
   } else {
      if (face) {
         N3F_D (na);
         V3F (va, -1, FILLET);
         N3F_D (nb);
         V3F (vb, -1, FILLET);
      } else {
         N3F_D (nb);
         V3F (vb, -1, FILLET);
         N3F_D (na);
         V3F (va, -1, FILLET);
         N3F_D (nb);
      }
      V3F (vc, -1, FILLET);
   }
   ENDTMESH ();

}

/* ============================================================ */

static void draw_fillets_and_join_plain
                    (int ncp, 
                    gleDouble trimmed_loop[][3],
                    gleDouble untrimmed_loop[][3], 
                    int is_trimmed[],
                    gleDouble bis_origin[3], 
                    gleDouble bis_vector[3], 
                    float front_color[3],
                    float back_color[3],
                    gleDouble cut_vector[3], 
                    int face,
                    void ((*cap_callback) (int iloop,
                                  double cap[][3],
                                  float face_color[3],
                                  gleDouble cut_vector[3],
                                  gleDouble bisect_vector[3],
                                  double norms[][3],
                                  int frontwards)))
{
   int istop;
   int icnt, icnt_prev, iloop;
   double *cap_loop;
   gleDouble sect[3];
   gleDouble tmp_vec[3];
   int save_style;
   int was_trimmed = FALSE;

   cap_loop = (double *) malloc ((ncp+3)*3*sizeof (double));
   
   /* if the first point on the contour isn't trimmed, go ahead and
    * drop an edge down to the bisecting plane, (thus starting the 
    * join).  (Only need to do this for cut join, its bad if done for
    * round join).
    *
    * But if the first point is trimmed, keep going until one
    * is found that is not trimmed, and start join there.  */

   icnt = 0;
   iloop = 0;
   if (!is_trimmed[0]) {
      if (__TUBE_CUT_JOIN) {
         VEC_SUM (tmp_vec, trimmed_loop[0], bis_vector);
         INNERSECT (sect, 
                    bis_origin,
                    bis_vector,
                    trimmed_loop[0],
                    tmp_vec);
         VEC_COPY ( (&cap_loop[3*iloop]), sect);
         iloop ++;
      }
      VEC_COPY ( (&cap_loop[3*iloop]), (trimmed_loop[0])); 
      iloop++;
      icnt_prev = icnt;
      icnt ++;
   } else {

      /* else, loop until an untrimmed point is found */
      was_trimmed = TRUE;
      while (is_trimmed[icnt]) {
         icnt_prev = icnt;
         icnt ++;
         if (icnt >= ncp) { 
            free (cap_loop);
            return;    /* oops - everything was trimmed */
         }
      }
   }

   /* Start walking around the end cap.  Every time the end loop is
    * trimmed, we know we'll need to draw a fillet triangle.  In
    * addition, after every pair of visibility changes, we draw a cap. */
   if (__TUBE_CLOSE_CONTOUR) {
      istop = ncp;
   } else {
      istop = ncp-1;
   }

   /* save the join style, and disable a closed contour.
    * Need to do this so partial contours don't close up. */
   save_style = gleGetJoinStyle ();
   gleSetJoinStyle (save_style & ~TUBE_CONTOUR_CLOSED);

   for (; icnt_prev < istop; icnt_prev ++, icnt ++, icnt %= ncp) {

      /* There are four interesting cases for drawing caps and fillets:
       *    1) this & previous point were trimmed.  Don't do anything, 
       *       advance counter.
       *    2) this point trimmed, previous not -- draw fillet, and 
       *       draw cap.
       *    3) this point not trimmed, previous one was -- compute
       *       intersection point, draw fillet with it, and save 
       *       point for cap contour.
       *    4) this & previous point not trimmed -- save for endcap.
       */

      /* Case 1 -- noop, just advance pointers */
      if (is_trimmed[icnt_prev] && is_trimmed[icnt]) {
      }

      /* Case 2 --  Hah! first point! compute intersect & draw fillet! */
      if (is_trimmed[icnt_prev] && !is_trimmed[icnt]) {

         /* important note: the array "untrimmed" contains valid
          * untrimmed data ONLY when is_trim is TRUE.  Otherwise, 
          * only "trim" containes valid data */

         /* compute intersection */
         INNERSECT (sect, 
                    bis_origin,
                    bis_vector,
                    untrimmed_loop[icnt_prev],
                    trimmed_loop[icnt]);

         /* Draw Fillet */
         draw_fillet_triangle_plain (trimmed_loop[icnt_prev],
                               trimmed_loop[icnt],
                               sect,
                               face,
                               front_color,
                               back_color);

         VEC_COPY ( (&cap_loop[3*iloop]), sect);
         iloop ++;
         VEC_COPY ( (&cap_loop[3*iloop]), (trimmed_loop[icnt])); 
         iloop++;
      }

      /* Case 3 -- add to collection of points */
      if (!is_trimmed[icnt_prev] && !is_trimmed[icnt]) {
         VEC_COPY ( (&cap_loop[3*iloop]), (trimmed_loop[icnt])); 
         iloop++; 
      } 

      /* Case 4 -- Hah! last point!  draw fillet & draw cap!  */
      if (!is_trimmed[icnt_prev] && is_trimmed[icnt]) {
         was_trimmed = TRUE;

         /* important note: the array "untrimmed" contains valid
          * untrimmed data ONLY when is_trim is TRUE.  Otherwise, 
          * only "trim" containes valid data */

         /* compute intersection */
         INNERSECT (sect, 
                    bis_origin,
                    bis_vector,
                    trimmed_loop[icnt_prev],
                    untrimmed_loop[icnt]);

         /* Draw Fillet */
         draw_fillet_triangle_plain (trimmed_loop[icnt_prev],
                               trimmed_loop[icnt],
                               sect,
                               face,
                               front_color,
                               back_color);

         VEC_COPY ( (&cap_loop[3*iloop]), sect);
         iloop ++;

         /* draw cap */
         if (iloop >= 3) (*cap_callback) (iloop, 
                                          (gleDouble (*)[3]) cap_loop, 
                                          front_color,
                                          cut_vector,
                                          bis_vector,
                                          NULL,
                                          face);

         /* reset cap counter */
         iloop = 0;
      }
   }

   /* now, finish up in the same way that we started.  If the last
    * point of the contour is visible, drop an edge to the bisecting 
    * plane, thus finishing the join, and then, draw the join! */

   icnt --;  /* decrement to make up for loop exit condititons */
   icnt += ncp;
   icnt %= ncp;
   if ((!is_trimmed[icnt]) && (iloop >= 2))  {
   
      VEC_SUM (tmp_vec, trimmed_loop[icnt], bis_vector);
      INNERSECT (sect, 
                 bis_origin,
                 bis_vector,
                 trimmed_loop[icnt],
                 tmp_vec);
      VEC_COPY ( (&cap_loop[3*iloop]), sect);
      iloop ++;

      /* if nothing was ever trimmed, then we want to draw the 
       * cap the way the user asked for it -- closed or not closed.
       * Therefore, reset the closure flag to its original state.
       */
      if (!was_trimmed) {
         gleSetJoinStyle (save_style);
      }

      /* draw cap */
      (*cap_callback) (iloop, 
                       (gleDouble (*)[3]) cap_loop, 
                       front_color,
                       cut_vector,
                       bis_vector,
                       NULL,
                       face);
   }

   /* rest to the saved style */
   gleSetJoinStyle (save_style);
   free (cap_loop);
}

/* ============================================================ */

void draw_fillets_and_join_n_norms
                    (int ncp, 
                    gleDouble trimmed_loop[][3],
                    gleDouble untrimmed_loop[][3], 
                    int is_trimmed[],
                    gleDouble bis_origin[3], 
                    gleDouble bis_vector[3], 
                    double normals[][3],
                    float front_color[3],
                    float back_color[3],
                    gleDouble cut_vector[3], 
                    int face,
                    void ((*cap_callback) (int iloop,
                                  double cap[][3],
                                  float face_color[3],
                                  gleDouble cut_vector[3],
                                  gleDouble bisect_vector[3],
                                  double norms[][3],
                                  int frontwards)))
{
   int istop;
   int icnt, icnt_prev, iloop;
   double *cap_loop, *norm_loop;
   gleDouble sect[3];
   gleDouble tmp_vec[3];
   int save_style;
   int was_trimmed = FALSE;

   cap_loop = (double *) malloc ((ncp+3)*3*2*sizeof (double));
   norm_loop = cap_loop + (ncp+3)*3;
   
   /* if the first point on the contour isn't trimmed, go ahead and
    * drop an edge down to the bisecting plane, (thus starting the 
    * join).  (Only need to do this for cut join, its bad if done for
    * round join).
    *
    * But if the first point is trimmed, keep going until one
    * is found that is not trimmed, and start join there.  */

   icnt = 0;
   iloop = 0;
   if (!is_trimmed[0]) {
      if (__TUBE_CUT_JOIN) {
         VEC_SUM (tmp_vec, trimmed_loop[0], bis_vector);
         INNERSECT (sect, 
                    bis_origin,
                    bis_vector,
                    trimmed_loop[0],
                    tmp_vec);
         VEC_COPY ( (&cap_loop[3*iloop]), sect);
         VEC_COPY ( (&norm_loop[3*iloop]), normals[0]);
         iloop ++;
      }
      VEC_COPY ( (&cap_loop[3*iloop]), (trimmed_loop[0])); 
      VEC_COPY ( (&norm_loop[3*iloop]), normals[0]);
      iloop++;
      icnt_prev = icnt;
      icnt ++;
   } else {

      /* else, loop until an untrimmed point is found */
      was_trimmed = TRUE;
      while (is_trimmed[icnt]) {
         icnt_prev = icnt;
         icnt ++;
         if (icnt >= ncp) {
            free (cap_loop);
            return;    /* oops - everything was trimmed */
         }
      }
   }

   /* Start walking around the end cap.  Every time the end loop is
    * trimmed, we know we'll need to draw a fillet triangle.  In
    * addition, after every pair of visibility changes, we draw a cap. */
   if (__TUBE_CLOSE_CONTOUR) {
      istop = ncp;
   } else {
      istop = ncp-1;
   }

   /* save the join style, and disable a closed contour.
    * Need to do this so partial contours don't close up. */
   save_style = gleGetJoinStyle ();
   gleSetJoinStyle (save_style & ~TUBE_CONTOUR_CLOSED);

   for (; icnt_prev < istop; icnt_prev ++, icnt ++, icnt %= ncp) {

      /* There are four interesting cases for drawing caps and fillets:
       *    1) this & previous point were trimmed.  Don't do anything, 
       *       advance counter.
       *    2) this point trimmed, previous not -- draw fillet, and 
       *       draw cap.
       *    3) this point not trimmed, previous one was -- compute
       *       intersection point, draw fillet with it, and save 
       *       point for cap contour.
       *    4) this & previous point not trimmed -- save for endcap.
       */

      /* Case 1 -- noop, just advance pointers */
      if (is_trimmed[icnt_prev] && is_trimmed[icnt]) {
      }

      /* Case 2 --  Hah! first point! compute intersect & draw fillet! */
      if (is_trimmed[icnt_prev] && !is_trimmed[icnt]) {

         /* important note: the array "untrimmed" contains valid
          * untrimmed data ONLY when is_trim is TRUE.  Otherwise, 
          * only "trim" containes valid data */

         /* compute intersection */
         INNERSECT (sect, 
                    bis_origin,
                    bis_vector,
                    untrimmed_loop[icnt_prev],
                    trimmed_loop[icnt]);

         /* Draw Fillet */
         draw_fillet_triangle_n_norms (trimmed_loop[icnt_prev],
                               trimmed_loop[icnt],
                               sect,
                               face,
                               front_color,
                               back_color,
                               normals[icnt_prev],
                               normals[icnt]);
         VEC_COPY ( (&cap_loop[3*iloop]), sect);
         VEC_COPY ( (&norm_loop[3*iloop]), normals[icnt_prev]);
         iloop ++;
         VEC_COPY ( (&cap_loop[3*iloop]), (trimmed_loop[icnt])); 
         VEC_COPY ( (&norm_loop[3*iloop]), normals[icnt]);
         iloop++;
      }

      /* Case 3 -- add to collection of points */
      if (!is_trimmed[icnt_prev] && !is_trimmed[icnt]) {
         VEC_COPY ( (&cap_loop[3*iloop]), (trimmed_loop[icnt])); 
         VEC_COPY ( (&norm_loop[3*iloop]), normals[icnt]);
         iloop++; 
      } 

      /* Case 4 -- Hah! last point!  draw fillet & draw cap!  */
      if (!is_trimmed[icnt_prev] && is_trimmed[icnt]) {
         was_trimmed = TRUE;

         /* important note: the array "untrimmed" contains valid
          * untrimmed data ONLY when is_trim is TRUE.  Otherwise, 
          * only "trim" containes valid data */

         /* compute intersection */
         INNERSECT (sect, 
                    bis_origin,
                    bis_vector,
                    trimmed_loop[icnt_prev],
                    untrimmed_loop[icnt]);

         /* Draw Fillet */
         draw_fillet_triangle_n_norms (trimmed_loop[icnt_prev],
                               trimmed_loop[icnt],
                               sect,
                               face,
                               front_color,
                               back_color,
                               normals[icnt_prev],
                               normals[icnt]);

         VEC_COPY ( (&cap_loop[3*iloop]), sect);

         /* OK, maybe phong normals are wrong, but at least facet
          * normals will come out OK. */
         if (__TUBE_DRAW_FACET_NORMALS) {
            VEC_COPY ( (&norm_loop[3*iloop]), normals[icnt_prev]);
         } else {
            VEC_COPY ( (&norm_loop[3*iloop]), normals[icnt]);
         }
         iloop ++;

         /* draw cap */
         if (iloop >= 3) (*cap_callback) (iloop, 
                                          (gleDouble (*)[3]) cap_loop, 
                                          front_color,
                                          cut_vector,
                                          bis_vector,
                                          (gleDouble (*)[3]) norm_loop,
                                          face);

         /* reset cap counter */
         iloop = 0;
      }
   }

   /* now, finish up in the same way that we started.  If the last
    * point of the contour is visible, drop an edge to the bisecting 
    * plane, thus finishing the join, and then, draw the join! */

   icnt --;  /* decrement to make up for loop exit condititons */
   icnt += ncp;
   icnt %= ncp;
   if ((!is_trimmed[icnt]) && (iloop >= 2))  {
   
      if (__TUBE_CUT_JOIN) {
         VEC_SUM (tmp_vec, trimmed_loop[icnt], bis_vector);
         INNERSECT (sect, 
                    bis_origin,
                    bis_vector,
                    trimmed_loop[icnt],
                    tmp_vec);
         VEC_COPY ( (&cap_loop[3*iloop]), sect);
         VEC_COPY ( (&norm_loop[3*iloop]), normals[icnt]);
         iloop ++;
      }

      /* if nothing was ever trimmed, then we want to draw the 
       * cap the way the user asked for it -- closed or not closed.
       * Therefore, reset the closure flag to its original state.
       */
      if (!was_trimmed) {
         gleSetJoinStyle (save_style);
      }

      /* draw cap */
      (*cap_callback) (iloop, 
                       (gleDouble (*)[3]) cap_loop, 
                       front_color,
                       cut_vector,
                       bis_vector,
                       (gleDouble (*)[3]) norm_loop,
                       face);
   }

   /* rest to the saved style */
   gleSetJoinStyle (save_style);
   free (cap_loop);
}

/* ============================================================ */
/* This routine draws "cut" style extrusions.  
 */

void extrusion_round_or_cut_join (int ncp,	/* number of contour points */
                           gleDouble contour[][2],	/* 2D contour */
                           gleDouble cont_normal[][2],/* 2D normal vecs */
                           gleDouble up[3],	/* up vector for contour */
                           int npoints,		/* numpoints in poly-line */
                           gleDouble point_array[][3],	/* polyline */
                           float color_array[][3],	/* color of polyline */
                           gleDouble xform_array[][2][3])   /* 2D contour xforms */
{
   int i, j;
   int inext, inextnext;
   gleDouble m[4][4];
   gleDouble tube_len, seg_len;
   gleDouble diff[3];
   gleDouble bi_0[3], bi_1[3];		/* bisecting plane */
   gleDouble bisector_0[3], bisector_1[3];		/* bisecting plane */
   gleDouble cut_0[3], cut_1[3];	/* cutting planes */
   gleDouble lcut_0[3], lcut_1[3];	/* cutting planes */
   int valid_cut_0, valid_cut_1;	/* flag -- cut vector is valid */
   gleDouble end_point_0[3], end_point_1[3]; 
   gleDouble torsion_point_0[3], torsion_point_1[3]; 
   gleDouble isect_point[3];
   gleDouble origin[3], neg_z[3];
   gleDouble yup[3];		/* alternate up vector */
   gleDouble *front_cap, *back_cap;	/* arrays containing the end caps */
   gleDouble *front_loop, *back_loop; /* arrays containing the tube ends */
   double *front_norm, *back_norm; /* arrays containing normal vecs */
   double *norm_loop, *tmp; /* normal vectors, cast into 3d from 2d */
   int *front_is_trimmed, *back_is_trimmed;   /* T or F */
   float *front_color, *back_color;  /* pointers to segment colors */
   void ((*cap_callback) (int,double [][3],float [3],gleDouble [3], gleDouble [3], double [][3],int));  /* function callback to draw cap */
   void ((*tmp_cap_callback) (int,double [][3],float [3],gleDouble [3], gleDouble [3], double [][3],int));  /* function callback to draw cap */

   int join_style_is_cut;      /* TRUE if join style is cut */
   double dot;                  /* partial dot product */
   char *mem_anchor;
   int first_time = TRUE;
   gleDouble *cut_vec;

   /* create a local, block scope copy of of the join style.
    * this will alleviate wasted cycles and register write-backs */
   /* choose the right callback, depending on the choosen join style */
   if (__TUBE_CUT_JOIN) {
      join_style_is_cut = TRUE;
      cap_callback =  draw_cut_style_cap_callback;
   } else {
      join_style_is_cut = FALSE;
      cap_callback =  draw_round_style_cap_callback;
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

   /* the origin is at the origin */
   origin [0] = 0.0;
   origin [1] = 0.0;
   origin [2] = 0.0;

   /* and neg_z is at neg z */
   neg_z[0] = 0.0;
   neg_z[1] = 0.0;
   neg_z[2] = 1.0;

   /* malloc the data areas that we'll need to store the end-caps */
   mem_anchor = malloc (4 * 3*ncp*sizeof(gleDouble)
                      + 2 * 3*ncp*sizeof(double)
                      + 2 * 1*ncp*sizeof(int));
   front_norm = (double *) mem_anchor;
   back_norm = front_norm + 3*ncp;
   front_loop = (gleDouble *) (back_norm + 3*ncp);
   back_loop = front_loop + 3*ncp;
   front_cap = back_loop + 3*ncp;
   back_cap  = front_cap + 3*ncp;
   front_is_trimmed = (int *) (back_cap + 3*ncp);
   back_is_trimmed = front_is_trimmed + ncp;

   /* ======================================= */

   /* |-|-|-|-|-|-|-|-| SET UP FOR FIRST SEGMENT |-|-|-|-|-|-|-| */

   /* ignore all segments of zero length */
   i = 1;
   inext = i;
   FIND_NON_DEGENERATE_POINT (inext, npoints, seg_len, diff, point_array);
   tube_len = seg_len;	/* store for later use */

   /* may as well get the normals set up now */
   if (cont_normal != NULL) {
      if (xform_array == NULL) {
         norm_loop = front_norm;
         back_norm = norm_loop;
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
   } else {
      front_norm = back_norm = norm_loop = NULL;
   }

   /* get the bisecting plane */
   bisecting_plane (bi_0, point_array[i-1], 
                          point_array[i], 
                          point_array[inext]);

   /* compute cutting plane */
   CUTTING_PLANE (valid_cut_0, cut_0, point_array[i-1], 
                         point_array[i], 
                         point_array[inext]);
   
   /* reflect the up vector in the bisecting plane */
   VEC_REFLECT (yup, yup, bi_0);

   /* |-|-|-|-|-|-|-|-| START LOOP OVER SEGMENTS |-|-|-|-|-|-|-| */

   /* draw tubing, not doing the first segment */
   while (inext<npoints-1) {

      inextnext = inext;
      /* ignore all segments of zero length */
      FIND_NON_DEGENERATE_POINT (inextnext, npoints, 
                                 seg_len, diff, point_array);

      /* get the far bisecting plane */
      bisecting_plane (bi_1, point_array[i], 
                             point_array[inext], 
                             point_array[inextnext]);


      /* compute cutting plane */
      CUTTING_PLANE (valid_cut_1, cut_1, point_array[i], 
                            point_array[inext], 
                            point_array[inextnext]);  

      /* rotate so that z-axis points down v2-v1 axis, 
       * and so that origen is at v1 */
      uviewpoint (m, point_array[i], point_array[inext], yup);
      PUSHMATRIX ();
      MULTMATRIX (m);

      /* rotate the cutting planes into the local coordinate system */
      MAT_DOT_VEC_3X3 (lcut_0, m, cut_0);
      MAT_DOT_VEC_3X3 (lcut_1, m, cut_1);

      /* rotate the bisecting planes into the local coordinate system */
      MAT_DOT_VEC_3X3 (bisector_0, m, bi_0);
      MAT_DOT_VEC_3X3 (bisector_1, m, bi_1);


      neg_z[2] = -tube_len;

      /* draw the tube */
      /* --------- START OF TMESH GENERATION -------------- */
      for (j=0; j<ncp; j++) {

         /* set up the endpoints for segment clipping */
         if (xform_array == NULL) {
            VEC_COPY_2 (end_point_0, contour[j]);
            VEC_COPY_2 (end_point_1, contour[j]);
            VEC_COPY_2 (torsion_point_0, contour[j]);
            VEC_COPY_2 (torsion_point_1, contour[j]);
         } else {
            /* transform the contour points with the local xform */
            MAT_DOT_VEC_2X3 (end_point_0,
                             xform_array[inext-1], contour[j]);
            MAT_DOT_VEC_2X3 (torsion_point_0,
                             xform_array[inext], contour[j]);
            MAT_DOT_VEC_2X3 (end_point_1,
                             xform_array[inext], contour[j]);
            MAT_DOT_VEC_2X3 (torsion_point_1,
                             xform_array[inext-1], contour[j]);

            /* if there are normals and there are affine xforms,
             * then compute local coordinate system normals.
             * Set up the back normals. (The front normals we inherit
             * from previous pass through the loop).  */
            if (cont_normal != NULL) {
               /* do up the normal vectors with the inverse transpose */
               NORM_XFORM_2X2 ( (&back_norm[3*j]),
                                xform_array[inext],
                                cont_normal [j]);
            }
         }
         end_point_0 [2] = 0.0;
         torsion_point_0 [2] = 0.0;
         end_point_1 [2] = - tube_len;
         torsion_point_1 [2] = - tube_len;

         /* The two end-points define a line.  Intersect this line
          * against the clipping plane defined by the PREVIOUS
          * tube segment.  */

         /* if this and the last tube are co-linear, don't cut the angle
          * if you do, a divide by zero will result.  This and last tube
          * are co-linear when the cut vector is of zero length */
         if (valid_cut_0 && join_style_is_cut) {
             INNERSECT (isect_point,  /* isect point (returned) */
                       origin,		/* point on intersecting plane */
                       lcut_0,		/* normal vector to plane */
                       end_point_0,	/* point on line */
                       end_point_1);	/* another point on the line */	

            /* determine whether the raw end of the extrusion would have
             * been cut, by checking to see if the raw and is on the 
             * far end of the half-plane defined by the cut vector.
             * If the raw end is not "cut", then it is "trimmed".
             */
            if (lcut_0[2] < 0.0) { VEC_SCALE (lcut_0, -1.0, lcut_0); }
            dot = lcut_0[0] * end_point_0[0];
            dot += lcut_0[1] * end_point_0[1];

            VEC_COPY ((&front_loop[3*j]), isect_point);
         } else {
            /* actual value of dot not interseting; need 
             * only be positive so that if test below failes */
            dot = 1.0;   
            VEC_COPY ((&front_loop[3*j]), end_point_0);
         }

         INNERSECT (isect_point, 	/* intersection point (returned) */
                    origin,		/* point on intersecting plane */
                    bisector_0,		/* normal vector to plane */
                    end_point_0,	/* point on line */
                    torsion_point_1);	/* another point on the line */	

         /* trim out interior of intersecting tube */
         /* ... but save the untrimmed version for drawing the endcaps */
         /* ... note that cap contains valid data ONLY when is_trimmed
          * is TRUE. */
         if ((dot <= 0.0) || (isect_point[2] < front_loop[3*j+2])) {
/*
         if ((dot <= 0.0) || (front_loop[3*j+2] > 0.0)) {
*/
            VEC_COPY ((&front_cap[3*j]), (&front_loop [3*j]));
            VEC_COPY ((&front_loop[3*j]), isect_point);
            front_is_trimmed[j] = TRUE;
         } else {
            front_is_trimmed[j] = FALSE;
         }

         /* if intersection is behind the end of the segment, 
          * truncate to the end of the segment
          * Note that coding front_loop [3*j+2] = -tube_len;
          * doesn't work when twists are involved, */
         if (front_loop[3*j+2] < -tube_len) {
            VEC_COPY( (&front_loop[3*j]), end_point_1);
         } 

         /* --------------------------------------------------- */
         /* The two end-points define a line.  We did one endpoint 
          * above. Now do the other.Intersect this line
          * against the clipping plane defined by the NEXT
          * tube segment.  */

         /* if this and the last tube are co-linear, don't cut the angle
          * if you do, a divide by zero will result.  This and last tube
          * are co-linear when the cut vector is of zero length */
         if (valid_cut_1 && join_style_is_cut) {
            INNERSECT (isect_point,  /* isect point (returned) */
                       neg_z,		/* point on intersecting plane */
                       lcut_1,		/* normal vector to plane */
                       end_point_1,	/* point on line */
                       end_point_0);	/* another point on the line */	

            if (lcut_1[2] > 0.0) { VEC_SCALE (lcut_1, -1.0, lcut_1); }
            dot = lcut_1[0] * end_point_1[0];
            dot += lcut_1[1] * end_point_1[1];

   
            VEC_COPY ((&back_loop[3*j]), isect_point);
         } else {
            /* actual value of dot not interseting; need 
             * only be positive so that if test below failes */
            dot = 1.0;   
            VEC_COPY ((&back_loop[3*j]), end_point_1);
         }

         INNERSECT (isect_point, 	/* intersection point (returned) */
                    neg_z,		/* point on intersecting plane */
                    bisector_1,		/* normal vector to plane */
                    torsion_point_0,	/* point on line */
                    end_point_1);	/* another point on the line */	

         /* cut out interior of intersecting tube */
         /* ... but save the uncut version for drawing the endcaps */
         /* ... note that cap contains valid data ONLY when is
          *_trimmed is TRUE. */
/*
        if ((dot <= 0.0) || (back_loop[3*j+2] < -tube_len)) {
*/
        if ((dot <= 0.0) || (isect_point[2] > back_loop[3*j+2])) {
            VEC_COPY ((&back_cap[3*j]), (&back_loop [3*j]));
            VEC_COPY ((&back_loop[3*j]), isect_point);
            back_is_trimmed[j] = TRUE;
         } else {
            back_is_trimmed[j] = FALSE;
         }

         /* if intersection is behind the end of the segment, 
          * truncate to the end of the segment 
          * Note that coding back_loop [3*j+2] = 0.0;
          * doesn't work when twists are involved, */
         if (back_loop[3*j+2] > 0.0) {
            VEC_COPY( (&back_loop[3*j]), end_point_0);
         } 
      }

      /* --------- END OF TMESH GENERATION -------------- */

      /* |||||||||||||||||| START SEGMENT DRAW |||||||||||||||||||| */
      /* There are six different cases we can have for presence and/or
       * absecnce of colors and normals, and for interpretation of
       * normals. The blechy set of nested if statements below
       * branch to each of the six cases */
      if (xform_array == NULL) {
         if (color_array == NULL) {
            if (cont_normal == NULL) {
               draw_segment_plain (ncp, (gleVector *) front_loop, (gleVector *) back_loop, inext, seg_len);
            } else
            if (__TUBE_DRAW_FACET_NORMALS) {
               draw_segment_facet_n (ncp, (gleVector *) front_loop, (gleVector *) back_loop, (gleVector *) norm_loop, 
                                     inext, seg_len);
            } else {
               draw_segment_edge_n (ncp, (gleVector *) front_loop, (gleVector *) back_loop, (gleVector *) norm_loop,
                                    inext, seg_len);
            }
         } else {
            if (cont_normal == NULL) {
               draw_segment_color (ncp, (gleVector *) front_loop, (gleVector *) back_loop, 
                                   color_array[inext-1],
                                   color_array[inext], inext, seg_len);
            } else
            if (__TUBE_DRAW_FACET_NORMALS) {
               draw_segment_c_and_facet_n (ncp, 
                                   (gleVector *) front_loop, (gleVector *) back_loop, (gleVector *) norm_loop,
                                   color_array[inext-1],
                                   color_array[inext], inext, seg_len);
            } else {
               draw_segment_c_and_edge_n (ncp, 
                                   (gleVector *) front_loop, (gleVector *) back_loop, (gleVector *) norm_loop,
                                   color_array[inext-1],
                                   color_array[inext], inext, seg_len);
             }
          }
      } else {
         if (color_array == NULL) {
            if (cont_normal == NULL) {
               draw_segment_plain (ncp, (gleVector *) front_loop, (gleVector *) back_loop, inext, seg_len);
            } else 
            if (__TUBE_DRAW_FACET_NORMALS) {
               draw_binorm_segment_facet_n (ncp, (gleVector *) front_loop, (gleVector *) back_loop,
                                                 (gleVector *) front_norm, (gleVector *) back_norm, 
                                                 inext, seg_len);
            } else {
               draw_binorm_segment_edge_n (ncp, (gleVector *) front_loop, (gleVector *) back_loop,
                                                (gleVector *) front_norm, (gleVector *) back_norm,
                                                inext, seg_len);
            }
         } else {
            if (cont_normal == NULL) {
               draw_segment_color (ncp, (gleVector *) front_loop, (gleVector *) back_loop, 
                                   color_array[inext-1],
                                   color_array[inext], inext, seg_len);
            } else
            if (__TUBE_DRAW_FACET_NORMALS) {
               draw_binorm_segment_c_and_facet_n (ncp, 
                                   (gleVector *) front_loop, (gleVector *) back_loop, 
                                   (gleVector *) front_norm, (gleVector *) back_norm, 
                                   color_array[inext-1],
                                   color_array[inext], inext, seg_len);
            } else {
               draw_binorm_segment_c_and_edge_n (ncp, 
                                   (gleVector *) front_loop, (gleVector *) back_loop,
                                   (gleVector *) front_norm, (gleVector *) back_norm, 
                                   color_array[inext-1],
                                   color_array[inext], inext, seg_len);
             }
          }
      }
      /* |||||||||||||||||| END SEGMENT DRAW |||||||||||||||||||| */

      /* v^v^v^v^v^v^v^v^v  BEGIN END CAPS v^v^v^v^v^v^v^v^v^v^v^v */

      /* if end caps are required, draw them. But don't draw any
       * but the very first and last caps */
      if (first_time) {
         first_time = FALSE;
         tmp_cap_callback = cap_callback;
         cap_callback = null_cap_callback;
         if (__TUBE_DRAW_CAP) {
            if (color_array != NULL) C3F (color_array[inext-1]);
            draw_angle_style_front_cap (ncp, bisector_0, (gleDouble (*)[3]) front_loop);
         }
      }
      /* v^v^v^v^v^v^v^v^v  END END CAPS v^v^v^v^v^v^v^v^v^v^v^v */

      /* $$$$$$$$$$$$$$$$ BEGIN -1, FILLET & JOIN DRAW $$$$$$$$$$$$$$$$$ */
      /* 
       * Now, draw the fillet triangles, and the join-caps.
       */
      if (color_array != NULL) {
         front_color = color_array[inext-1];
         back_color = color_array[inext];
      } else {
         front_color = NULL;
         back_color = NULL;
      }

      if (cont_normal == NULL) {
         /* the flag valid-cut is true if the cut vector has a valid 
          * value (i.e. if a degenerate case has not occured). 
          */
         if (valid_cut_0) {
            cut_vec = lcut_0;
         } else {
            cut_vec = NULL;
         }
         draw_fillets_and_join_plain (ncp, 
                                  (gleVector *) front_loop,
                                  (gleVector *) front_cap, 
                                  front_is_trimmed,
                                  origin,
                                  bisector_0, 
                                  front_color,
                                  back_color,
                                  cut_vec,
                                  TRUE,
                                  cap_callback);

         /* v^v^v^v^v^v^v^v^v  BEGIN END CAPS v^v^v^v^v^v^v^v^v^v^v^v */
         if (inext == npoints-2) {
            if (__TUBE_DRAW_CAP) {
               if (color_array != NULL) C3F (color_array[inext]);
               draw_angle_style_back_cap (ncp, bisector_1, (gleDouble (*)[3]) back_loop);
               cap_callback = null_cap_callback;
            }
         } else {
            /* restore ability to draw cap */
            cap_callback = tmp_cap_callback;
         }
         /* v^v^v^v^v^v^v^v^v  END END CAPS v^v^v^v^v^v^v^v^v^v^v^v */
   
         /* the flag valid-cut is true if the cut vector has a valid 
          * value (i.e. if a degenerate case has not occured). 
          */
         if (valid_cut_1) {
            cut_vec = lcut_1;
         } else {
            cut_vec = NULL;
         }
         draw_fillets_and_join_plain (ncp, 
                                  (gleVector *) back_loop,
                                  (gleVector *) back_cap, 
                                  back_is_trimmed,
                                  neg_z,
                                  bisector_1, 
                                  back_color,
                                  front_color,
                                  cut_vec,
                                  FALSE,
                                  cap_callback);
      } else {
   
         /* the flag valid-cut is true if the cut vector has a valid 
          * value (i.e. if a degenerate case has not occured). 
          */
         if (valid_cut_0) {
            cut_vec = lcut_0;
         } else {
            cut_vec = NULL;
         }
         draw_fillets_and_join_n_norms (ncp, 
                                  (gleVector *) front_loop,
                                  (gleVector *) front_cap, 
                                  front_is_trimmed,
                                  origin,
                                  bisector_0, 
                                  (gleVector *) front_norm,
                                  front_color,
                                  back_color,
                                  cut_vec,
                                  TRUE,
                                  cap_callback);
   
         /* v^v^v^v^v^v^v^v^v  BEGIN END CAPS v^v^v^v^v^v^v^v^v^v^v^v */
         if (inext == npoints-2) {
            if (__TUBE_DRAW_CAP) {
               if (color_array != NULL) C3F (color_array[inext]);
               draw_angle_style_back_cap (ncp, bisector_1, (gleDouble (*)[3]) back_loop);
               cap_callback = null_cap_callback;
            }
         } else {
            /* restore ability to draw cap */
            cap_callback = tmp_cap_callback;
         }
         /* v^v^v^v^v^v^v^v^v  END END CAPS v^v^v^v^v^v^v^v^v^v^v^v */

         /* the flag valid-cut is true if the cut vector has a valid 
          * value (i.e. if a degenerate case has not occured). 
          */
         if (valid_cut_1) {
            cut_vec = lcut_1;
         } else {
            cut_vec = NULL;
         }
         draw_fillets_and_join_n_norms (ncp, 
                                  (gleVector *) back_loop,
                                  (gleVector *) back_cap, 
                                  back_is_trimmed,
                                  neg_z,
                                  bisector_1, 
                                  (gleVector *) back_norm,
                                  back_color,
                                  front_color,
                                  cut_vec,
                                  FALSE,
                                  cap_callback);
      }

      /* $$$$$$$$$$$$$$$$ END FILLET & JOIN DRAW $$$$$$$$$$$$$$$$$ */

      /* pop this matrix, do the next set */
      POPMATRIX ();

      /* slosh stuff over to next vertex */
      tmp = front_norm;
      front_norm = back_norm;
      back_norm = tmp;

      tube_len = seg_len;
      i = inext;
      inext = inextnext;
      VEC_COPY (bi_0, bi_1);
      VEC_COPY (cut_0, cut_1);
      valid_cut_0 = valid_cut_1;

      /* reflect the up vector in the bisecting plane */
      VEC_REFLECT (yup, yup, bi_0);
   }
   /* |-|-|-|-|-|-|-|-| END LOOP OVER SEGMENTS |-|-|-|-|-|-|-| */

   free (mem_anchor);

}
   
/* =================== END OF FILE =============================== */
