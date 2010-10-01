/* ==========================================================================
                               FILEIO_C
=============================================================================

    FUNCTION NAMES

    read_polygon_indices        -- reads the polygon indices file.
    read_polygon_line		-- read the face polyline.
    read_muscles		-- reads the face muscles. 
    read_expression_vectors 	-- reads a vector of expressions.
    add_muscle_to_face 		-- add a muscle to the face.

    C SPECIFICATIONS

    read_polygon_indices	( FileName, face ) 
    read_polygon_line		( FileName, face )
    read_muscles		( FileName, face )
    read_expression_vectors	( FileName, face )
    add_muscle_to_face		( m, face )

    DESCRIPTION
	
	This module is responsible for reading the face data files.  
	This module comes as is with no warranties. 

    SIDE EFFECTS
	Unknown.
   
    HISTORY
	Created 16-Dec-94  Keith Waters at DEC's Cambridge Research Lab.
	Modified 22-Nov-96 Sing Bing Kang (sbk@crl.dec.com)
	   modified function read_expression_vectors() to allocate
	   memory to face->expression (done once)

============================================================================ */

#include <math.h>		/* C header for any math functions	     */
#include <stdio.h>		/* C header for standard I/O                 */
#include <string.h>		/* For String compare 			     */
#include <stdlib.h>
#ifndef _WIN32
#include <sys/types.h>
#include <sys/file.h>
#endif

/* 
 * from /usr/include/sys/types.h
 * Just in case TRUE and FALSE are not defined
 */

#ifndef TRUE
#define TRUE    1
#endif

#ifndef FALSE
#define FALSE   0
#endif


#include "head.h"		/* local header for the face data structure  */
#include "memory.h"

void add_muscle_to_face ( MUSCLE  *m , HEAD    *face );

/* ========================================================================= */  
/* read_polygon_indices                                                      */
/* ========================================================================= */  
/*
**   Read in the face data file (x,y,z)
**
*/

void
read_polygon_indices ( char *FileName, HEAD *face )
 { 
   FILE *InFile ;
   int i, ii ;

   /* 
    * Check the FileName.
    */
   if (( InFile = fopen ( FileName, "r" )) == 0 ) {
     fprintf ( stderr, "can't open input file: %s\n", FileName ) ;
     exit(-1) ;
   }
     
   fscanf ( InFile,"%d", &face->npindices ) ;

   /* 
    * Allocate some memory.
    */
   face->indexlist = ( int * ) malloc ( face->npindices*4 * sizeof ( int )) ; 

   for( i=0, ii=0; i<face->npindices; i++, ii+=4 )
     fscanf(InFile,"%d%d%d%d", 
	    &face->indexlist[ii],   &face->indexlist[ii+1], 
	    &face->indexlist[ii+2], &face->indexlist[ii+3] ) ;
        
   fclose( InFile ) ;
   
 }

/* ========================================================================= */  
/* read_polygon_line                                                         */
/* ========================================================================= */  
/*
**   Read in the face data file (x,y,z)
**
*/

void
read_polygon_line ( char *FileName, HEAD *face )
{
  FILE *InFile ;
  int i, ii ;

   /* 
    * Check the FileName.
    */
   if (( InFile = fopen ( FileName, "r" )) == 0 ) {
     fprintf ( stderr, "can't open input file: %s\n", FileName ) ;
     exit(-1) ;
   }

  fscanf ( InFile, "%d", &face->npolylinenodes ) ;

  /*
   * Allocate some memory.
   */
  face->polyline = ( float * ) malloc ( face->npolylinenodes*3 * sizeof ( float )) ; 

  for ( i=0, ii=0; i<face->npolylinenodes; i++, ii+=3 ) {
    
    fscanf ( InFile,"%f%f%f",
	    &face->polyline[ii], 
	    &face->polyline[ii+1], 
	    &face->polyline[ii+2] ) ;
  }
  
  fclose ( InFile ) ;

}

/* =============================================================
   read_muscles ( FileName, face )
   ========================================================== */
/*
** This function reads in the muscles.
**
*/

