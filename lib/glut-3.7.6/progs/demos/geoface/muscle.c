/* ==========================================================================
                               MUSCLE_C
=============================================================================

    FUNCTION NAMES

    float VecLen 	-- calculates a vector length.
    float CosAng 	-- compute the cosine angle between two vectors.
    activate_muscle     -- activate a muscle.
    act_muscles 	-- activate muscles.
    reset_muscles 	-- reset all the muscles.

    C SPECIFICATIONS

    float VecLen 	( float *v )
    float CosAng 	( float *v1, float *v2 )
    activate_muscle 	( HEAD *face, float *vt, float *vh, 
                          float fstart, float fin, float ang, float val )
    act_muscles 	( HEAD *face ) 
    reset_muscles 	( HEAD *face ) 

    DESCRIPTION
	
	This module is where all the muscle action takes place.  This module 
	comes as is with no warranties.  

    SIDE EFFECTS
	Unknown.
   
    HISTORY
	Created 16-Dec-94  Keith Waters at DEC's Cambridge Research Lab.

============================================================================ */

#include <math.h>
#include <stdio.h>
#include "head.h"

#ifdef _WIN32
#pragma warning (disable:4244)	/* Disable bogus conversion warnings. */
#pragma warning (disable:4305)  /* VC++ 5.0 version of above warning. */
#endif

/* Some <math.h> files do not define M_PI... */
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#define DTOR(deg) ((deg)*0.017453292)	/* degrees to radians   */
#define RTOD(rad) ((rad)*57.29577951)   /* radians to degrees   */
#define RADF 180.0 / M_PI 


/* ======================================================================== */
/* float VecLen ( vec )							    */
/* ======================================================================== */
/*
** Caculates the length of the vector.
*/

float VecLen ( float *v )
{
  return (float) sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
}

/* ======================================================================== */
/* float CosAng ( v1, v2 )						    */
/* ======================================================================== */
/*
** Rotates the facial muscles 90.0 degrees.
*/

float CosAng ( float *v1, float *v2 )
{
  float ang, a,b ;
  
  a = VecLen ( v1 ) ;
  b = VecLen ( v2 ) ;

  ang = ((v1[0]*v2[0]) + (v1[1]*v2[1] ) + (v1[2]*v2[2])) / (a*b) ;

  return ( ang ) ;
}

/* ======================================================================== */
/* activate_muscle ( face, vt, vh, fstart, fin, ang, val )		    */
/* ======================================================================== */
/*
** activate the muscle.
*/

void
activate_muscle (HEAD *face, float *vt, float *vh, float fstart,  float fin,  float ang,  float val)
{
  float newp[3], va[3], vb[3] ;
  int i,j,k,l ;
  float valen, vblen ;
  float cosa, cosv, dif, tot, percent, thet, newv, the, radf ;
  
  radf  = 180.0/ M_PI ;
  the   = ang / radf ; ;
  thet  = cos ( the ) ;

  cosa = 0.0 ;

  /* find the length of the muscle */
  for (i=0; i<3; i++)
    va[i] = vt[i] - vh[i] ;
  valen = VecLen ( va ) ;

  /* loop for all polygons */
  for (i=0; i<face->npolygons; i++) {

    /* loop for all vertices */
    for (j=0; j<3; j++) {

      /* find the length of the muscle head to the mesh node */
      for (k=0; k<3; k++)
	vb[k] = face->polygon[i]->vertex[j]->xyz[k] - vh[k] ;
      vblen = VecLen ( vb ) ;

      if ( valen > 0.0 && vblen > 0.0) {
	cosa = CosAng ( va, vb ) ;

	if ( cosa >= thet ) {
	  if ( vblen <= fin ) {
	    cosv = val * ( 1.0 - (cosa/thet) ) ;

	    if ( vblen >= fstart && vblen <= fin) {
	      dif       = vblen - fstart ;
	      tot       = fin - fstart ;
	      percent   = dif/tot ;
	      newv      = cos ( DTOR(percent*90.0) ) ;

	      for ( l=0; l<3; l++)
		newp[l] = (vb[l] * cosv) * newv ;
	    }
	    else {
	      for ( l=0; l<3; l++)
		newp[l] = vb[l] * cosv ;

	    }   /* endif vblen>fin */

	    for (l=0; l<3; l++)
	      face->polygon[i]->vertex[j]->xyz[l] += newp[l] ;
		  
	  }  /* endif vblen>fin    */
	}   /* endif cosa>thet    */
      }    /* endif mlen&&tlen   */
    }     /* end for j vertices */
  }      /* end for i polygon  */
}

/* ======================================================================== */
/* act_muscles ( face ) 						    */
/* ======================================================================== */
/*
** activate the muscles
*/

void
act_muscles ( HEAD *face ) 
{
  int i ;

  /* 
   * Loop all the muscles.             
   */ 
  for (i=0; i<face->nmuscles; i++) {

    /*
     * Check to see if the muscle is active.                        
    */
    if (face->muscle[i]->active) {

      activate_muscle ( face, face->muscle[i]->head,
		              face->muscle[i]->tail,
		              face->muscle[i]->fs,
		              face->muscle[i]->fe,
		              face->muscle[i]->zone,
 		              face->muscle[i]->mval ) ;

      /* 
      * Reset the muscle activity.     
      */
      face->muscle[i]->active = 1 ;

    }
  } 
}

/* ======================================================================== */
/* reset_muscles ( face ) 						    */
/* ======================================================================== */
/*
** Resets the muscles of the face.  This is achieved by reversing 
** the muscle contraction.
*/

void
reset_muscles ( HEAD *face ) 
{
  int i,j,k ;

  for ( i=0; i<face->npolygons; i++ ) {
    for ( j=0; j<3; j++ ) {

      for ( k=0; k<3; k++ )
	face->polygon[i]->vertex[j]->xyz[k] = 
	  face->polygon[i]->vertex[j]->nxyz[k] ;

    } /* end for j */
  } /* end for i */
} 
