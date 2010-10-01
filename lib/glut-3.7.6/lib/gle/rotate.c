/* 
 * MODULE NAME: rotate.c
 *
 * FUNCTION:
 * This module contains three different routines that compute rotation
 * matricies and load them into GL.
 * Detailed description is provided below.
 *
 * DEPENDENCIES:
 * The routines call GL matrix routines.
 *
 * HISTORY:
 * Developed & written, Linas Vepstas, Septmeber 1991
 * Double precision port, March 1993
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

#include <GL/tube.h>
#include "port.h"
#include "gutil.h"
   
/* ========================================================== */

#ifdef __GUTIL_DOUBLE
void rot_axis_d (double omega, 		/* input */
               double axis[3])		/* input */
{
   double m[4][4];

   (void) urot_axis_d (m, omega, axis);
   MULTMATRIX_D (m);

}
#else 

void rot_axis_f (float omega, 		/* input */
               float axis[3])		/* input */
{
   float m[4][4];

   (void) urot_axis_f (m, omega, axis);
   MULTMATRIX_F (m);

}
#endif 

/* ========================================================== */

#ifdef __GUTIL_DOUBLE
void rot_about_axis_d (double angle, 		/* input */
                       double axis[3])		/* input */
{
   double m[4][4];

   (void) urot_about_axis_d (m, angle, axis);
   MULTMATRIX_D (m);
}

#else
void rot_about_axis_f (float angle, 		/* input */
                       float axis[3])		/* input */
{
   float m[4][4];

   (void) urot_about_axis_f (m, angle, axis);
   MULTMATRIX_F (m);
}
#endif

/* ========================================================== */

#ifdef __GUTIL_DOUBLE
void rot_omega_d (double axis[3])		/* input */
{
   double m[4][4];

   (void) urot_omega_d (m, axis);
   MULTMATRIX_D(m);
}
#else
void rot_omega_f (float axis[3])		/* input */
{
   float m[4][4];

   (void) urot_omega_f (m, axis);
   MULTMATRIX_F(m);
}
#endif

/* ========================================================== */
