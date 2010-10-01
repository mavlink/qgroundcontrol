
/*
 * MODULE: extrude.c
 *
 * FUNCTION:
 * Provides  code for the cylinder, cone and extrusion routines. 
 * The cylinders/cones/etc. are built on top of general purpose
 * extrusions. The code that handles the general purpose extrusions 
 * is in other modules.
 * 
 * AUTHOR:
 * written by Linas Vepstas August/September 1991
 * added polycone, February 1993
 */

#include <stdlib.h>
#include <math.h>
#include <string.h>	/* for the memcpy() subroutine */
#include <GL/tube.h>
#include "gutil.h"
#include "vvector.h"
#include "tube_gc.h"
#include "extrude.h"
#include "intersect.h"

/* ============================================================ */
/* The routine below  determines the type of join style that will be
 * used for tubing. */

void gleSetJoinStyle (int style) 
{
   INIT_GC();
   extrusion_join_style = style;
}

int gleGetJoinStyle (void)
{
   INIT_GC();
   return (extrusion_join_style);
}

/* ============================================================ */
/*
 * draw a general purpose extrusion 
 */

void gleSuperExtrusion (int ncp,               /* number of contour points */
                gleDouble contour[][2],    /* 2D contour */
                gleDouble cont_normal[][2], /* 2D contour normals */
                gleDouble up[3],           /* up vector for contour */
                int npoints,           /* numpoints in poly-line */
                gleDouble point_array[][3],        /* polyline */
                float color_array[][3],        /* color of polyline */
                gleDouble xform_array[][2][3])   /* 2D contour xforms */
{   
   INIT_GC();
   _gle_gc -> ncp = ncp;
   _gle_gc -> contour = contour;
   _gle_gc -> cont_normal = cont_normal;
   _gle_gc -> up = up;
   _gle_gc -> npoints = npoints;
   _gle_gc -> point_array = point_array;
   _gle_gc -> color_array = color_array;
   _gle_gc -> xform_array = xform_array;

   switch (__TUBE_STYLE) {
      case TUBE_JN_RAW:
         (void) extrusion_raw_join (ncp, contour, cont_normal, up,
                                    npoints,
                                    point_array, color_array,
                                    xform_array);
         break;

      case TUBE_JN_ANGLE:
         (void) extrusion_angle_join (ncp, contour, cont_normal, up,
                                    npoints,
                                    point_array, color_array,
                                    xform_array);
         break;

      case TUBE_JN_CUT:
      case TUBE_JN_ROUND:
         /* This routine used for both cut and round styles */
         (void) extrusion_round_or_cut_join (ncp, contour, cont_normal, up,
                                    npoints,
                                    point_array, color_array,
                                    xform_array);
         break;

      default:
         break;
   }
}

/* ============================================================ */

void gleExtrusion (int ncp,               /* number of contour points */
                gleDouble contour[][2],    /* 2D contour */
                gleDouble cont_normal[][2], /* 2D contour normals */
                gleDouble up[3],           /* up vector for contour */
                int npoints,           /* numpoints in poly-line */
                gleDouble point_array[][3],        /* polyline */
                float color_array[][3])        /* color of polyline */
{   
   gleSuperExtrusion (ncp, contour, cont_normal, up,
                    npoints,
                    point_array, color_array,
                    NULL);
}

/* ============================================================ */

/* should really make this an adaptive algorithm ... */
static int __gleSlices = 20;

int
gleGetNumSlices(void)
{
  return __gleSlices;
}

void
gleSetNumSlices(int slices)
{
  __gleSlices = slices;
}

