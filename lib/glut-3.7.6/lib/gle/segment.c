
/*
 * MODULE NAME: segment.c
 *
 * FUNCTION:
 * This module contains code that draws cylinder sections.  There are a
 * number of different segment routines presented: with and without colors,
 * with and without normals, with and without front and back normals.
 *
 * HISTORY:
 * written by Linas Vepstas August/September 1991
 * split into multiple compile units, Linas, October 1991
 * added normal vectors Linas, October 1991
 * consoldated from other modules,  Linas Vepstas, March 1993
 */

#include <malloc.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>	/* for the memcpy() subroutine */
#include "GL/tube.h"
#include "port.h"
#include "extrude.h"
#include "tube_gc.h"
#include "segment.h"


/* ============================================================ */

void draw_segment_plain (int ncp,	/* number of contour points */
                         gleDouble front_contour[][3],	
                         gleDouble back_contour[][3],
                         int inext, double len)
{
   int j;

   /* draw the tube segment */
   BGNTMESH (inext, len);
   for (j=0; j<ncp; j++) {
      V3F (front_contour[j], j, FRONT);
      V3F (back_contour[j], j, BACK);
   }

   if (__TUBE_CLOSE_CONTOUR) {
      /* connect back up to first point of contour */
      V3F (front_contour[0], 0, FRONT);
      V3F (back_contour[0], 0, BACK);
   }
   ENDTMESH ();
}

/* ============================================================ */

void draw_segment_color (int ncp,	/* number of contour points */
                         gleDouble front_contour[][3],	
                         gleDouble back_contour[][3],	
                         float color_last[3],
                         float color_next[3],
                         int inext, double len)
{
   int j;

   /* draw the tube segment */
   BGNTMESH (inext, len);
   for (j=0; j<ncp; j++) {
      C3F (color_last);
      V3F (front_contour[j], j, FRONT);

      C3F (color_next);
      V3F (back_contour[j], j, BACK);
   }

   if (__TUBE_CLOSE_CONTOUR) {
      /* connect back up to first point of contour */
      C3F (color_last);
      V3F (front_contour[0], 0, FRONT);

      C3F (color_next);
      V3F (back_contour[0], 0, BACK);
   }

   ENDTMESH ();
}

/* ============================================================ */

void draw_segment_edge_n (int ncp,	/* number of contour points */
                           gleDouble front_contour[][3],	
                           gleDouble back_contour[][3],	
                           double norm_cont[][3],
                           int inext, double len)
{
   int j;

   /* draw the tube segment */
   BGNTMESH (inext,len);
   for (j=0; j<ncp; j++) {
      N3F_D (norm_cont[j]);
      V3F (front_contour[j], j, FRONT);
      V3F (back_contour[j], j, BACK);
   }

   if (__TUBE_CLOSE_CONTOUR) {
      /* connect back up to first point of contour */
      N3F_D (norm_cont[0]);
      V3F (front_contour[0], 0, FRONT);
      V3F (back_contour[0], 0, BACK);
   }
   ENDTMESH ();
}

/* ============================================================ */

void draw_segment_c_and_edge_n (int ncp,	/* number of contour points */
                           gleDouble front_contour[][3],	
                           gleDouble back_contour[][3],	
                           double norm_cont[][3],
                           float color_last[3],
                           float color_next[3],
                           int inext, double len)
{
   int j;

   /* draw the tube segment */
   BGNTMESH (inext, len);
   for (j=0; j<ncp; j++) {
      C3F (color_last);
      N3F_D (norm_cont[j]);
      V3F (front_contour[j], j, FRONT);

      C3F (color_next);
      N3F_D (norm_cont[j]);
      V3F (back_contour[j], j, BACK);
   }

   if (__TUBE_CLOSE_CONTOUR) {
      /* connect back up to first point of contour */
      C3F (color_last);
      N3F_D (norm_cont[0]);
      V3F (front_contour[0], 0, FRONT);
   
      C3F (color_next);
      N3F_D (norm_cont[0]);
      V3F (back_contour[0], 0, BACK);
   }
   ENDTMESH ();
}

/* ============================================================ */

