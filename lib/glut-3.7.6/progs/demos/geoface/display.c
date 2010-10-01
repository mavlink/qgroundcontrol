/* ==========================================================================
                            DISPLAY_C
=============================================================================

    FUNCTION NAMES

    void paint_muscles 			-- displays the muscles.
    void paint_polyline 		-- paint the polyline.
    paint_polygons			-- display the polygons.
    calculate_polygon_vertex_normal 	-- calculate the vertex norms.
    calc_normal 			-- calculate a normal.

    C SPECIFICATIONS

    void paint_muscles 			( HEAD *face )
    void paint_polyline 		( HEAD *face )
    paint_polygons      		( HEAD *face, int type, int normals )
    calculate_polygon_vertex_normal 	( HEAD *face ) 
    calc_normal 			( float *p1, float *p2, float *p3, 
                                          float *norm )

    DESCRIPTION
    
    This module is responsible for displaying the face geometry.
    This module comes as is with no warranties.  

    HISTORY
	16-Dec-94  Keith Waters (waters) at DEC's Cambridge Research Lab
	Created.

============================================================================ */

#include <math.h>           /* C header for any math functions               */
#include <stdio.h>          /* C header for standard I/O                     */
#include <string.h>         /* For String compare                            */
#include <stdlib.h>
#ifndef _WIN32
#include <sys/types.h>
#include <sys/file.h>
#endif
#include <GL/glut.h>	    /* OpenGl headers				     */

#include "head.h"	   /* local header for the face		             */

void calc_normal ( float *p1, float *p2, float *p3, float *norm );

/* ========================================================================= */  
/* paint_muscles                                                             */
/* ========================================================================= */  
/*
** Displays the face muscles.
**
*/

#define PAINT_MUSCLES_DEBUG 0
void paint_muscles ( HEAD *face )
{
  int i,j;
  float v1[3], v2[3] ;

  glLineWidth   ( 3.0 ) ;
  glColor3f     ( 100.0, 200.0, 200.0 ) ;

    for ( i=0; i<face->nmuscles; i++ ) {
    
      for (j=0; j<3; j++) {
      v1[j] = face->muscle[i]->head[j] ;
      v2[j] = face->muscle[i]->tail[j] ;
    }
      
#if PAINT_MUSCLES_DEBUG
    fprintf (stderr, "head x: %f y: %f z: %f\n",   v1[0], v1[1], v1[2] ) ;
    fprintf (stderr, "tail x: %f y: %f z: %f\n\n", v2[0], v2[1], v2[2] ) ;
#endif

      glBegin ( GL_LINE_STRIP ) ;
      glVertex3f ( v1[0], v1[1], v1[2] ) ;
      glVertex3f ( v2[0], v2[1], v2[2] ) ;
      glEnd ( ) ;
  }
  glLineWidth   ( 1.0 ) ;
}

/* ========================================================================= */  
/* paint_polyline                                                            */
/* ========================================================================= */  
/*
** Displays the polyline.
**
*/

void paint_polyline ( HEAD *face )
{
  int i,j,cnt ;
  float v1[3] ;
  static float r ;

  glClear    	( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT ) ;
  glLineWidth   ( 1.0 ) ;
  glColor3f     ( 100.0, 100.0, 0.0 ) ;
  
  glPushMatrix  ( ) ;
  glRotatef	( r, 1.0, 1.0, 1.0 ) ;

  glBegin ( GL_LINE_STRIP ) ;
    for (cnt=0, i=0; i<face->npolylinenodes; i++ ) {
    
    for (j=0; j<3; j++, cnt++) 
      v1[j] = face->polyline[cnt] ;

#if PAINT_POLYLINE_DEBUG
    printf ("x: %f y: %f z: %f\n", v1[0], v1[1], v1[2] ) ;
#endif

      glVertex3f ( v1[0], v1[1], v1[2] ) ;
  }
  
  glEnd ( ) ;

  glPopMatrix	( ) ;
  glFlush	( ) ;
  
}


/* ========================================================================= */  
/* paint_polygons                                                            */
/* ========================================================================= */  
/*
** Paints the polygons of the face. 
** Type indicates if they are to be 
** drawn         (type=0), 
** flat shaded   (type=1),
** smooth shaded (type=2).
*/

