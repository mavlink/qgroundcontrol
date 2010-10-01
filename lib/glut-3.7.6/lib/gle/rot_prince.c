
#include <math.h>
#include "rot.h"
#include "port.h"

/* ========================================================== */
/* 
 * The routines below generate and return more traditional rotation
 * matrices -- matrices for rotations about principal axes.
 */
/* ========================================================== */

#ifdef __GUTIL_DOUBLE
void urotx_sc_d (double m[4][4], 		/* returned */
                 double cosine,		/* input */
                 double sine) 		/* input */
#else
void urotx_sc_f (float m[4][4], 		/* returned */
                 float cosine,		/* input */
                 float sine) 		/* input */
#endif
{
   /* create matrix that represents rotation about the x-axis */

   ROTX_CS (m, cosine, sine);
}

/* ========================================================== */

#if 0
#ifdef __GUTIL_DOUBLE
void rotx_cs_d (double cosine,		/* input */
                double sine) 		/* input */
{
   /* create and load matrix that represents rotation about the x-axis */
   double m[4][4];

   (void) urotx_cs_d (m, cosine, sine);
   MULTMATRIX_D (m);
}

#else 
void rotx_cs_f (float cosine,		/* input */
                float sine) 		/* input */
{
   /* create and load matrix that represents rotation about the x-axis */
   float m[4][4];

   (void) urotx_cs_f (m, cosine, sine);
   MULTMATRIX_F (m);
}
#endif
#endif

/* ========================================================== */

#ifdef __GUTIL_DOUBLE
void uroty_sc_d (double m[4][4], 		/* returned */
                 double cosine,		/* input */
                 double sine) 		/* input */
#else
void uroty_sc_f (float m[4][4], 		/* returned */
                 float cosine,		/* input */
                 float sine) 		/* input */
#endif
{
   /* create matriy that represents rotation about the y-ayis */

   ROTX_CS (m, cosine, sine);
}

/* ========================================================== */

#if 0
#ifdef __GUTIL_DOUBLE
void roty_cs_d (double cosine,		/* input */
                double sine) 		/* input */
{
   /* create and load matriy that represents rotation about the y-ayis */
   double m[4][4];

   (void) uroty_cs_d (m, cosine, sine);
   MULTMATRIX_D (m);
}

#else 
void roty_cs_f (float cosine,		/* input */
                float sine) 		/* input */
{
   /* create and load matriy that represents rotation about the y-ayis */
   float m[4][4];

   (void) uroty_cs_f (m, cosine, sine);
   MULTMATRIX_F (m);
}
#endif
#endif

/* ========================================================== */

#ifdef __GUTIL_DOUBLE
void urotz_sc_d (double m[4][4], 		/* returned */
                 double cosine,		/* input */
                 double sine) 		/* input */
#else
void urotz_sc_f (float m[4][4], 		/* returned */
                 float cosine,		/* input */
                 float sine) 		/* input */
#endif
{
   /* create matriz that represents rotation about the z-azis */

   ROTX_CS (m, cosine, sine);
}

/* ========================================================== */

#if 0
#ifdef __GUTIL_DOUBLE
void rotz_cs_d (double cosine,		/* input */
                double sine) 		/* input */
{
   /* create and load matriz that represents rotation about the z-azis */
   double m[4][4];

   (void) urotz_cs_d (m, cosine, sine);
   MULTMATRIX_D (m);
}

#else 
void rotz_cs_f (float cosine,		/* input */
                float sine) 		/* input */
{
   /* create and load matriz that represents rotation about the z-azis */
   float m[4][4];

   (void) urotz_cs_f (m, cosine, sine);
   MULTMATRIX_F (m);
}
#endif
#endif

/* ========================================================== */

#if 0
#ifdef __GUTIL_DOUBLE
void urot_cs_d (double m[4][4],		/* returned */
                double cosine,		/* input */
                double sine,		/* input */
                char axis) 		/* input */
{
   /* create matrix that represents rotation about a principle axis */

   switch (axis) {
      case 'x':
      case 'X':
         urotx_cs_d (m, cosine, sine);
         break;
      case 'y':
      case 'Y':
         uroty_cs_d (m, cosine, sine);
         break;
      case 'z':
      case 'Z':
         urotz_cs_d (m, cosine, sine);
         break;
      default:
         break;
   }

}

#else
void urot_cs_f (float m[4][4],		/* returned */
                float cosine,		/* input */
                float sine,		/* input */
                char axis) 		/* input */
{
   /* create matrix that represents rotation about a principle axis */

   switch (axis) {
      case 'x':
      case 'X':
         urotx_cs_f (m, cosine, sine);
         break;
      case 'y':
      case 'Y':
         uroty_cs_f (m, cosine, sine);
         break;
      case 'z':
      case 'Z':
         urotz_cs_f (m, cosine, sine);
         break;
      default:
         break;
   }

}
#endif 
#endif

/* ========================================================== */

#if 0
#ifdef __GUTIL_DOUBLE
void rot_cs_d (double cosine,		/* input */
               double sine,		/* input */
               char axis)  		/* input */
{
   /* create and load matrix that represents rotation about the z-axis */
   double m[4][4];

   (void) urot_cs_d (m, cosine, sine, axis);
   MULTMATRIX_D (m);
}
#else
void rot_cs_f (float cosine,		/* input */
               float sine,		/* input */
               char axis)  		/* input */
{
   /* create and load matrix that represents rotation about the z-axis */
   float m[4][4];

   (void) urot_cs_f (m, cosine, sine, axis);
   MULTMATRIX_F (m);
}
#endif
#endif

/* ========================================================== */

#if 0
#ifdef __GUTIL_DOUBLE
void urot_prince_d (double m[4][4],	/* returned */
                    double theta,		/* input */
                    char axis) 		/* input */
{
   /* 
    * generate rotation matrix for rotation around principal axis;
    * note that angle is measured in radians (divide by 180, multiply by
    * PI to convert from degrees).
    */

   (void) urot_cs_d (m, 
                   cos (theta),
                   sin (theta),
                   axis);
}
#else 
void urot_prince_f (float m[4][4],	/* returned */
                    float theta,		/* input */
                    char axis) 		/* input */
{
   /* 
    * generate rotation matrix for rotation around principal axis;
    * note that angle is measured in radians (divide by 180, multiply by
    * PI to convert from degrees).
    */

   (void) urot_cs_f (m, 
                   (float) cos ((double) theta),
                   (float) sin ((double) theta),
                   axis);
}
#endif
#endif

/* ========================================================== */

#if 0
#ifdef __GUTIL_DOUBLE
void rot_prince_d (double theta,	/* input */
                   char axis) 		/* input */
{
   double m[4][4];
   /* 
    * generate rotation matrix for rotation around principal axis;
    * note that angle is measured in radians (divide by 180, multiply by
    * PI to convert from degrees).
    */

   (void) urot_prince_d (m, theta, axis);
   MULTMATRIX_D (m);
}
#else

void rot_prince_f (float theta,		/* input */
                   char axis) 		/* input */
{
   float m[4][4];
   /* 
    * generate rotation matrix for rotation around principal axis;
    * note that angle is measured in radians (divide by 180, multiply by
    * PI to convert from degrees).
    */

   (void) urot_prince_f (m, theta, axis);
   MULTMATRIX_F (m);
}
#endif
#endif

/* ========================================================== */