void draw_segment_facet_n (int ncp,	/* number of contour points */
                           gleDouble front_contour[][3],	
                           gleDouble back_contour[][3],	
                           double norm_cont[][3],
                           int inext, double len)
{
   int j;

   /* draw the tube segment */
   BGNTMESH (inext, len);
   for (j=0; j<ncp-1; j++) {
      N3F_D (norm_cont[j]);
      V3F (front_contour[j], j, FRONT);
      V3F (back_contour[j], j, BACK);
      V3F (front_contour[j+1], j+1, FRONT);
      V3F (back_contour[j+1], j+1, BACK);
   }

   if (__TUBE_CLOSE_CONTOUR) {
      /* connect back up to first point of contour */
      N3F_D (norm_cont[ncp-1]);
      V3F (front_contour[ncp-1], ncp-1, FRONT);
      V3F (back_contour[ncp-1], ncp-1, BACK);
      V3F (front_contour[0], 0, FRONT);
      V3F (back_contour[0], 0, BACK);
   }

   ENDTMESH ();
}

/* ============================================================ */

void draw_segment_c_and_facet_n (int ncp,	/* number of contour points */
                           gleDouble front_contour[][3],	
                           gleDouble back_contour[][3],	
                           double norm_cont[][3],
                           float color_last[3],
                           float color_next[3],
                           int inext, double len)
{
   int j;
   /* Note about this code:
    * At first, when looking at this code, it appears to be really dumb:
    * the N3F() call appears to be repeated multiple times, for no
    * apparent purpose.  It would seem that a performance improvement
    * would be gained by stripping it out. !DONT DO IT!
    * When there are no local lights or viewers, the V3F() subroutine
    * does not trigger a recalculation of the lighting equations.
    * However, we MUST trigger lighting, since otherwise colors come out
    * wrong.  Trigger lighting by doing an N3F call.
    */

   /* draw the tube segment */
   BGNTMESH (inext, len);
   for (j=0; j<ncp-1; j++) {
      C3F (color_last);
      N3F_D (norm_cont[j]);
      V3F (front_contour[j], j, FRONT);

      C3F (color_next);
      N3F_D (norm_cont[j]);
      V3F (back_contour[j], j, BACK);

      C3F (color_last);
      N3F_D (norm_cont[j]);
      V3F (front_contour[j+1], j+1, FRONT);

      C3F (color_next);
      N3F_D (norm_cont[j]);
      V3F (back_contour[j+1], j+1, BACK);
   }

   if (__TUBE_CLOSE_CONTOUR) {
      /* connect back up to first point of contour */
      C3F (color_last);
      N3F_D (norm_cont[ncp-1]);
      V3F (front_contour[ncp-1], ncp-1, FRONT);
   
      C3F (color_next);
      N3F_D (norm_cont[ncp-1]);
      V3F (back_contour[ncp-1], ncp-1, BACK);
   
      C3F (color_last);
      N3F_D (norm_cont[ncp-1]);
      V3F (front_contour[0], 0, FRONT);
   
      C3F (color_next);
      N3F_D (norm_cont[ncp-1]);
      V3F (back_contour[0], 0, BACK);
   }

   ENDTMESH ();
}

/* ============================================================ */
/* ============================================================ */
/* 
 * This routine draws a segment with normals specified at each end.
 */

void draw_binorm_segment_edge_n (int ncp,      /* number of contour points */
                           double front_contour[][3],
                           double back_contour[][3],
                           double front_norm[][3],
                           double back_norm[][3],
                           int inext, double len)
{
   int j;

   /* draw the tube segment */
   BGNTMESH (inext, len);
   for (j=0; j<ncp; j++) {
      N3F_D (front_norm[j]);
      V3F_D (front_contour[j], j, FRONT);
      N3F_D (back_norm[j]);
      V3F_D (back_contour[j], j, BACK);
   }

   if (__TUBE_CLOSE_CONTOUR) {
      /* connect back up to first point of contour */
      N3F_D (front_norm[0]);
      V3F_D (front_contour[0], 0, FRONT);
      N3F_D (back_norm[0]);
      V3F_D (back_contour[0], 0, BACK);
   }
   ENDTMESH ();

}

/* ============================================================ */

