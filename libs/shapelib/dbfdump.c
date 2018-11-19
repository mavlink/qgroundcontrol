/******************************************************************************
 * $Id: dbfdump.c,v 1.14 2016-12-05 12:44:05 erouault Exp $
 *
 * Project:  Shapelib
 * Purpose:  Sample application for dumping .dbf files to the terminal.
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
 * $Log: dbfdump.c,v $
 * Revision 1.14  2016-12-05 12:44:05  erouault
 * * Major overhaul of Makefile build system to use autoconf/automake.
 *
 * * Warning fixes in contrib/
 *
 * Revision 1.13  2013-11-26 21:52:33  fwarmerdam
 * report deleted rows in dbfdump
 *
 * Revision 1.12  2006-06-17 00:15:08  fwarmerdam
 * Free panWidth for better memory testing.
 *
 * Revision 1.11  2006/02/15 01:11:27  fwarmerdam
 * added reporting of native type
 *
 * Revision 1.10  2004/09/26 20:09:35  fwarmerdam
 * avoid rcsid warnings
 *
 * Revision 1.9  2002/01/15 14:36:07  warmerda
 * updated email address
 *
 * Revision 1.8  2001/05/31 18:15:40  warmerda
 * Added support for NULL fields in DBF files
 *
 * Revision 1.7  2000/09/20 13:13:55  warmerda
 * added break after default:
 *
 * Revision 1.6  2000/07/07 13:39:45  warmerda
 * removed unused variables, and added system include files
 *
 * Revision 1.5  1999/11/05 14:12:04  warmerda
 * updated license terms
 *
 * Revision 1.4  1998/12/31 15:30:13  warmerda
 * Added -m, -r, and -h commandline options.
 *
 * Revision 1.3  1995/10/21 03:15:01  warmerda
 * Changed to use binary file access.
 *
 * Revision 1.2  1995/08/04  03:16:22  warmerda
 * Added header.
 *
 */

#include <stdlib.h>
#include <string.h>
#include "shapefil.h"

SHP_CVSID("$Id: dbfdump.c,v 1.14 2016-12-05 12:44:05 erouault Exp $")

int main( int argc, char ** argv )

