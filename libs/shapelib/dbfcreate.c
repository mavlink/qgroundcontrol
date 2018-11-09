/******************************************************************************
 * $Id: dbfcreate.c,v 1.8 2016-12-05 12:44:05 erouault Exp $
 *
 * Project:  Shapelib
 * Purpose:  Sample application for creating a new .dbf file.
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
 * $Log: dbfcreate.c,v $
 * Revision 1.8  2016-12-05 12:44:05  erouault
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
 * Revision 1.5  2000/07/07 13:39:45  warmerda
 * removed unused variables, and added system include files
 *
 * Revision 1.4  1999/11/05 14:12:04  warmerda
 * updated license terms
 *
 * Revision 1.3  1999/04/01 18:47:44  warmerda
 * Fixed DBFAddField() call convention.
 *
 * Revision 1.2  1995/08/04 03:17:11  warmerda
 * Added header.
 *
 */

#include <stdlib.h>
#include <string.h>
#include "shapefil.h"

SHP_CVSID("$Id: dbfcreate.c,v 1.8 2016-12-05 12:44:05 erouault Exp $")

int main( int argc, char ** argv )

{
    DBFHandle	hDBF;
    int		i;

/* -------------------------------------------------------------------- */
/*      Display a usage message.                                        */
/* -------------------------------------------------------------------- */
    if( argc < 2 )
    {
	printf( "dbfcreate xbase_file [[-s field_name width],[-n field_name width decimals]]...\n" );

	exit( 1 );
    }

/* -------------------------------------------------------------------- */
/*	Create the database.						*/
/* -------------------------------------------------------------------- */
    hDBF = DBFCreate( argv[1] );
    if( hDBF == NULL )
    {
	printf( "DBFCreate(%s) failed.\n", argv[1] );
	exit( 2 );
    }
    
/* -------------------------------------------------------------------- */
/*	Loop over the field definitions adding new fields.	       	*/
/* -------------------------------------------------------------------- */
    for( i = 2; i < argc; i++ )
    {
	if( strcmp(argv[i],"-s") == 0 && i < argc-2 )
	{
	    if( DBFAddField( hDBF, argv[i+1], FTString, atoi(argv[i+2]), 0 )
                == -1 )
	    {
		printf( "DBFAddField(%s,FTString,%d,0) failed.\n",
		        argv[i+1], atoi(argv[i+2]) );
		exit( 4 );
	    }
	    i = i + 2;
	}
	else if( strcmp(argv[i],"-n") == 0 && i < argc-3 )
	{
	    if( DBFAddField( hDBF, argv[i+1], FTDouble, atoi(argv[i+2]), 
			      atoi(argv[i+3]) ) == -1 )
	    {
		printf( "DBFAddField(%s,FTDouble,%d,%d) failed.\n",
		        argv[i+1], atoi(argv[i+2]), atoi(argv[i+3]) );
		exit( 4 );
	    }
	    i = i + 3;
	}
	else
	{
	    printf( "Argument incomplete, or unrecognised:%s\n", argv[i] );
	    exit( 3 );
	}
    }

    DBFClose( hDBF );

    return( 0 );
}