void paint_polygons ( HEAD *face, int type, int normals )
{
  int    i, j ;
  float  v1[3], v2[3], v3[3] ;
  float  norm1[3], norm2[3], norm3[3] ;
  float  vn1[3], vn2[3], vn3[3] ;

  glLineWidth   ( 2.0 ) ;  

  for (i=0; i<face->npolygons; i++ )
    {
      for (j=0; j<3; j++) {
	v1[j] = face->polygon[i]->vertex[0]->xyz[j] ;
	v2[j] = face->polygon[i]->vertex[1]->xyz[j] ;
	v3[j] = face->polygon[i]->vertex[2]->xyz[j] ;
      }

      if ( type == 0 ) {
	
	for (j=0; j<3; j++) {
	  norm1[j] = face->polygon[i]->vertex[0]->norm[j] ;
	  norm2[j] = face->polygon[i]->vertex[1]->norm[j] ;
	  norm3[j] = face->polygon[i]->vertex[2]->norm[j] ;
	}
	glBegin ( GL_LINE_LOOP ) ; {
	  glNormal3f ( norm1[0], norm1[1], norm1[2] ) ;
	  glVertex3f ( v1[0],    v1[1],    v1[2]    ) ;
	  glNormal3f ( norm2[0], norm2[1], norm2[2] ) ;
	  glVertex3f ( v2[0],    v2[1],    v2[2]    ) ;
	  glNormal3f ( norm3[0], norm3[1], norm3[2] ) ;
	  glVertex3f ( v3[0],    v3[1],    v3[2]    ) ;
	}  glEnd ( ) ;

      } /* end if drawn */

      if ( type == 1 ) {
	
	for (j=0; j<3; j++) {
	  norm1[j] = face->polygon[i]->vertex[0]->norm[j] ;
	  norm2[j] = face->polygon[i]->vertex[1]->norm[j] ;
	  norm3[j] = face->polygon[i]->vertex[2]->norm[j] ;
	}
	glBegin ( GL_TRIANGLES ) ; {
	  glNormal3f ( norm1[0], norm1[1], norm1[2] ) ;
	  glVertex3f ( v1[0],    v1[1],    v1[2]    ) ;
	  glNormal3f ( norm2[0], norm2[1], norm2[2] ) ;
	  glVertex3f ( v2[0],    v2[1],    v2[2]    ) ;
	  glNormal3f ( norm3[0], norm3[1], norm3[2] ) ;
	  glVertex3f ( v3[0],    v3[1],    v3[2]    ) ;
	}  glEnd ( ) ;

      } /* end if drawn */


      else if ( type == 1) {
	for (j=0; j<3; j++) {
	  norm1[j] = face->polygon[i]->vertex[0]->norm[j] ;
	  norm2[j] = face->polygon[i]->vertex[1]->norm[j] ;
	  norm3[j] = face->polygon[i]->vertex[2]->norm[j] ;
	}
      } /* end if flat */

      else if ( type == 2 ) {

	averaged_vertex_normals ( face, i, norm1, norm2, norm3 ) ;

      } /* end if smoothed */

      if ( type ) {

	glBegin ( GL_TRIANGLES ) ; {
	  glNormal3f ( norm1[0], norm1[1], norm1[2] ) ;
	  glVertex3f ( v1[0],    v1[1],    v1[2]    ) ;
	  glNormal3f ( norm2[0], norm2[1], norm2[2] ) ;
	  glVertex3f ( v2[0],    v2[1],    v2[2]    ) ;
	  glNormal3f ( norm3[0], norm3[1], norm3[2] ) ;
	  glVertex3f ( v3[0],    v3[1],    v3[2]    ) ;
	}  glEnd ( ) ;
      } /* endif painted */

      if ( normals ) {
	for (j=0; j<3; j++) {
	  vn1[j] = face->polygon[i]->vertex[0]->xyz[j] + norm1[j] ;
	  vn2[j] = face->polygon[i]->vertex[1]->xyz[j] + norm2[j] ;
	  vn3[j] = face->polygon[i]->vertex[2]->xyz[j] + norm3[j] ;
	}

	glBegin ( GL_LINE_STRIP ) ; {
	  glVertex3f ( v1[0],    v1[1],    v1[2]     ) ;
	  glVertex3f ( vn1[0],  vn1[1],    vn1[2]    ) ;
	}  glEnd ( ) ;


	glBegin ( GL_LINES ) ; {
	  glVertex3f ( v2[0],    v2[1],    v2[2]     ) ;
	  glVertex3f ( vn2[0],  vn2[1],    vn2[2]    ) ;
	}  glEnd ( ) ;


	glBegin ( GL_LINES ) ; {
	  glVertex3f ( v3[0],    v3[1],    v3[2]     ) ;
	  glVertex3f ( vn3[0],  vn3[1],    vn3[2]    ) ;
	}  glEnd ( ) ;

      }
    }
  glLineWidth   ( 1.0 ) ;  
} 

/* ========================================================================= */  
/* calculate_polygon_vertex_normal.					     */
/* ========================================================================= */
/*
** As it says.
*/

void
calculate_polygon_vertex_normal ( HEAD *face ) 
{
  int i,j,k ;
  float p1[3], p2[3], p3[3] ;
  float norm[3] ;
  for (i=0; i<face->npolygons; i++ )
    {
      for (j=0; j<3; j++) 
	p1[j] = face->polygon[i]->vertex[0]->xyz[j] ;
      for (j=0; j<3; j++) 
	p2[j] = face->polygon[i]->vertex[1]->xyz[j] ;
      for (j=0; j<3; j++) 
	p3[j] = face->polygon[i]->vertex[2]->xyz[j] ;

      calc_normal ( p1, p2, p3, norm ) ;

      for (j=0; j<3; j++) 
	for (k=0; k<3; k++)
	  face->polygon[i]->vertex[j]->norm[k] = norm[k] ;
    }
}

/* ========================================================================= */  
/* calc_normal.					     			     */
/* ========================================================================= */
/*
** Calculates the normal vector from three vertices.
*/
void
calc_normal ( float *p1, float *p2, float *p3, float *norm )
{
  float coa, cob, coc ;
  float px1, py1, pz1 ;
  float px2, py2, pz2 ;
  float px3, py3, pz3 ;
  
  float absvec ;
  
  px1 = p1[0] ;
  py1 = p1[1] ;
  pz1 = p1[2] ;
  
  px2 = p2[0] ;
  py2 = p2[1] ;
  pz2 = p2[2] ;
  
  px3 = p3[0] ;
  py3 = p3[1] ;
  pz3 = p3[2] ;
  
  coa = -(py1 * (pz2-pz3) + py2*(pz3-pz1) + py3*(pz1-pz2)) ;
  cob = -(pz1 * (px2-px3) + pz2*(px3-px1) + pz3*(px1-px2)) ;
  coc = -(px1 * (py2-py3) + px2*(py3-py1) + px3*(py1-py2)) ;
  
  absvec = sqrt ((double) ((coa*coa) + (cob*cob) + (coc*coc))) ;
  
  norm[0] = coa/absvec ;
  norm[1] = cob/absvec ;
  norm[2] = coc/absvec ;
}
