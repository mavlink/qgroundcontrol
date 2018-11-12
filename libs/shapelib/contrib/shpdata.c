/******************************************************************************
 * Copyright (c) 1999, Carl Anderson
 *
 * this code is based in part on the earlier work of Frank Warmerdam
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
 *
 * shpdata.c  - utility program for testing elements of the libraries
 *
 *
 * $Log: shpdata.c,v $
 * Revision 1.3  2016-12-05 12:44:07  erouault
 * * Major overhaul of Makefile build system to use autoconf/automake.
 *
 * * Warning fixes in contrib/
 *
 * Revision 1.2  1999-05-26 02:56:31  candrsn
 * updates to shpdxf, dbfinfo, port from Shapelib 1.1.5 of dbfcat and shpinfo
 *
 *
 * 
 */
      
#include <stdlib.h>
#include "shapefil.h"
#include "shpgeo.h"

int main( int argc, char ** argv )

{
    SHPHandle	old_SHP, new_SHP;
    DBFHandle   old_DBF, new_DBF;
    int		nShapeType, nEntities, nVertices, nParts, *panParts, i, iPart;
    double	*padVertices, adBounds[4];
    const char 	*pszPlus;
    DBFFieldType  idfld_type;
    int		idfld, nflds;
    char	kv[257] = "";
    char	idfldName[120] = "";
    char	fldName[120] = "";
    char	shpFileName[120] = "";
    char	dbfFileName[120] = "";
    char	*DBFRow = NULL;
    int		Cpan[2] = { 0,0 };
    int		byRing = 1;
    PT		oCentrd, ringCentrd;
    SHPObject	*psCShape, *cent_pt;
    double	oArea = 0.0, oLen = 0.0;

    if( argc < 2 )
    {
	printf( "shpdata shp_file \n" );
	exit( 1 );
    }
    
    old_SHP = SHPOpen (argv[1], "rb" );
    old_DBF = DBFOpen (argv[1], "rb");
    if( old_SHP == NULL || old_DBF == NULL )
    {
	printf( "Unable to open old files:%s\n", argv[1] );
	exit( 1 );
    }

    SHPGetInfo( old_SHP, &nEntities, &nShapeType, NULL, NULL );
    for( i = 0; i < nEntities; i++ )
    {
	int		res ;

	psCShape = SHPReadObject( old_SHP, i );

        if ( byRing == 1 ) {
          int 	   ring, prevStart, ringDir;
	  double   ringArea;

          prevStart = psCShape->nVertices;
          for ( ring = (psCShape->nParts - 1); ring >= 0; ring-- ) {
	    SHPObject 	*psO;
	    int		j, numVtx, rStart;
            
            rStart = psCShape->panPartStart[ring];
            if ( ring == (psCShape->nParts -1) )
              { numVtx = psCShape->nVertices - rStart; }
             else
              { numVtx = psCShape->panPartStart[ring+1] - rStart; }
              
            printf ("(shpdata) Ring(%d) (%d for %d) \n", ring, rStart, numVtx);              
	    psO = SHPClone ( psCShape, ring,  ring + 1 );

            ringDir = SHPRingDir_2d ( psO, 0 );
            ringArea = RingArea_2d (psO->nVertices,(double*) psO->padfX,
            	 (double*) psO->padfY);
            RingCentroid_2d ( psO->nVertices, (double*) psO->padfX, 
     		(double*) psO->padfY, &ringCentrd, &ringArea);  

            	 
            printf ("(shpdata)  Ring %d, %f Area %d dir \n", 
           	ring, ringArea, ringDir );

	    SHPDestroyObject ( psO );
            printf ("(shpdata) End Ring \n");
           }  /* (ring) [0,nParts  */

          }  /* by ring   */

	   oArea = SHPArea_2d ( psCShape );
	   oLen = SHPLength_2d ( psCShape ); 
	   oCentrd = SHPCentrd_2d ( psCShape );
           printf ("(shpdata) Part (%d) %f Area  %f length, C (%f,%f)\n",
           	 i, oArea, oLen, oCentrd.x, oCentrd.y );
    }

    SHPClose( old_SHP );

    DBFClose( old_DBF );

    printf ("\n");
}