void
read_muscles ( char *FileName, HEAD *face )
{
  FILE *Infile;
  int i, nm ;
  MUSCLE *m ;

  /* 
   * Open the file to be read.
  */
  if((Infile = fopen(FileName,"r")) == 0) {
      fprintf(stderr,"Opening error on file:%10s\n", FileName) ;
      exit(0);
    }
  fscanf ( Infile, "%d", &nm ) ;

  for ( i=0; i < nm; i++ ) {

      m = _new ( MUSCLE ) ;

      fscanf (Infile, "%s %f %f %f %f %f %f %f %f %f %f",
	      &(*m->name),
	      &m->head[0], &m->head[1], &m->head[2],
	      &m->tail[0], &m->tail[1], &m->tail[2],
	      &m->fs, &m->fe, &m->zone, &m->clampv ) ;

      m->active = FALSE ;
      m->mstat  = 0.0 ;
      
      if (verbose) {
      fprintf(stderr,"%s: %d\n========================\nhx: %2.2f hy: %2.2f hz: %2.2f\ntx: %2.2f ty: %2.2f tz: %2.2f\n fall start: %2.2f\n fall end: %2.2f\n zone: %2.2f\n clampv: %2.2f mstat: %2.2f\n\n",
	     m->name, i, 
	     m->head[0], 
	     m->head[1], 
	     m->head[2],
	     m->tail[0], 
	     m->tail[1],
	     m->tail[2],
	     m->fs,
	     m->fe,
	     m->zone,
	     m->clampv,
	     m->mstat ) ;
      }

      add_muscle_to_face ( m, face ) ;

    }

  fclose(Infile) ;
}


/* ========================================================================= */  
/* read_expression_vectors                                                   */
/* ========================================================================= */  
/* sbk - added allocated var - 11/22/96 */

/*
**   Read in the expression vectors.
*/
void
read_expression_vectors ( char *FileName, HEAD *face )
{
  FILE *InFile ;
  int i, k ;
  EXPRESSION *e ;
  static int allocated = 0;

   /* 
    * Check the FileName.
    */
   if (( InFile = fopen ( FileName, "r" )) == 0 ) {
#if 0  /* Silently ignore the lack of expression vectors.  I never got the file. -mjk */
     fprintf ( stderr, "can't open input file: %s\n", FileName ) ;
#endif
     face->expression = NULL;
     return;
   }

  fscanf ( InFile, "%d", &face->nexpressions ) ;
  fprintf( stderr, "Number of expressions = %d\n", face->nexpressions ) ;

  /*
   * Allocate some memory.
   */
  if (!allocated)
    face->expression = (EXPRESSION  **)malloc( face->nexpressions*
					       sizeof(EXPRESSION  *) );
  
  for ( i=0; i<face->nexpressions; i++) {
    if (allocated)
      e = face->expression[i];
    else
      e = face->expression[i] = _new(EXPRESSION) ;

    fscanf ( InFile, "%s\n", &(*e->name) ) ;
    
    fprintf ( stderr, "%s\n", e->name ) ;
    
    for ( k=0; k < 17; k++) {
      
      fscanf ( InFile,(k==16) ? "%f\n" : "%f ",   &e->m[k]) ;
      fprintf (stderr,"%2.2f ", e->m[k] ) ;
    }
    fprintf (stderr, "\n") ;
  }
  
  fclose ( InFile ) ;

  allocated = 1;
}

/* =============================================================== 
   add_muscle_to_face ( m, face )
   =============================================================== */
/*
**   adds a muscle to the face muscle list.
**
*/

void
add_muscle_to_face ( MUSCLE  *m , HEAD    *face )
{
  int nn ;

  if(face->nmuscles == 0)
      face->muscle = _new_array(MUSCLE *, 50) ;
  else if(face->nmuscles % 50 == 0)
      face->muscle = _resize_array(face->muscle,MUSCLE *,face->nmuscles+50) ;

  nn = face->nmuscles ;
  face->muscle[nn] = m ;

  face->nmuscles++ ;

}