void gen_polycone (int npoints,
               gleDouble point_array[][3],
               float color_array[][3],
               gleDouble radius,
               gleDouble xform_array[][2][3])
{
   int saved_style;
   glePoint *circle = (glePoint*) malloc(sizeof(glePoint)*2*__gleSlices);
   glePoint *norm = &circle[__gleSlices];
   double c, s;
   int i;
   double v21[3];
   double len;
   gleDouble up[3];

   INIT_GC();

   /* this if statement forces this routine into double-duty for
    * both the polycone and the polycylinder routines */
   if (xform_array != NULL) radius = 1.0;

   s = sin (2.0*M_PI/ ((double) __gleSlices));
   c = cos (2.0*M_PI/ ((double) __gleSlices));

   norm [0][0] = 1.0;
   norm [0][1] = 0.0;
   circle [0][0] = radius;
   circle [0][1] = 0.0;

   /* draw a norm using recursion relations */
   for (i=1; i<__gleSlices; i++) {
      norm [i][0] = norm[i-1][0] * c - norm[i-1][1] * s;
      norm [i][1] = norm[i-1][0] * s + norm[i-1][1] * c;
      circle [i][0] = radius * norm[i][0];
      circle [i][1] = radius * norm[i][1];
   }

   /* avoid degenerate vectors */
   /* first, find a non-zero length segment */
   i=0;
   FIND_NON_DEGENERATE_POINT(i,npoints,len,v21,point_array)
   if (i == npoints) return;

   /* next, check to see if this segment lies along x-axis */
   if ((v21[0] == 0.0) && (v21[2] == 0.0)) {
      up[0] = up[1] = up[2] = 1.0;
   } else {
      up[0] = up[2] = 0.0;
      up[1] = 1.0;
   }

   /* save the current join style */
   saved_style = extrusion_join_style;
   extrusion_join_style |= TUBE_CONTOUR_CLOSED;

   /* if lighting is not turned on, don't send normals.  
    * MMODE is a good indicator of whether lighting is active */
   if (!__IS_LIGHTING_ON) {
       gleSuperExtrusion (__gleSlices, circle, NULL, up,
                     npoints, point_array, color_array,
                     xform_array);
   } else {
       gleSuperExtrusion (__gleSlices, circle, norm, up,
                     npoints, point_array, color_array,
                     xform_array);
   }
   
   /* restore the join style */
   extrusion_join_style = saved_style;

   free(circle);
}

/* ============================================================ */

void glePolyCylinder (int npoints,
                   gleDouble point_array[][3],
                   float color_array[][3],
                   gleDouble radius)
{
   gen_polycone (npoints, point_array, color_array, radius, NULL);
}

/* ============================================================ */

void glePolyCone (int npoints,
               gleDouble point_array[][3],
               float color_array[][3],
               gleDouble radius_array[])
{
   gleAffine * xforms;
   int j;

   /* build 2D affine matrices from radius array */
   xforms = (gleAffine *) malloc (npoints * sizeof(gleAffine));
   for (j=0; j<npoints; j++) {
      AVAL(xforms,j,0,0) = radius_array[j];
      AVAL(xforms,j,0,1) = 0.0;
      AVAL(xforms,j,0,2) = 0.0;
      AVAL(xforms,j,1,0) = 0.0;
      AVAL(xforms,j,1,1) = radius_array[j];
      AVAL(xforms,j,1,2) = 0.0;
   }

   gen_polycone (npoints, point_array, color_array, 1.0, xforms);

   free (xforms);
}

/* ============================================================ */

void gleTwistExtrusion (int ncp,         /* number of contour points */
                gleDouble contour[][2],    /* 2D contour */
                gleDouble cont_normal[][2], /* 2D contour normals */
                gleDouble up[3],           /* up vector for contour */
                int npoints,           /* numpoints in poly-line */
                gleDouble point_array[][3],        /* polyline */
                float color_array[][3],        /* color of polyline */
                gleDouble twist_array[])   /* countour twists (in degrees) */

