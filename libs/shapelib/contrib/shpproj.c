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
 *
 * requires shapelib 1.2
 *   gcc shpproj ../shpopen.o ../dbfopen.o shpgeo.o -lm -lproj -o shpproj
 * 
 * this requires linking with the PROJ4.3 projection library available from
 *
 * ftp://kai.er.usgs.gov/ftp/PROJ.4
 *
 *
 * SHPGeo must be compiled with -DPROJ4 support
 *
 * $Log: shpproj.c,v $
 * Revision 1.10  2011-07-24 03:17:46  fwarmerdam
 * include string.h and stdlib.h where needed in contrib (#2146)
 *
 * Revision 1.9  2002-01-15 14:37:08  warmerda
 * upgrade to use proj_api.h
 *
 * Revision 1.8  2002/01/11 15:47:26  warmerda
 * several fixes
 *
 * Revision 1.7  2002/01/11 15:23:28  warmerda
 * use text mode reading and writing .prj files
 *
 * Revision 1.6  1999/05/26 02:56:31  candrsn
 * updates to shpdxf, dbfinfo, port from Shapelib 1.1.5 of dbfcat and shpinfo
 *
 * Revision 1.2  1999/05/13 19:30:52  warmerda
 * Removed libgen.h, added url for PROJ.4, and corrected unsafe return of
 * local variable in asFileName().
 *
 */


#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "shapefil.h"
#include "shpgeo.h"

int main( int argc, char ** argv )
{
    SHPHandle	old_SHP, new_SHP;
    DBFHandle   old_DBF, new_DBF;
    int		nShapeType, nEntities, nVertices, nParts, *panParts, i, iPart, j;
    double	*padVertices, adBounds[4];
    const char 	*pszPlus;
    DBFFieldType  idfld_type;
    SHPObject	*psCShape;
    FILE	*ifp = NULL;
    int		idfld, nflds;
    char	kv[257] = "";
    char	idfldName[120] = "";
    char	fldName[120] = "";
    char	shpFileName[120] = "";
    char	dbfFileName[120] = "";
    char	prjFileName[120] = "";
    char	parg[80];
    double	apeture[4];
    int		inarg, outarg;
    char	*DBFRow = NULL;

/* for testing only 
    char	*in_args[] = { "init=nad83:1002", "units=us-ft" };
    char	*out_args[] = { "proj=utm", "zone=16", "units=m" };
*/

    char	*in_args[16];
    char	*out_args[16];
    int		in_argc = 0 , out_argc = 0, outf_arg;
    char	*arglst;
    projPJ	orig_prj, new_prj;
    va_list	myargs, moargs;

    if( argc < 4)
    {
	printf( "shpproj shp_file new_shp ( -i=in_proj_file | -i=\"in_params\" | -i=geographic ) ( -o=out_info_file | -o=\"out_params\" | -o=geographic ) \n" );
	exit( 1 );
    }

    old_SHP = SHPOpen( argv[1], "rb" );
    old_DBF = DBFOpen( argv[1], "rb" );
    if( old_SHP == NULL || old_DBF == NULL )
    {
	printf( "Unable to open old files:%s\n", argv[1] );
	exit( 1 );
    }
    
   outf_arg = 2;
   inarg = 0;
   outarg = 0;
   for ( i = 3; i < argc; i++ ) {
     if ( !strncmp ("-i=", argv[i], 3 ))  inarg = i;
     if ( !strncmp ("-o=", argv[i], 3 ))  outarg = i;
    }


/* if shapefile has a prj component then use that 
   else try for a file then read args as list */

    if( inarg == 0 )
    {
        strcpy( prjFileName, argv[1] );
        ifp = fopen( asFileName ( prjFileName, "prj" ),"rt");
    }
    else
    {
        ifp = fopen( asFileName ( argv[inarg] + 3, "prj" ),"rt");
    }

    i = 0;
    if ( ifp ) {
        if( inarg == 0 )
            printf ("using default file proj params from <- %s\n",
                    asFileName ( prjFileName, "prj"  ) );
        else
            printf ("using file proj params from <- %s\n",
                    asFileName ( argv[inarg] + 3, "prj" ) );

       while( fscanf( ifp, "%s", parg) != EOF ) {
         in_args[i] = malloc ( strlen(parg)+1 );
         strcpy ( in_args[i], parg);
         i++;
       }

       in_argc = i;
       fclose (ifp);
      }
     else {
      if ( inarg > 0 ) {
       arglst = argv[inarg] + 3;
       j = 0;
       i = 0;
       while ( j < strlen (arglst) ) {    
         in_argc += sscanf ( arglst + j, "%s", parg);
        
         in_args[i] = malloc( strlen (parg)+1); 
         strcpy (in_args[i], parg);
         i++;
         j += strlen (parg) +1;
         if ( arglst[j] + 1 == 0 ) j = strlen (argv[inarg]);  
       }
      }
     }  

    i = 0;
    if ( outarg > 0 ) ifp = fopen( asFileName ( argv[outarg] + 3, "prj" ),"rt");   
    if ( ifp ) {
       while( fscanf( ifp, "%s", parg) != EOF ) {
         out_args[i] = malloc ( strlen(parg));
         strcpy ( out_args[i], parg);
         i++;
       }
       out_argc = i;
       fclose (ifp);
     }
     else {
      if ( outarg > 0 ) {
       arglst = argv[outarg] + 3;
       j = 0;
       i = 0;
       while ( j < strlen (arglst) ) {    
         out_argc += sscanf ( arglst + j, "%s", parg);
         
         out_args[i] = malloc( strlen (parg)+1); 
         strcpy (out_args[i], parg);
         i++;
         j += strlen (parg) +1;
         if ( arglst[j] + 1 == 0 ) j = strlen (argv[outarg]);  
       }
      }
    }       
    
    if ( !strcmp( argv[inarg], "-i=geographic" )) in_argc = 0;
    if ( !strcmp( argv[outarg], "-o=geographic" )) out_argc = 0;
    
    orig_prj = SHPSetProjection ( in_argc, in_args );
    new_prj = SHPSetProjection ( out_argc, out_args );

    if ( !(( (!in_argc) || orig_prj) && ( (!out_argc) || new_prj) )) { 
      fprintf (stderr, "unable to process projection, exiting...\n");
      exit(1);
    }   


    SHPGetInfo( old_SHP, &nEntities, &nShapeType, NULL, NULL);
    new_SHP = SHPCreate ( argv[outf_arg], nShapeType ); 
    
    new_DBF = DBFCloneEmpty (old_DBF, argv[outf_arg]);
    if( new_SHP == NULL || new_DBF == NULL )
    {
	printf( "Unable to create new files:%s\n", argv[outf_arg] );
	exit( 1 );
    }

    DBFRow = (char *) malloc ( (old_DBF->nRecordLength) + 15 );
     
    for( i = 0; i < nEntities; i++ )
    {
	int		j;

	psCShape = SHPReadObject ( old_SHP, i );

	SHPProject (psCShape, orig_prj, new_prj );

	SHPWriteObject ( new_SHP, -1, psCShape );   
	SHPDestroyObject ( psCShape );

        memcpy ( DBFRow, DBFReadTuple ( old_DBF, i ), old_DBF->nRecordLength );
        DBFWriteTuple ( new_DBF, new_DBF->nRecords, DBFRow );

    }

    SHPFreeProjection ( orig_prj );
    SHPFreeProjection ( new_prj );

    /* store projection params into prj file */
    ifp = fopen( asFileName ( argv[outf_arg], "prj" ),"wt");   
    if ( ifp ) {

       if ( out_argc == 0 ) 
        { fprintf( ifp, "proj=geographic\n" ); }
       else
        { for ( i = 0; i < out_argc; i++ )
           fprintf( ifp, "%s\n", out_args[i]);
        }
       fclose (ifp);
    }
    
    SHPClose( old_SHP );
    SHPClose( new_SHP );
    DBFClose( old_DBF );
    DBFClose( new_DBF );
    printf ("\n");
}