{
    DBFHandle	hDBF;
    int		*panWidth, i, iRecord;
    char	szFormat[32], *pszFilename = NULL;
    int		nWidth, nDecimals;
    int		bHeader = 0;
    int		bRaw = 0;
    int		bMultiLine = 0;
    char	szTitle[12];

/* -------------------------------------------------------------------- */
/*      Handle arguments.                                               */
/* -------------------------------------------------------------------- */
    for( i = 1; i < argc; i++ )
    {
        if( strcmp(argv[i],"-h") == 0 )
            bHeader = 1;
        else if( strcmp(argv[i],"-r") == 0 )
            bRaw = 1;
        else if( strcmp(argv[i],"-m") == 0 )
            bMultiLine = 1;
        else
            pszFilename = argv[i];
    }

/* -------------------------------------------------------------------- */
/*      Display a usage message.                                        */
/* -------------------------------------------------------------------- */
    if( pszFilename == NULL )
    {
	printf( "dbfdump [-h] [-r] [-m] xbase_file\n" );
        printf( "        -h: Write header info (field descriptions)\n" );
        printf( "        -r: Write raw field info, numeric values not reformatted\n" );
        printf( "        -m: Multiline, one line per field.\n" );
	exit( 1 );
    }

/* -------------------------------------------------------------------- */
/*      Open the file.                                                  */
/* -------------------------------------------------------------------- */
    hDBF = DBFOpen( pszFilename, "rb" );
    if( hDBF == NULL )
    {
	printf( "DBFOpen(%s,\"r\") failed.\n", argv[1] );
	exit( 2 );
    }
    
/* -------------------------------------------------------------------- */
/*	If there is no data in this file let the user know.		*/
/* -------------------------------------------------------------------- */
    if( DBFGetFieldCount(hDBF) == 0 )
    {
	printf( "There are no fields in this table!\n" );
	exit( 3 );
    }

/* -------------------------------------------------------------------- */
/*	Dump header definitions.					*/
/* -------------------------------------------------------------------- */
    if( bHeader )
    {
        for( i = 0; i < DBFGetFieldCount(hDBF); i++ )
        {
            DBFFieldType	eType;
            const char	 	*pszTypeName;
            char chNativeType;

            chNativeType = DBFGetNativeFieldType( hDBF, i );

            eType = DBFGetFieldInfo( hDBF, i, szTitle, &nWidth, &nDecimals );
            if( eType == FTString )
                pszTypeName = "String";
            else if( eType == FTInteger )
                pszTypeName = "Integer";
            else if( eType == FTDouble )
                pszTypeName = "Double";
            else if( eType == FTInvalid )
                pszTypeName = "Invalid";

            printf( "Field %d: Type=%c/%s, Title=`%s', Width=%d, Decimals=%d\n",
                    i, chNativeType, pszTypeName, szTitle, nWidth, nDecimals );
        }
    }

/* -------------------------------------------------------------------- */
/*	Compute offsets to use when printing each of the field 		*/
/*	values. We make each field as wide as the field title+1, or 	*/
/*	the field value + 1. 						*/
/* -------------------------------------------------------------------- */
    panWidth = (int *) malloc( DBFGetFieldCount( hDBF ) * sizeof(int) );

    for( i = 0; i < DBFGetFieldCount(hDBF) && !bMultiLine; i++ )
    {
	DBFFieldType	eType;

	eType = DBFGetFieldInfo( hDBF, i, szTitle, &nWidth, &nDecimals );
	if( (int) strlen(szTitle) > nWidth )
	    panWidth[i] = strlen(szTitle);
	else
	    panWidth[i] = nWidth;

	if( eType == FTString )
	    sprintf( szFormat, "%%-%ds ", panWidth[i] );
	else
	    sprintf( szFormat, "%%%ds ", panWidth[i] );
	printf( szFormat, szTitle );
    }
    printf( "\n" );

/* -------------------------------------------------------------------- */
/*	Read all the records 						*/
/* -------------------------------------------------------------------- */
    for( iRecord = 0; iRecord < DBFGetRecordCount(hDBF); iRecord++ )
    {
        if( bMultiLine )
            printf( "Record: %d\n", iRecord );
        
	for( i = 0; i < DBFGetFieldCount(hDBF); i++ )
	{
            DBFFieldType	eType;
            
            eType = DBFGetFieldInfo( hDBF, i, szTitle, &nWidth, &nDecimals );

            if( bMultiLine )
            {
                printf( "%s: ", szTitle );
            }
            
/* -------------------------------------------------------------------- */
/*      Print the record according to the type and formatting           */
/*      information implicit in the DBF field description.              */
/* -------------------------------------------------------------------- */
            if( !bRaw )
            {
                if( DBFIsAttributeNULL( hDBF, iRecord, i ) )
                {
                    if( eType == FTString )
                        sprintf( szFormat, "%%-%ds", nWidth );
                    else
                        sprintf( szFormat, "%%%ds", nWidth );

                    printf( szFormat, "(NULL)" );
                }
                else
                {
                    switch( eType )
                    {
                      case FTString:
                        sprintf( szFormat, "%%-%ds", nWidth );
                        printf( szFormat, 
                                DBFReadStringAttribute( hDBF, iRecord, i ) );
                        break;
                        
                      case FTInteger:
                        sprintf( szFormat, "%%%dd", nWidth );
                        printf( szFormat, 
                                DBFReadIntegerAttribute( hDBF, iRecord, i ) );
                        break;
                        
                      case FTDouble:
                        sprintf( szFormat, "%%%d.%dlf", nWidth, nDecimals );
                        printf( szFormat, 
                                DBFReadDoubleAttribute( hDBF, iRecord, i ) );
                        break;
                        
                      default:
                        break;
                    }
                }
            }

/* -------------------------------------------------------------------- */
/*      Just dump in raw form (as formatted in the file).               */
/* -------------------------------------------------------------------- */
            else
            {
                sprintf( szFormat, "%%-%ds", nWidth );
                printf( szFormat, 
                        DBFReadStringAttribute( hDBF, iRecord, i ) );
            }

/* -------------------------------------------------------------------- */
/*      Write out any extra spaces required to pad out the field        */
/*      width.                                                          */
/* -------------------------------------------------------------------- */
	    if( !bMultiLine )
	    {
		sprintf( szFormat, "%%%ds", panWidth[i] - nWidth + 1 );
		printf( szFormat, "" );
	    }

            if( bMultiLine )
                printf( "\n" );

	    fflush( stdout );
	}

        if( DBFIsRecordDeleted(hDBF, iRecord) )
            printf( "(DELETED)" );
        
	printf( "\n" );
    }

    DBFClose( hDBF );
    free( panWidth );

    return( 0 );
}
