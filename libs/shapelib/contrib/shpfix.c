/******************************************************************************
 * Copyright (c) 1999, Carl Anderson
 *
 * This code is based in part on the earlier work of Frank Warmerdam
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 ******************************************************************************
 * shpfix
 *
 *
 *  gcc -c shpfix.c shpopen.c -o shpfix
 *
 *  Utility program to fix nulls and inconsistencies in Shapefiles
 *  as happens from time to time
 *
 *  Simply load and rewrite each record, parameter fixrex allow user to null 
 *  a particularly nasty record if needed
 *
 */

#include <stdlib.h>
#include <string.h>
#include "shapefil.h"

int main( int argc, char ** argv )

{
    SHPHandle	hSHP, cSHP;
    int		nShapeType, cShapeType, nEntities, i;
    double	adBounds[4];
    SHPObject	*shape;
    int		fix_rec;	

/* -------------------------------------------------------------------- */
/*      Display a usage message.                                        */
/* -------------------------------------------------------------------- */
    if( argc <= 3 )
    {
	printf( "shpfix shpfile new_file <Record# to Blank>\n" );
	exit( 1 );
    }

   fix_rec = atoi (argv[3]);
   fix_rec --;

/* -------------------------------------------------------------------- */
/*      Open the passed shapefile.                                      */
/* -------------------------------------------------------------------- */
    hSHP = SHPOpen( argv[1], "rb+" );

    if( hSHP == NULL )
    {
	printf( "Unable to open:%s\n", argv[1] );
	exit( 1 );
    }

    SHPGetInfo( hSHP, &nEntities, &nShapeType, NULL, NULL );


/* -------------------------------------------------------------------- */
/*      Open the passed shapefile.                                      */
/* -------------------------------------------------------------------- */
    cSHP = SHPCreate( argv[2], nShapeType );

    if( cSHP == NULL )
    {
	printf( "Unable to open:%s\n", argv[2] );
	exit( 1 );
    }

    SHPGetInfo( cSHP, NULL, &cShapeType, &(adBounds[0]), &(adBounds[2]) );


/* -------------------------------------------------------------------- */
/*	Skim over the list of shapes, printing all the vertices.	*/
/* -------------------------------------------------------------------- */

    for( i = 0; i < nEntities; i++ )
    {

        shape = SHPReadObject( hSHP, i );
        if ( i == fix_rec ) 
          {  shape->nParts = 0;
             shape->nVertices = 0;
          }
        SHPWriteObject( cSHP, -1, shape );          
        SHPDestroyObject ( shape );

    }
    

    SHPClose ( hSHP );
    SHPClose ( cSHP );
}
