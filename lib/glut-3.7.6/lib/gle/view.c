
/*
 * MODULE Name: view.c
 *
 * FUNCTION:
 * This module provides two different routines that compute and return
 * viewing matrices.  Both routines take a direction and an up vector, 
 * and return a matrix that transforms the direction to the z-axis, and
 * the up-vector to the y-axis.
 * 
 * HISTORY:
 * written by Linas Vepstas August 1991
 * Added double precision interface, March 1993, Linas
 */

#include <math.h>
#include "rot.h"
#include "gutil.h"
#include "vvector.h"

/* ============================================================ */
/*
 * The uviewdirection subroutine computes and returns a 4x4 rotation
 * matrix that puts the negative z axis along the direction v21 and 
 * puts the y axis along the up vector.
 *
 * Note that this code is fairly tolerant of "weird" paramters.
 * It normalizes when necessary, it does nothing when vectors are of
 * zero length, or are co-linear.  This code shouldn't croak, no matter
 * what the user sends in as arguments.
 */
#ifdef __GUTIL_DOUBLE
void uview_direction_d (double m[4][4],		/* returned */
                        double v21[3],		/* input */
                        double up[3])		/* input */
#else
void uview_direction_f (float m[4][4],		/* returned */
                        float v21[3],		/* input */
                        float up[3])		/* input */
#endif
{
   double amat[4][4];
   double bmat[4][4];
   double cmat[4][4];
   double v_hat_21[3];
   double v_xy[3];
   double sine, cosine;
   double len;
   double up_proj[3];
   double tmp[3];

   /* find the unit vector that points in the v21 direction */
   VEC_COPY (v_hat_21, v21);    
   VEC_LENGTH (len, v_hat_21);
   if (len != 0.0) {
      len = 1.0 / len;
      VEC_SCALE (v_hat_21, len, v_hat_21);

      /* rotate z in the xz-plane until same latitude */
      sine = sqrt ( 1.0 - v_hat_21[2] * v_hat_21[2]);
      ROTY_CS (amat, (-v_hat_21[2]), (-sine));

   } else {

      /* error condition: zero length vecotr passed in -- do nothing */
      IDENTIFY_MATRIX_4X4 (amat);
   }


   /* project v21 onto the xy plane */
   v_xy[0] = v21[0];
   v_xy[1] = v21[1];
   v_xy[2] = 0.0;
   VEC_LENGTH (len, v_xy);

   /* rotate in the x-y plane until v21 lies on z axis ---
    * but of course, if its already there, do nothing */
   if (len != 0.0) { 

      /* want xy projection to be unit vector, so that sines/cosines pop out */
      len = 1.0 / len;
      VEC_SCALE (v_xy, len, v_xy);

      /* rotate the projection of v21 in the xy-plane over to the x axis */
      ROTZ_CS (bmat, v_xy[0], v_xy[1]);

      /* concatenate these together */
      MATRIX_PRODUCT_4X4 (cmat, amat, bmat);

   } else {

      /* no-op -- vector is already in correct position */
      COPY_MATRIX_4X4 (cmat, amat);
   }

   /* up vector really should be perpendicular to the x-form direction --
    * Use up a couple of cycles, and make sure it is, 
    * just in case the user blew it.
    */
   VEC_PERP (up_proj, up, v_hat_21); 
   VEC_LENGTH (len, up_proj);
   if (len != 0.0) {

      /* normalize the vector */
      len = 1.0/len;
      VEC_SCALE (up_proj, len, up_proj);
   
      /* compare the up-vector to the  y-axis to get the cosine of the angle */
      tmp [0] = cmat [1][0];
      tmp [1] = cmat [1][1];
      tmp [2] = cmat [1][2];
      VEC_DOT_PRODUCT (cosine, tmp, up_proj);
   
      /* compare the up-vector to the x-axis to get the sine of the angle */
      tmp [0] = cmat [0][0];
      tmp [1] = cmat [0][1];
      tmp [2] = cmat [0][2];
      VEC_DOT_PRODUCT (sine, tmp, up_proj);
   
      /* rotate to align the up vector with the y-axis */
      ROTZ_CS (amat, cosine, -sine);
   
      /* This xform, although computed last, acts first */
      MATRIX_PRODUCT_4X4 (m, amat, cmat);

   } else {

      /* error condition: up vector is indeterminate (zero length) 
       * -- do nothing */
      COPY_MATRIX_4X4 (m, cmat);
   }
}

/* ============================================================ */
#ifdef __STALE_CODE
/*
 * The uview_dire subroutine computes and returns a 4x4 rotation
 * matrix that puts the negative z axis along the direction v21 and 
 * puts the y axis along the up vector.
 * 
 * It computes exactly the same matrix as the code above
 * (uview_direction), but with an entirely different (and slower)
 * algorithm.
 *
 * Note that the code below is slightly less robust than that above --
 * it may croak if the supplied vectors are of zero length, or are
 * parallel to each other ... 
 */