{
   int j;
   double angle;
   double si, co;

   gleAffine *xforms;

   /* build 2D affine matrices from radius array */
   xforms = (gleAffine *) malloc (npoints * sizeof(gleAffine));

   for (j=0; j<npoints; j++) {
      angle = (M_PI/180.0) * twist_array[j];
      si = sin (angle);
      co = cos (angle);
      AVAL(xforms,j,0,0) = co;
      AVAL(xforms,j,0,1) = -si;
      AVAL(xforms,j,0,2) = 0.0;
      AVAL(xforms,j,1,0) = si;
      AVAL(xforms,j,1,1) = co;
      AVAL(xforms,j,1,2) = 0.0;
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

/* ============================================================ */
/* 
 * The spiral primitive forms the basis for the helicoid primitive.
 *
 * Note that this primitive sweeps a contour along a helical path.
 * The algorithm assumes that the path is embedded in Euclidean space,
 * and uses parallel transport along the path.  Parallel transport
 * provides the simplest mathematical model for moving a coordinate 
 * system along a curved path, but some of the effects of doing so 
 * may prove to be surprising to one uninitiated to the concept.
 *
 * Thus, we provide another, related, algorithm below, called "lathe"
 * which introduces a torsion component along the path, correcting for
 * the rotation induced by the helical path.
 *
 * If the above sounds like gobldy-gook to you, you may want to brush 
 * up on differential geometry. Recommend Spivak, Differential Geometry,
 * Volume 1, pages xx-xx.
 */

void gleSpiral (int ncp,               /* number of contour points */
             gleDouble contour[][2],    /* 2D contour */
             gleDouble cont_normal[][2], /* 2D contour normals */
             gleDouble up[3],           /* up vector for contour */
             gleDouble startRadius,
             gleDouble drdTheta,        /* change in radius per revolution */
             gleDouble startZ,
             gleDouble dzdTheta,        /* change in Z per revolution */
             gleDouble startXform[2][3],
             gleDouble dXformdTheta[2][3], /* tangent change xform per revolution */
             gleDouble startTheta,	      /* start angle, in degrees */
             gleDouble sweepTheta)        /* sweep angle, in degrees */
{
   int npoints;
   gleDouble deltaAngle;
   char * mem_anchor;
   gleDouble *pts;
   gleAffine *xforms;
   double delta;

   int saved_style;
   double ccurr, scurr;
   double cprev, sprev;
   double cdelta, sdelta;
   double mA[2][2], mB[2][2];
   double run[2][2];
   double deltaTrans[2];
   double trans[2];
   int i;

   /* allocate sufficient memory to store path */
   npoints = (int) ((((double) __gleSlices) /360.0) * fabs(sweepTheta)) + 4;

   if (startXform == NULL) {
      mem_anchor = malloc (3*npoints * sizeof (gleDouble));
      pts = (gleDouble *) mem_anchor;
      xforms = NULL;
   } else {
      mem_anchor = malloc ((1+2)* 3*npoints * sizeof (gleDouble));
      pts = (gleDouble *) mem_anchor;
      xforms = (gleAffine *) (pts + 3*npoints);
   }

   /* compute delta angle based on number of points */
   deltaAngle = (M_PI / 180.0) * sweepTheta / ((gleDouble) (npoints-3));
   startTheta *= M_PI / 180.0;
   startTheta -= deltaAngle;

   /* initialize factors */
   cprev = cos ((double) startTheta);
   sprev = sin ((double) startTheta);

   cdelta = cos ((double) deltaAngle);
   sdelta = sin ((double) deltaAngle);

   /* renormalize differential factors */
   delta = deltaAngle / (2.0 * M_PI);
   dzdTheta *= delta;
   drdTheta *= delta;

   /* remember, the first point is hidden, so back-step */
   startZ -=  dzdTheta;
   startRadius -=  drdTheta;

   /* draw spiral path using recursion relations for sine, cosine */
   for (i=0; i<npoints; i++) {
      pts [3*i] =  startRadius * cprev;
      pts [3*i+1] =  startRadius * sprev;
      pts [3*i+2] = (gleDouble) startZ;

      startZ +=  dzdTheta;
      startRadius +=  drdTheta;
      ccurr = cprev * cdelta - sprev * sdelta;
      scurr = cprev * sdelta + sprev * cdelta;
      cprev = ccurr;
      sprev = scurr;
   }

   /* If there is a deformation matrix specified, then a deformation
    * path must be generated also */
   if (startXform != NULL) {
      if (dXformdTheta == NULL) {
         for (i=0; i<npoints; i++) {
            xforms[i][0][0] = startXform[0][0];
            xforms[i][0][1] = startXform[0][1];
            xforms[i][0][2] = startXform[0][2];
            xforms[i][1][0] = startXform[1][0];
            xforms[i][1][1] = startXform[1][1];
            xforms[i][1][2] = startXform[1][2];
         }
      } else {
         /* 
          * if there is a differential matrix specified, treat it a 
          * a tangent (algebraic, infinitessimal) matrix.  We need to
          * project it into the group of real 2x2 matricies.  (Note that
          * the specified matrix is affine.  We treat the translation 
          * components linearly, and only treat the 2x2 submatrix as an 
          * algebraic tangenet).
          *
          * For exponentiaition, we use the well known approx:
          * exp(x) = lim (N->inf) (1+x/N) ** N
          * and take N=32. 
          */

         /* initialize translation and delta translation */
         deltaTrans[0] = delta * dXformdTheta[0][2];
         deltaTrans[1] = delta * dXformdTheta[1][2];
         trans[0] = startXform[0][2];
         trans[1] = startXform[1][2];
   
         /* prepare the tangent matrix */
         delta /= 32.0;
         mA[0][0] = 1.0 + delta * dXformdTheta[0][0];
         mA[0][1] = delta * dXformdTheta[0][1];
         mA[1][0] = delta * dXformdTheta[1][0];
         mA[1][1] = 1.0 + delta * dXformdTheta[1][1];
   
         /* compute exponential of matrix */
         MATRIX_PRODUCT_2X2 (mB, mA, mA);  /* squared */
         MATRIX_PRODUCT_2X2 (mA, mB, mB);  /* 4th power */
         MATRIX_PRODUCT_2X2 (mB, mA, mA);  /* 8th power */
         MATRIX_PRODUCT_2X2 (mA, mB, mB);  /* 16th power */
         MATRIX_PRODUCT_2X2 (mB, mA, mA);  /* 32nd power */
   
         /* initialize running matrix */
         COPY_MATRIX_2X2 (run, startXform);
   
         /* remember, the first point is hidden -- load some, any 
          * xform for the first point */
         xforms[0][0][0] = startXform[0][0];
         xforms[0][0][1] = startXform[0][1];
         xforms[0][0][2] = startXform[0][2];
         xforms[0][1][0] = startXform[1][0];
         xforms[0][1][1] = startXform[1][1];
         xforms[0][1][2] = startXform[1][2];

         for (i=1; i<npoints; i++) {
#ifdef FUNKY_C
            xforms[6*i] = run[0][0];
            xforms[6*i+1] = run[0][1];
            xforms[6*i+3] = run[1][0];
            xforms[6*i+4] = run[1][1];
#endif /* FUNKY_C */
            xforms[i][0][0] = run[0][0];
            xforms[i][0][1] = run[0][1];
            xforms[i][1][0] = run[1][0];
            xforms[i][1][1] = run[1][1];

            /* integrate to get exponential matrix */
            /* (Note that the group action is a left-action --
             * i.e. multiply on the left (not the right)) */
            MATRIX_PRODUCT_2X2 (mA, mB, run);
            COPY_MATRIX_2X2 (run, mA);
         
#ifdef FUNKY_C
            xforms[6*i+2] = trans [0];
            xforms[6*i+5] = trans [1];
#endif /* FUNKY_C */
            xforms[i][0][2] = trans [0];
            xforms[i][1][2] = trans [1];
            trans[0] += deltaTrans[0];
            trans[1] += deltaTrans[1];

         }
      }
   }

   /* save the current join style */
   saved_style = extrusion_join_style;

   /* Allow only angle joins (for performance reasons).
    * The idea is that if the tesselation is fine enough, then an angle
    * join should be sufficient to get the desired visual quality.  A 
    * raw join would look terrible, an cut join would leave garbage 
    * everywhere, and a round join will over-tesselate (and thus 
    * should be avoided for performance reasons). 
    */
   extrusion_join_style  &= ~TUBE_JN_MASK; 
   extrusion_join_style  |= TUBE_JN_ANGLE;

   gleSuperExtrusion (ncp, contour, cont_normal, up,
                  npoints, (gleVector *) pts, NULL, xforms);

   /* restore the join style */
   extrusion_join_style = saved_style;

   free (mem_anchor);

}


/* ============================================================ */
/* 
 */

void gleLathe (int ncp,               /* number of contour points */
             gleDouble contour[][2],    /* 2D contour */
             gleDouble cont_normal[][2], /* 2D contour normals */
             gleDouble up[3],           /* up vector for contour */
             gleDouble startRadius,
             gleDouble drdTheta,        /* change in radius per revolution */
             gleDouble startZ,
             gleDouble dzdTheta,        /* change in Z per revolution */
             gleDouble startXform[2][3],
             gleDouble dXformdTheta[2][3], /* tangent change xform per revln */
             gleDouble startTheta,	      /* start angle, in degrees */
             gleDouble sweepTheta)        /* sweep angle, in degrees */
{
   gleDouble localup[3];
   gleDouble len;
   gleDouble trans[2];
   gleDouble start[2][3], delt[2][3];

   /* Because the spiral always starts on the axis, and proceeds in the
    * positive y direction, we can see that valid up-vectors must lie 
    * in the x-z plane. Therefore, we make sure we have a valid up
    * vector by projecting it onto the x-z plane, and normalizing. */
   if (up[1] != 0.0) {
      localup[0] = up[0];
      localup[1] = 0.0;
      localup[2] = up[2];
      VEC_LENGTH (len, localup);
      if (len != 0.0) {
         len = 1.0/len;
         localup[0] *= len;
         localup[2] *= len;
         VEC_SCALE (localup, len, localup);
      } else {
         /* invalid up vector was passed in */
         localup[0] = 0.0;
         localup[2] = 1.0;
      }
   } else {
      VEC_COPY (localup, up);
   }

   /* the dzdtheta derivative and the drdtheta derivative form a vector
    * in the x-z plane.  dzdtheta is the z component, and drdtheta is 
    * the x component.  We need to convert this vector into the local 
    * coordinate system defined by the up vector.  We do this by 
    * applying a 2D rotation matrix. 
    */
   trans[0] = localup[2] * drdTheta - localup[0] * dzdTheta;
   trans[1] = localup[0] * drdTheta + localup[2] * dzdTheta;

   /* now, add this translation vector into the affine xform */
   if (startXform != NULL) {
      if (dXformdTheta != NULL) {
         COPY_MATRIX_2X3 (delt, dXformdTheta);
         delt[0][2] += trans[0];
         delt[1][2] += trans[1];
      } else {
         /*Hmm- the transforms don't exist */

         delt[0][0] = 0.0;
         delt[0][1] = 0.0;
         delt[0][2] = trans[0];
         delt[1][0] = 0.0;
         delt[1][1] = 0.0;
         delt[1][2] = trans[1];
      }
      gleSpiral (ncp, contour, cont_normal, up, 
              startRadius, 0.0, startZ, 0.0,
              startXform, delt,
              startTheta, sweepTheta);

   } else {
      /* Hmm- the transforms don't exist */
      start[0][0] = 1.0;
      start[0][1] = 0.0;
      start[0][2] = 0.0;
      start[1][0] = 0.0;
      start[1][1] = 1.0;
      start[1][2] = 0.0;

      delt[0][0] = 0.0;
      delt[0][1] = 0.0;
      delt[0][2] = trans[0];
      delt[1][0] = 0.0;
      delt[1][1] = 0.0;
      delt[1][2] = trans[1];
      gleSpiral (ncp, contour, cont_normal, up, 
              startRadius, 0.0, startZ, 0.0,
              start, delt,
              startTheta, sweepTheta);
   }
}


/* ============================================================ */
/*
 * Super-Helicoid primitive 
 */

typedef void (*HelixCallback) (
             int ncp,               
             gleDouble contour[][2],
             gleDouble cont_normal[][2],
             gleDouble up[3],
             gleDouble startRadius,
             gleDouble drdTheta,
             gleDouble startZ,
             gleDouble dzdTheta,
             gleDouble startXform[2][3],
             gleDouble dXformdTheta[2][3],
             gleDouble startTheta,
             gleDouble sweepTheta);

void super_helix (gleDouble rToroid,
             gleDouble startRadius,
             gleDouble drdTheta,        /* change in radius per revolution */
             gleDouble startZ,
             gleDouble dzdTheta,        /* change in Z per revolution */
             gleDouble startXform[2][3],
             gleDouble dXformdTheta[2][3], /* tangent change xform per revol */
             gleDouble startTheta,	      /* start angle, in degrees */
             gleDouble sweepTheta,        /* sweep angle, in degrees */
	     HelixCallback helix_callback)
{

   int saved_style;
   glePoint *circle = (glePoint*) malloc(sizeof(glePoint)*2*__gleSlices);
   glePoint *norm = &circle[__gleSlices];
   double c, s;
   int i;
   gleDouble up[3];

   /* initialize sine and cosine for circle recusrion equations */
   s = sin (2.0*M_PI/ ((double) __gleSlices));
   c = cos (2.0*M_PI/ ((double) __gleSlices));

   norm [0][0] = 1.0;
   norm [0][1] = 0.0;
   circle [0][0] = rToroid;
   circle [0][1] = 0.0;

   /* draw a norm using recursion relations */
   for (i=1; i<__gleSlices; i++) {
      norm [i][0] = norm[i-1][0] * c - norm[i-1][1] * s;
      norm [i][1] = norm[i-1][0] * s + norm[i-1][1] * c;
      circle [i][0] = rToroid * norm[i][0];
      circle [i][1] = rToroid * norm[i][1];
   }

   /* make up vector point along x axis */
   up[1] = up[2] = 0.0;
   up[0] = 1.0;

   /* save the current join style */
   saved_style = extrusion_join_style;
   extrusion_join_style |= TUBE_CONTOUR_CLOSED;
   extrusion_join_style |= TUBE_NORM_PATH_EDGE;

   /* if lighting is not turned on, don't send normals.  
    * MMODE is a good indicator of whether lighting is active */
   if (!__IS_LIGHTING_ON) {
      (*helix_callback) (__gleSlices, circle, NULL, up,
             startRadius,
             drdTheta,
             startZ,
             dzdTheta,
             startXform,
             dXformdTheta,
             startTheta,
             sweepTheta);
   } else {
      (*helix_callback) (__gleSlices, circle, norm, up,
             startRadius,
             drdTheta,
             startZ,
             dzdTheta,
             startXform,
             dXformdTheta,
             startTheta,
             sweepTheta);
   }
   
   /* restore the join style */
   extrusion_join_style = saved_style;

   free(circle);
}

/* ============================================================ */
/*
 * Helicoid primitive 
 * Uses Parallel Transport to take a circular contour along a helical
 * path.
 */

void gleHelicoid (gleDouble rToroid,
             gleDouble startRadius,
             gleDouble drdTheta,        /* change in radius per revolution */
             gleDouble startZ,
             gleDouble dzdTheta,        /* change in Z per revolution */
             gleDouble startXform[2][3],
             gleDouble dXformdTheta[2][3], /* tangent change xform per revol */
             gleDouble startTheta,	      /* start angle, in degrees */
             gleDouble sweepTheta)        /* sweep angle, in degrees */
{
   super_helix (rToroid,
             startRadius,
             drdTheta,        /* change in radius per revolution */
             startZ,
             dzdTheta,        /* change in Z per revolution */
             startXform,
             dXformdTheta, /* tangent change xform per revolution */
             startTheta,	      /* start angle, in degrees */
             sweepTheta,       /* sweep angle, in degrees */
             gleSpiral);
}


/* ============================================================ */
/*
 * Toroid primitive 
 * Uses a helical coordinate system dislocation to take a circular 
 * contour along a helical path.
 */

void gleToroid (gleDouble rToroid,
             gleDouble startRadius,
             gleDouble drdTheta,        /* change in radius per revolution */
             gleDouble startZ,
             gleDouble dzdTheta,        /* change in Z per revolution */
             gleDouble startXform[2][3],
             gleDouble dXformdTheta[2][3], /* tangent change xform per revol */
             gleDouble startTheta,	      /* start angle, in degrees */
             gleDouble sweepTheta)        /* sweep angle, in degrees */
{
   super_helix (rToroid,
             startRadius,
             drdTheta,        /* change in radius per revolution */
             startZ,
             dzdTheta,        /* change in Z per revolution */
             startXform,
             dXformdTheta, /* tangent change xform per revolution */
             startTheta,	      /* start angle, in degrees */
             sweepTheta,       /* sweep angle, in degrees */
             gleLathe);
}

/* ============================================================ */

void gleScrew (int ncp, 
               gleDouble contour[][2], 
               gleDouble cont_normal[][2], 
               gleDouble up[3],
               gleDouble startz,
               gleDouble endz,
               gleDouble twist) 
{
   int i, numsegs;
   gleVector * path; 
   gleDouble *twarr;
   gleDouble currz, delta; 
   gleDouble currang, delang; 

   /* no segment should rotate more than 18 degrees */
   numsegs = (int) fabs (twist / 18.0) + 4;

   /* malloc the extrusion array and the twist array */
   path = (gleVector *) malloc (numsegs * sizeof (gleVector));
   twarr = (gleDouble *) malloc (numsegs * sizeof (gleDouble));

   /* fill in the extrusion array and the twist array uniformly */
   delta = (endz-startz) / ((gleDouble) (numsegs-3));
   currz = startz-delta;
   delang = twist / ((gleDouble) (numsegs-3));
   currang = -delang;
   for (i=0; i<numsegs; i++) {
      path [i][0] = 0.0;
      path [i][1] = 0.0;
      path [i][2] = currz;
      currz += delta;
      twarr[i] = currang;
      currang +=delang;
   }

   gleTwistExtrusion (ncp, contour, cont_normal, up, numsegs, path, NULL, twarr);

   free (path);
   free (twarr);
}

/* ============================================================ */
