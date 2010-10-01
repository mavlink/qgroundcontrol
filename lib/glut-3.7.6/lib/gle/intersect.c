/*
 * FUNCTION:
 * This file contains a number of utilities useful to 3D graphics in
 * general, and to the generation of tubing and extrusions in particular
 * 
 * HISTORY:
 * Written by Linas Vepstas, August 1991
 */

#include "gutil.h"
#include "intersect.h"

/* ========================================================== */
/* 
 * The macro and subroutine INTERSECT are designed to compute the
 * intersection of a line (defined by the points v1 and v2) and a plane
 * (defined as plane which is normal to the vector n, and contains the
 * point p).  Both sect the array "sect", which is the point of
 * interesection.
 * 
 * The subroutine returns a value indicating if the specified inputs
 * represented a degenerate case. Valid is TRUE if the computed
 * intersection is valid, else it is FALSE.
 */


/* ========================================================== */

void intersect (gleDouble sect[3],	/* returned */
                gleDouble p[3],	/* input */
                gleDouble n[3],	/* input */
                gleDouble v1[3],	/* input */
                gleDouble v2[3])	/* input */
{
   INTERSECT (sect, p, n, v1, v2);
}

/* ========================================================== */
/* 
 * The macro and subroutine BISECTING_PLANE compute a normal vecotr that
 * describes the bisecting plane between three points (v1, v2 and v3).  
 * This bisecting plane has the following properties:
 * 1) it contains the point v2
 * 2) the angle it makes with v21 == v2 - v1 is equal to the angle it 
 *    makes with v32 == v3 - v2 
 * 3) it is perpendicular to the plane defined by v1, v2, v3.
 *
 * Having input v1, v2, and v3, it returns a vector n.
 * Note that n is NOT normalized (is NOT of unit length).
 * 
 * The subroutine returns a value indicating if the specified inputs
 * represented a degenerate case. Valid is TRUE if the computed
 * intersection is valid, else it is FALSE.
 */

int bisecting_plane (gleDouble n[3],	/* returned */
                      gleDouble v1[3],	/* input */
                      gleDouble v2[3],	/* input */
                      gleDouble v3[3])	/* input */
{
   int valid;

   BISECTING_PLANE (valid, n, v1, v2, v3);
   return (valid);
}

/* ========================================================== */
