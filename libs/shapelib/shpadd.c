/******************************************************************************
 * $Id: shpadd.c,v 1.18 2016-12-05 12:44:05 erouault Exp $
 *
 * Project:  Shapelib
 * Purpose:  Sample application for adding a shape to a shapefile.
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *
 ******************************************************************************
 * Copyright (c) 1999, Frank Warmerdam
 *
 * This software is available under the following "MIT Style" license,
 * or at the option of the licensee under the LGPL (see COPYING).  This
 * option is discussed in more detail in shapelib.html.
 *
 * --
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
 * $Log: shpadd.c,v $
 * Revision 1.18  2016-12-05 12:44:05  erouault
 * * Major overhaul of Makefile build system to use autoconf/automake.
 *
 * * Warning fixes in contrib/
 *
 * Revision 1.17  2016-12-04 15:30:15  erouault
 * * shpopen.c, dbfopen.c, shptree.c, shapefil.h: resync with
 * GDAL Shapefile driver. Mostly cleanups. SHPObject and DBFInfo
 * structures extended with new members. New functions:
 * DBFSetLastModifiedDate, SHPOpenLLEx, SHPRestoreSHX,
 * SHPSetFastModeReadObject
 *
 * * sbnsearch.c: new file to implement original ESRI .sbn spatial
 * index reading. (no write support). New functions:
 * SBNOpenDiskTree, SBNCloseDiskTree, SBNSearchDiskTree,
 * SBNSearchDiskTreeInteger, SBNSearchFreeIds
 *
 * * Makefile, makefile.vc, CMakeLists.txt, shapelib.def: updates
 * with new file and symbols.
 *
 * * commit: helper script to cvs commit
 *
 * Revision 1.16  2010-06-21 20:41:52  fwarmerdam
 * reformat white space
 *
 * Revision 1.15  2007-12-30 16:57:32  fwarmerdam
 * add support for z and m
 *
 * Revision 1.14  2004/09/26 20:09:35  fwarmerdam
 * avoid rcsid warnings
 *
 * Revision 1.13  2002/01/15 14:36:07  warmerda
 * updated email address
 *
 * Revision 1.12  2001/05/31 19:35:29  warmerda
 * added support for writing null shapes
 *
 * Revision 1.11  2000/07/07 13:39:45  warmerda
 * removed unused variables, and added system include files
 *
 * Revision 1.10  2000/05/24 15:09:22  warmerda
 * Added logic to graw vertex lists of needed.
 *
 * Revision 1.9  1999/11/05 14:12:04  warmerda
 * updated license terms
 *
 * Revision 1.8  1998/12/03 16:36:26  warmerda
 * Use r+b rather than rb+ for binary access.
 *
 * Revision 1.7  1998/11/09 20:57:04  warmerda
 * Fixed SHPGetInfo() call.
 *
 * Revision 1.6  1998/11/09 20:19:16  warmerda
 * Changed to use SHPObject based API.
 *
 * Revision 1.5  1997/03/06 14:05:02  warmerda
 * fixed typo.
 *
 * Revision 1.4  1997/03/06 14:01:16  warmerda
 * added memory allocation checking, and free()s.
 *
 * Revision 1.3  1995/10/21 03:14:37  warmerda
 * Changed to use binary file access
 *
 * Revision 1.2  1995/08/04  03:18:01  warmerda
 * Added header.
 *
 */

#include <stdlib.h>
#include <string.h>
#include "shapefil.h"

SHP_CVSID("$Id: shpadd.c,v 1.18 2016-12-05 12:44:05 erouault Exp $")

int main( int argc, char ** argv )

