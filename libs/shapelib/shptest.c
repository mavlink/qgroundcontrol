/******************************************************************************
 * $Id: shptest.c,v 1.8 2016-12-05 12:44:06 erouault Exp $
 *
 * Project:  Shapelib
 * Purpose:  Application for generating sample Shapefiles of various types.
 *           Used by the stream2.sh test script.
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
 * $Log: shptest.c,v $
 * Revision 1.8  2016-12-05 12:44:06  erouault
 * * Major overhaul of Makefile build system to use autoconf/automake.
 *
 * * Warning fixes in contrib/
 *
 * Revision 1.7  2004-09-26 20:09:35  fwarmerdam
 * avoid rcsid warnings
 *
 * Revision 1.6  2002/01/15 14:36:07  warmerda
 * updated email address
 *
 * Revision 1.5  2001/06/22 02:18:20  warmerda
 * Added null shape support
 *
 * Revision 1.4  2000/07/07 13:39:45  warmerda
 * removed unused variables, and added system include files
 *
 * Revision 1.3  1999/11/05 14:12:05  warmerda
 * updated license terms
 *
 * Revision 1.2  1998/12/16 05:15:20  warmerda
 * Added support for writing multipatch.
 *
 * Revision 1.1  1998/11/09 20:18:42  warmerda
 * Initial revision
 *
 */

#include <stdlib.h>
#include <string.h>
#include "shapefil.h"

SHP_CVSID("$Id: shptest.c,v 1.8 2016-12-05 12:44:06 erouault Exp $")

/************************************************************************/
/*                          Test_WritePoints()                          */
/*                                                                      */
/*      Write a small point file.                                       */
/************************************************************************/

static void Test_WritePoints( int nSHPType, const char *pszFilename )

{
    SHPHandle	hSHPHandle;
    SHPObject	*psShape;
    double	x, y, z, m;

    hSHPHandle = SHPCreate( pszFilename, nSHPType );

    x = 1.0;
    y = 2.0;
    z = 3.0;
    m = 4.0;
    psShape = SHPCreateObject( nSHPType, -1, 0, NULL, NULL,
                               1, &x, &y, &z, &m );
    SHPWriteObject( hSHPHandle, -1, psShape );
    SHPDestroyObject( psShape );
    
    x = 10.0;
    y = 20.0;
    z = 30.0;
    m = 40.0;
    psShape = SHPCreateObject( nSHPType, -1, 0, NULL, NULL,
                               1, &x, &y, &z, &m );
    SHPWriteObject( hSHPHandle, -1, psShape );
    SHPDestroyObject( psShape );

    SHPClose( hSHPHandle );
}

/************************************************************************/
/*                       Test_WriteMultiPoints()                        */
/*                                                                      */
/*      Write a small multipoint file.                                  */
/************************************************************************/

static void Test_WriteMultiPoints( int nSHPType, const char *pszFilename )

{
    SHPHandle	hSHPHandle;
    SHPObject	*psShape;
    double	x[4], y[4], z[4], m[4];
    int		i, iShape;

    hSHPHandle = SHPCreate( pszFilename, nSHPType );

    for( iShape = 0; iShape < 3; iShape++ )
    {
        for( i = 0; i < 4; i++ )
        {
            x[i] = iShape * 10 + i + 1.15;
            y[i] = iShape * 10 + i + 2.25;
            z[i] = iShape * 10 + i + 3.35;
            m[i] = iShape * 10 + i + 4.45;
        }
        
        psShape = SHPCreateObject( nSHPType, -1, 0, NULL, NULL,
                                   4, x, y, z, m );
        SHPWriteObject( hSHPHandle, -1, psShape );
        SHPDestroyObject( psShape );
    }    

    SHPClose( hSHPHandle );
}

/************************************************************************/
/*                         Test_WriteArcPoly()                          */
/*                                                                      */
/*      Write a small arc or polygon file.                              */
/************************************************************************/

static void Test_WriteArcPoly( int nSHPType, const char *pszFilename )