void uview_dire (float m[4][4],		/* returned */
                 float v21[3],		/* input */
                 float up[3])		/* input */
{
   double theta;
   float v_hat_21 [3];
   float z_hat [3];
   float v_cross_z [3];
   float u[3];
   float y_hat [3];
   float u_cross_y [3];
   double cosine;
   float zmat [4][4];
   float upmat[4][4];
   float dot;

   /* perform rotation to z-axis only if not already 
    * pointing down z */
   if ((v21[0] != 0.0 ) || (v21[1] != 0.0)) {

      /* find the unit vector that points in the v21 direction */
      VEC_COPY (v_hat_21, v21);    
      VEC_NORMALIZE (v_hat_21);
   
      /* cosine theta equals v_hat dot z_hat */
      cosine = - v_hat_21 [2];
      theta = - acos (cosine);
   
      /* Take cros product with z -- we need this, because we will rotate
       * about this axis */
      z_hat[0] = 0.0;
      z_hat[1] = 0.0;
      z_hat[2] = -1.0;
   
      VEC_CROSS_PRODUCT (v_cross_z, v_hat_21, z_hat);
      VEC_NORMALIZE (v_cross_z);
   
      /* compute rotation matrix that takes -z axis to the v21 axis */
      urot_axis (zmat, (float) theta, v_cross_z);

   } else {

      IDENTIFY_MATRIX_4X4 (zmat);
      if (v21[2] > 0.0) {
         /* if its pointing down the positive z-axis, flip it, so that
          * we point down negative z-axis.  We flip x so that the partiy
          * isn't destroyed (looks like a rotation)
          */
         zmat[0][0] = -1.0;
         zmat[2][2] = -1.0;
      }
   }
   
   /* --------------------- */
   /* OK, now compute the part that takes the y-axis to the up vector */

   VEC_COPY (u, up);
   /* the rotation blows up, if the up vector is not perpendicular to
    * the v21 vector.  Let us make sure that this is so. */
   VEC_PERP (u, u, v_hat_21);

   /* need to run the y axis through above x-form, to see where it went */
   y_hat[0] = zmat [1][0];
   y_hat[1] = zmat [1][1];
   y_hat[2] = zmat [1][2];
   
   /* perform rotation to up-axis only if not already 
    * pointing along y axis */
   VEC_DOT_PRODUCT (dot, y_hat, u);
   if ((-1.0 < dot) && (dot < 1.0))  {

      /* make sure that up really is a unit vector */
      VEC_NORMALIZE (u);
      /* cosine phi equals y_hat dot up_vec */
      VEC_DOT_PRODUCT (cosine, u, y_hat);
      theta = - acos (cosine);
   
      /* Take cross product with y */
      VEC_CROSS_PRODUCT (u_cross_y, u, y_hat);
      VEC_NORMALIZE (u_cross_y);
   
      /* As a matter of fact, u_cross_y points either in the v21 direction,
       * or in the minus v21 direction.  In either case, we needed to compute 
       * it, because the the arccosine function returns values only for 
       * 0 to 180 degree, not 0 to 360, which is what we need.  The 
       * cross-product helps us make up for this.
       */
      /* rotate about the NEW z axis (i.e. v21) by the cosine */
      urot_axis (upmat, (float) theta, u_cross_y);

   } else {

      IDENTIFY_MATRIX_4X4 (upmat);
      if (dot == -1.0) {
         /* if its pointing along the negative y-axis, flip it, so that
          * we point along the positive y-axis.  We flip x so that the partiy
          * isn't destroyed (looks like a rotation)
          */
         upmat[0][0] = -1.0;
         upmat[1][1] = -1.0;
      }
   }
   
   MATRIX_PRODUCT_4X4 (m, zmat, upmat);

}
#endif /* __STALE_CODE */

/* ============================================================ */
/*
 * The uviewpoint subroutine computes and returns a 4x4 matrix that 
 * translates the origen to the point v1, puts the negative z axis
 * along the direction v21==v2-v1, and puts the y axis along the up
 * vector.
 */
#ifdef __GUTIL_DOUBLE
void uviewpoint_d (double m[4][4],		/* returned */
                   double v1[3],		/* input */
                   double v2[3],		/* input */
                   double up[3])		/* input */
#else 
void uviewpoint_f (float m[4][4],		/* returned */
                   float v1[3],		/* input */
                   float v2[3],		/* input */
                   float up[3])		/* input */
#endif
{
#ifdef __GUTIL_DOUBLE
   double v_hat_21 [3];
   double trans_mat[4][4];
   double rot_mat[4][4];
#else
   float v_hat_21 [3];
   float trans_mat[4][4];
   float rot_mat[4][4];
#endif

   /* find the vector that points in the v21 direction */
   VEC_DIFF (v_hat_21, v2, v1);

   /* compute rotation matrix that takes -z axis to the v21 axis,
    * and y to the up dierction */
#ifdef __GUTIL_DOUBLE
   uview_direction_d (rot_mat, v_hat_21, up);
#else
   uview_direction_f (rot_mat, v_hat_21, up);
#endif

   /* build matrix that translates the origin to v1 */
   IDENTIFY_MATRIX_4X4 (trans_mat);
   trans_mat[3][0] = v1[0];
   trans_mat[3][1] = v1[1];
   trans_mat[3][2] = v1[2];

   /* concatenate the matrices together */
   MATRIX_PRODUCT_4X4 (m, rot_mat, trans_mat);

}

/* ================== END OF FILE ============================ */
