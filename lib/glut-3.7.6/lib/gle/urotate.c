
/* 
 * MODULE: urotate.c
 *
 * FUNCTION:
 * This module contains three different routines that compute rotation
 * matricies and return these to user.
 * Detailed description is provided below.
 *
 * HISTORY:
 * Developed & written, Linas Vepstas, Septmeber 1991
 * double precision port, March 1993
 *
 * DETAILED DESCRIPTION:
 * This module contains three routines:
 * --------------------------------------------------------------------
 *
 * void urot_about_axis (float m[4][4],      --- returned
 *                       float angle,        --- input 
 *                       float axis[3])      --- input
 * Computes a rotation matrix.
 * The rotation is around the the direction specified by the argument
 * argument axis[3].  User may specify vector which is not of unit
 * length.  The angle of rotation is specified in degrees, and is in the
 * right-handed direction.
 *
 * void rot_about_axis (float angle,        --- input 
 *                      float axis[3])      --- input
 * Same as above routine, except that the matrix is multiplied into the
 * GL matrix stack.
 *
 * --------------------------------------------------------------------
 *
 * void urot_axis (float m[4][4],      --- returned
 *                 float omega,        --- input
 *                 float axis[3])      --- input
 * Same as urot_about_axis(), but angle specified in radians.
 * It is assumed that the argument axis[3] is a vector of unit length.
 * If it is not of unit length, the returned matrix will not be correct.
 *
 * void rot_axis (float omega,        --- input
 *                float axis[3])      --- input
 * Same as above routine, except that the matrix is multiplied into the
 * GL matrix stack.
 *
 * --------------------------------------------------------------------
 *
 * void urot_omega (float m[4][4],       --- returned
 *                  float omega[3])      --- input
 * same as urot_axis(), but the angle is taken as the length of the
 * vector omega[3]
 *
 * void rot_omega (float omega[3])      --- input
 * Same as above routine, except that the matrix is multiplied into the
 * GL matrix stack.
 *
 * --------------------------------------------------------------------
 */

#include <math.h>
#include "gutil.h"

/* Some <math.h> files do not define M_PI... */
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
   
/* ========================================================== */

#ifdef __GUTIL_DOUBLE
void urot_axis_d (double m[4][4], 		/* returned */
                  double omega, 		/* input */
                  double axis[3])		/* input */
#else
void urot_axis_f (float m[4][4], 		/* returned */
                  float omega, 		/* input */
                  float axis[3])		/* input */
#endif
{
   double s, c, ssq, csq, cts;
   double tmp;

   /*
    * The formula coded up below can be derived by using the 
    * homomorphism between SU(2) and O(3), namely, that the
    * 3x3 rotation matrix R is given by
    *      t.R.v = S(-1) t.v S
    * where
    * t are the Pauli matrices (similar to Quaternions, easier to use)
    * v is an arbitrary 3-vector
    * and S is a 2x2 hermitian matrix:
    *     S = exp ( i omega t.axis / 2 )
    * 
    * (Also, remember that computer graphics uses the transpose of R).
    * 
    * The Pauli matrices are:
    * 
    *  tx = (0 1)          ty = (0 -i)            tz = (1  0)
    *       (1 0)               (i  0)                 (0 -1)
    *
    * Note that no error checking is done -- if the axis vector is not 
    * of unit length, you'll get strange results.
    */

   tmp = (double) omega / 2.0;
   s = sin (tmp);
   c = cos (tmp);

   ssq = s*s;
   csq = c*c;

   m[0][0] = m[1][1] = m[2][2] = csq - ssq;

   ssq *= 2.0;

   /* on-diagonal entries */
   m[0][0] += ssq * axis[0]*axis[0];
   m[1][1] += ssq * axis[1]*axis[1];
   m[2][2] += ssq * axis[2]*axis[2];

   /* off-diagonal entries */
   m[0][1] = m[1][0] = axis[0] * axis[1] * ssq;
   m[1][2] = m[2][1] = axis[1] * axis[2] * ssq;
   m[2][0] = m[0][2] = axis[2] * axis[0] * ssq;

   cts = 2.0 * c * s;

   tmp = cts * axis[2];
   m[0][1] += tmp;
   m[1][0] -= tmp;

   tmp = cts * axis[0];
   m[1][2] += tmp;
   m[2][1] -= tmp;

   tmp = cts * axis[1];
   m[2][0] += tmp;
   m[0][2] -= tmp;

   /* homogeneous entries */
   m[0][3] = m[1][3] = m[2][3] = m[3][2] = m[3][1] = m[3][0] = 0.0;
   m[3][3] = 1.0;


}

/* ========================================================== */

#ifdef __GUTIL_DOUBLE
void urot_about_axis_d (double m[4][4], 		/* returned */
                        double angle, 		/* input */
                        double axis[3])		/* input */
#else
void urot_about_axis_f (float m[4][4], 		/* returned */
                        float angle, 		/* input */
                        float axis[3])		/* input */
#endif
{
   gutDouble len, ax[3];

   angle *= M_PI/180.0;		/* convert to radians */

   /* renormalize axis vector, if needed */
   len = axis[0]*axis[0] + axis[1]*axis[1] + axis[2]*axis[2];

   /* we can save some machine instructions by normalizing only
    * if needed.  The compiler should be able to schedule in the 
    * if test "for free". */
   if (len != 1.0) {
      len = (gutDouble) (1.0 / sqrt ((double) len));
      ax[0] = axis[0] * len;
      ax[1] = axis[1] * len;
      ax[2] = axis[2] * len;
#ifdef __GUTIL_DOUBLE
      urot_axis_d (m, angle, ax);
#else
      urot_axis_f (m, angle, ax);
#endif
   } else {
#ifdef __GUTIL_DOUBLE
      urot_axis_d (m, angle, axis);
#else
      urot_axis_f (m, angle, axis);
#endif
   }
}

/* ========================================================== */

#ifdef __GUTIL_DOUBLE
void urot_omega_d (double m[4][4], 	/* returned */
                   double axis[3])		/* input */
#else
void urot_omega_f (float m[4][4], 	/* returned */
                   float axis[3])		/* input */
#endif
{
   gutDouble len, ax[3];

   /* normalize axis vector */
   len = axis[0]*axis[0] + axis[1]*axis[1] + axis[2]*axis[2];

   len = (gutDouble) (1.0 / sqrt ((double) len));
   ax[0] = axis[0] * len;
   ax[1] = axis[1] * len;
   ax[2] = axis[2] * len;

   /* the amount of rotation is equal to the length, in radians */
#ifdef __GUTIL_DOUBLE
   urot_axis_d (m, len, ax);
#else
   urot_axis_f (m, len, ax);
#endif

}

/* ======================= END OF FILE ========================== */