{
    SHPHandle	hSHPHandle;
    SHPObject	*psShape;
    double	x[100], y[100], z[100], m[100];
    int		anPartStart[100];
    int		anPartType[100], *panPartType;
    int		i, iShape;

    hSHPHandle = SHPCreate( pszFilename, nSHPType );

    if( nSHPType == SHPT_MULTIPATCH )
        panPartType = anPartType;
    else
        panPartType = NULL;

    for( iShape = 0; iShape < 3; iShape++ )
    {
        x[0] = 1.0;
        y[0] = 1.0+iShape*3;
        x[1] = 2.0;
        y[1] = 1.0+iShape*3;
        x[2] = 2.0;
        y[2] = 2.0+iShape*3;
        x[3] = 1.0;
        y[3] = 2.0+iShape*3;
        x[4] = 1.0;
        y[4] = 1.0+iShape*3;

        for( i = 0; i < 5; i++ )
        {
            z[i] = iShape * 10 + i + 3.35;
            m[i] = iShape * 10 + i + 4.45;
        }
        
        psShape = SHPCreateObject( nSHPType, -1, 0, NULL, NULL,
                                   5, x, y, z, m );
        SHPWriteObject( hSHPHandle, -1, psShape );
        SHPDestroyObject( psShape );
    }

/* -------------------------------------------------------------------- */
/*      Do a multi part polygon (shape).  We close it, and have two     */
/*      inner rings.                                                    */
/* -------------------------------------------------------------------- */
    x[0] = 0.0;
    y[0] = 0.0;
    x[1] = 0;
    y[1] = 100;
    x[2] = 100;
    y[2] = 100;
    x[3] = 100;
    y[3] = 0;
    x[4] = 0;
    y[4] = 0;

    x[5] = 10;
    y[5] = 20;
    x[6] = 30;
    y[6] = 20;
    x[7] = 30;
    y[7] = 40;
    x[8] = 10;
    y[8] = 40;
    x[9] = 10;
    y[9] = 20;

    x[10] = 60;
    y[10] = 20;
    x[11] = 90;
    y[11] = 20;
    x[12] = 90;
    y[12] = 40;
    x[13] = 60;
    y[13] = 40;
    x[14] = 60;
    y[14] = 20;

    for( i = 0; i < 15; i++ )
    {
        z[i] = i;
        m[i] = i*2;
    }

    anPartStart[0] = 0;
    anPartStart[1] = 5;
    anPartStart[2] = 10;

    anPartType[0] = SHPP_RING;
    anPartType[1] = SHPP_INNERRING;
    anPartType[2] = SHPP_INNERRING;
    
    psShape = SHPCreateObject( nSHPType, -1, 3, anPartStart, panPartType,
                               15, x, y, z, m );
    SHPWriteObject( hSHPHandle, -1, psShape );
    SHPDestroyObject( psShape );
    

    SHPClose( hSHPHandle );
}

/************************************************************************/
/*                                main()                                */
/************************************************************************/
int main( int argc, char ** argv )

{
/* -------------------------------------------------------------------- */
/*      Display a usage message.                                        */
/* -------------------------------------------------------------------- */
    if( argc != 2 )
    {
	printf( "shptest test_number\n" );
	exit( 1 );
    }

/* -------------------------------------------------------------------- */
/*      Figure out which test to run.                                   */
/* -------------------------------------------------------------------- */

    if( atoi(argv[1]) == 0 )
        Test_WritePoints( SHPT_NULL, "test0.shp" );

    else if( atoi(argv[1]) == 1 )
        Test_WritePoints( SHPT_POINT, "test1.shp" );
    else if( atoi(argv[1]) == 2 )
        Test_WritePoints( SHPT_POINTZ, "test2.shp" );
    else if( atoi(argv[1]) == 3 )
        Test_WritePoints( SHPT_POINTM, "test3.shp" );

    else if( atoi(argv[1]) == 4 )
        Test_WriteMultiPoints( SHPT_MULTIPOINT, "test4.shp" );
    else if( atoi(argv[1]) == 5 )
        Test_WriteMultiPoints( SHPT_MULTIPOINTZ, "test5.shp" );
    else if( atoi(argv[1]) == 6 )
        Test_WriteMultiPoints( SHPT_MULTIPOINTM, "test6.shp" );

    else if( atoi(argv[1]) == 7 )
        Test_WriteArcPoly( SHPT_ARC, "test7.shp" );
    else if( atoi(argv[1]) == 8 )
        Test_WriteArcPoly( SHPT_ARCZ, "test8.shp" );
    else if( atoi(argv[1]) == 9 )
        Test_WriteArcPoly( SHPT_ARCM, "test9.shp" );

    else if( atoi(argv[1]) == 10 )
        Test_WriteArcPoly( SHPT_POLYGON, "test10.shp" );
    else if( atoi(argv[1]) == 11 )
        Test_WriteArcPoly( SHPT_POLYGONZ, "test11.shp" );
    else if( atoi(argv[1]) == 12 )
        Test_WriteArcPoly( SHPT_POLYGONM, "test12.shp" );
    
    else if( atoi(argv[1]) == 13 )
        Test_WriteArcPoly( SHPT_MULTIPATCH, "test13.shp" );
    else
    {
        printf( "Test `%s' not recognised.\n", argv[1] );
        exit( 10 );
    }

#ifdef USE_DBMALLOC
    malloc_dump(2);
#endif

    exit( 0 );
}