void draw_binorm_segment_c_and_edge_n (int ncp,	/* number of contour points */
                           double front_contour[][3],	
                           double back_contour[][3],	
                           double front_norm[][3],
                           double back_norm[][3],
                           float color_last[3],
                           float color_next[3],
                           int inext, double len)
{
   int j;

   /* draw the tube segment */
   BGNTMESH (inext, len);
   for (j=0; j<ncp; j++) {
      C3F (color_last);
      N3F_D (front_norm[j]);
      V3F_D (front_contour[j], j, FRONT);

      C3F (color_next);
      N3F_D (back_norm[j]);
      V3F_D (back_contour[j], j, BACK);
   }

   if (__TUBE_CLOSE_CONTOUR) {
      /* connect back up to first point of contour */
      C3F (color_last);
      N3F_D (front_norm[0]);
      V3F_D (front_contour[0], 0, FRONT);
   
      C3F (color_next);
      N3F_D (back_norm[0]);
      V3F_D (back_contour[0], 0, BACK);
   }
   ENDTMESH ();
}

/* ============================================================ */
/* 
 * This routine draws a piece of the round segment with psuedo-facet
 * normals.  I say "psuedo-facet" because the resulting object looks 
 * much, much better than real facet normals, and is what the  round
 * join style was meant to accomplish for face normals.   
 */

void draw_binorm_segment_facet_n (int ncp,      /* number of contour points */
                           double front_contour[][3],
                           double back_contour[][3],
                           double front_norm[][3],
                           double back_norm[][3],
                           int inext, double len)
{
   int j;

   /* draw the tube segment */
   BGNTMESH (inext, len);
   for (j=0; j<ncp-1; j++) {
      N3F_D (front_norm[j]);
      V3F_D (front_contour[j], j, FRONT);

      N3F_D (back_norm[j]);
      V3F_D (back_contour[j], j, BACK);

      N3F_D (front_norm[j]);
      V3F_D (front_contour[j+1], j+1, FRONT);

      N3F_D (back_norm[j]);
      V3F_D (back_contour[j+1], j+1, BACK);
   }

   if (__TUBE_CLOSE_CONTOUR) {
      /* connect back up to first point of contour */
      N3F_D (front_norm[ncp-1]);
      V3F_D (front_contour[ncp-1], ncp-1, FRONT);

      N3F_D (back_norm[ncp-1]);
      V3F_D (back_contour[ncp-1], ncp-1, BACK);

      N3F_D (front_norm[ncp-1]);
      V3F_D (front_contour[0], 0, FRONT);

      N3F_D (back_norm[ncp-1]);
      V3F_D (back_contour[0], 0, BACK);
   }
   ENDTMESH ();
}

/* ============================================================ */

void draw_binorm_segment_c_and_facet_n (int ncp,
                           double front_contour[][3],	
                           double back_contour[][3],	
                           double front_norm[][3],
                           double back_norm[][3],
                           float color_last[3],
                           float color_next[3],
                           int inext, double len)
{
   int j;

   /* draw the tube segment */
   BGNTMESH (inext, len);
   for (j=0; j<ncp-1; j++) {
      C3F (color_last);
      N3F_D (front_norm[j]);
      V3F_D (front_contour[j], j, FRONT);

      C3F (color_next);
      N3F_D (back_norm[j]);
      V3F_D (back_contour[j], j, BACK);

      C3F (color_last);
      N3F_D (front_norm[j]);
      V3F_D (front_contour[j+1], j+1, FRONT);

      C3F (color_next);
      N3F_D (back_norm[j]);
      V3F_D (back_contour[j+1], j+1, BACK);
   }

   if (__TUBE_CLOSE_CONTOUR) {
      /* connect back up to first point of contour */
      C3F (color_last);
      N3F_D (front_norm[ncp-1]);
      V3F_D (front_contour[ncp-1], ncp-1, FRONT);
   
      C3F (color_next);
      N3F_D (back_norm[ncp-1]);
      V3F_D (back_contour[ncp-1], ncp-1, BACK);
   
      C3F (color_last);
      N3F_D (front_norm[ncp-1]);
      V3F_D (front_contour[0], 0, FRONT);
   
      C3F (color_next);
      N3F_D (back_norm[ncp-1]);
      V3F_D (back_contour[0], 0, BACK);
   }

   ENDTMESH ();
}

/* ==================== END OF FILE =========================== */
