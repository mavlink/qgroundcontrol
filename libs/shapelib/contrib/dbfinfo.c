/*
 * Copyright (c) 1999 Carl Anderson
 *
 * This code is in the public domain.
 *
 * This code is based in part on the earlier work of Frank Warmerdam
 *
 * requires shapelib 1.2
 *   gcc dbfinfo dbfopen.o dbfinfo
 * 
 *
 * $Log: dbfinfo.c,v $
 * Revision 1.4  2016-12-05 12:44:07  erouault
 * * Major overhaul of Makefile build system to use autoconf/automake.
 *
 * * Warning fixes in contrib/
 *
 * Revision 1.3  2011-07-24 03:17:46  fwarmerdam
 * include string.h and stdlib.h where needed in contrib (#2146)
 *
 * Revision 1.2  1999-05-26 02:56:31  candrsn
 * updates to shpdxf, dbfinfo, port from Shapelib 1.1.5 of dbfcat and shpinfo
 *
 * 
 */

#include <stdlib.h>
#include <string.h>
#include "shapefil.h"

int main( int argc, char ** argv )

{
    DBFHandle	hDBF;
    int		*panWidth, i, iRecord;
    char	szFormat[32], szField[1024];
    char	ftype[32], cTitle[32], nTitle[32];
    int		nWidth, nDecimals;
    int		cnWidth, cnDecimals;
    DBFHandle	cDBF;
    DBFFieldType	hType,cType;
    int		ci, ciRecord;

/* -------------------------------------------------------------------- */
/*      Display a usage message.                                        */
/* -------------------------------------------------------------------- */
    if( argc != 2 )
    {
	printf( "dbfinfo xbase_file\n" );
	exit( 1 );
    }

/* -------------------------------------------------------------------- */
/*      Open the file.                                                  */
/* -------------------------------------------------------------------- */
    hDBF = DBFOpen( argv[1], "rb" );
    if( hDBF == NULL )
    {
	printf( "DBFOpen(%s,\"r\") failed.\n", argv[1] );
	exit( 2 );
    }

    printf ("Info for %s\n",argv[1]);

/* -------------------------------------------------------------------- */
/*	If there is no data in this file let the user know.		*/
/* -------------------------------------------------------------------- */
    i = DBFGetFieldCount(hDBF);
    printf ("%d Columns,  %d Records in file\n",i,DBFGetRecordCount(hDBF));
    
/* -------------------------------------------------------------------- */
/*	Compute offsets to use when printing each of the field 		*/
/*	values. We make each field as wide as the field title+1, or 	*/
/*	the field value + 1. 						*/
/* -------------------------------------------------------------------- */
    panWidth = (int *) malloc( DBFGetFieldCount( hDBF ) * sizeof(int) );

    for( i = 0; i < DBFGetFieldCount(hDBF); i++ )
    {
	char		szTitle[12];
	DBFFieldType	eType;

	switch ( DBFGetFieldInfo( hDBF, i, szTitle, &nWidth, &nDecimals )) {
	      case FTString:
	        strcpy (ftype, "string");;
		break;

	      case FTInteger:
	    	strcpy (ftype, "integer");
		break;

	      case FTDouble:
	    	strcpy (ftype, "float");
		break;
		
	      case FTInvalid:
	    	strcpy (ftype, "invalid/unsupported");
		break;
		
	      default:
	      	strcpy (ftype, "unknown");
	      	break;			
	    }
        printf ("%15.15s\t%15s  (%d,%d)\n",szTitle, ftype, nWidth, nDecimals);

    }

    DBFClose( hDBF );

    return( 0 );
}