{
    SHPHandle	hSHP;
    int		nShapeType, nVertices, nParts, *panParts, i, nVMax;
    double	*padfX, *padfY, *padfZ = NULL, *padfM = NULL;
    SHPObject	*psObject;
    const char  *tuple = "";
    const char  *filename;

/* -------------------------------------------------------------------- */
/*      Display a usage message.                                        */
/* -------------------------------------------------------------------- */
    if( argc < 2 )
    {
        printf( "shpadd shp_file [[x y] [+]]*\n" );
        printf( "  or\n" );
        printf( "shpadd shp_file -m [[x y m] [+]]*\n" );
        printf( "  or\n" );
        printf( "shpadd shp_file -z [[x y z] [+]]*\n" );
        printf( "  or\n" );
        printf( "shpadd shp_file -zm [[x y z m] [+]]*\n" );
        exit( 1 );
    }

    filename = argv[1];
    argv++;
    argc--;

/* -------------------------------------------------------------------- */
/*      Check for tuple description options.                            */
/* -------------------------------------------------------------------- */
    if( argc > 1 
        && (strcmp(argv[1],"-z") == 0 
            || strcmp(argv[1],"-m") == 0 
            || strcmp(argv[1],"-zm") == 0) )
    {
        tuple = argv[1] + 1;
        argv++;
        argc--;
    }

/* -------------------------------------------------------------------- */
/*      Open the passed shapefile.                                      */
/* -------------------------------------------------------------------- */
    hSHP = SHPOpen( filename, "r+b" );

    if( hSHP == NULL )
    {
        printf( "Unable to open:%s\n", filename );
        exit( 1 );
    }

    SHPGetInfo( hSHP, NULL, &nShapeType, NULL, NULL );

    if( argc == 1 )
        nShapeType = SHPT_NULL;

/* -------------------------------------------------------------------- */
/*	Build a vertex/part list from the command line arguments.	*/
/* -------------------------------------------------------------------- */
    nVMax = 1000;
    padfX = (double *) malloc(sizeof(double) * nVMax);
    padfY = (double *) malloc(sizeof(double) * nVMax);

    if( strchr(tuple,'z') )
        padfZ = (double *) malloc(sizeof(double) * nVMax);
    if( strchr(tuple,'m') )
        padfM = (double *) malloc(sizeof(double) * nVMax);
    
    nVertices = 0;

    if( (panParts = (int *) malloc(sizeof(int) * 1000 )) == NULL )
    {
        printf( "Out of memory\n" );
        exit( 1 );
    }
    
    nParts = 1;
    panParts[0] = 0;

    for( i = 1; i < argc;  )
    {
        if( argv[i][0] == '+' )
        {
            panParts[nParts++] = nVertices;
            i++;
        }
        else if( i < argc-1-(int)strlen(tuple) )
        {
            if( nVertices == nVMax )
            {
                nVMax = nVMax * 2;
                padfX = (double *) realloc(padfX,sizeof(double)*nVMax);
                padfY = (double *) realloc(padfY,sizeof(double)*nVMax);
                if( padfZ )
                    padfZ = (double *) realloc(padfZ,sizeof(double)*nVMax);
                if( padfM )
                    padfM = (double *) realloc(padfM,sizeof(double)*nVMax);
            }

            sscanf( argv[i++], "%lg", padfX+nVertices );
            sscanf( argv[i++], "%lg", padfY+nVertices );
            if( padfZ )
                sscanf( argv[i++], "%lg", padfZ+nVertices );
            if( padfM )
                sscanf( argv[i++], "%lg", padfM+nVertices );
                
            nVertices += 1;
        }
    }

/* -------------------------------------------------------------------- */
/*      Write the new entity to the shape file.                         */
/* -------------------------------------------------------------------- */
    psObject = SHPCreateObject( nShapeType, -1, nParts, panParts, NULL,
                                nVertices, padfX, padfY, padfZ, padfM );
    SHPWriteObject( hSHP, -1, psObject );
    SHPDestroyObject( psObject );
    
    SHPClose( hSHP );

    free( panParts );
    free( padfX );
    free( padfY );
    free( padfZ );
    free( padfM );

    return 0;
}
