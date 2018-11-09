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
 * shp2dxf.c
 *
 * derived from a ESRI Avenue Script
 * and DXF specification from AutoCad 3 (yes 1984)
 *
 * modifications Carl Andrson 11/96
 * modifications Carl Andrson 3/97
 *
 * converted to C code 12/98
 *
 * requires shapelib 1.2
 *   gcc shpdxf.c shpopen.o dbfopen.o -o shpdxf 
 *
 */

#include <stdlib.h>
#include <string.h>
#include "shapefil.h"

#define FLOAT_PREC "%16.5f\r\n"

void dxf_hdr (x1,y1,x2,y2,df)
double x1,y1,x2,y2;
FILE  *df;
{
/* Create HEADER section */

  fprintf( df, "  0\r\n");
  fprintf( df, "SECTION\r\n");
  fprintf( df, "  2\r\n" );
  fprintf( df, "HEADER\r\n" );
  fprintf( df, "  9\r\n" );
  fprintf( df, "$EXTMAX\r\n" );
  fprintf( df, " 10\r\n" );
  fprintf( df, FLOAT_PREC, x2 );
  fprintf( df, " 20\r\n" );
  fprintf( df, FLOAT_PREC, y2 );
  fprintf( df, "  9\r\n" );
  fprintf( df, "$EXTMIN\r\n" );
  fprintf( df, " 10\r\n" );
  fprintf( df, FLOAT_PREC, x1 );
  fprintf( df, " 20\r\n" );
  fprintf( df, FLOAT_PREC, y1 );
  fprintf( df, "  9\r\n" );
  fprintf( df, "$LUPREC\r\n" );
  fprintf( df, " 70\r\n" );
  fprintf( df, "    14\r\n" );
  fprintf( df, "  0\r\n" );
  fprintf( df, "ENDSEC\r\n" );

/*  ' Create TABLES section */

  fprintf( df, "  0\r\n" );
  fprintf( df, "SECTION\r\n" );
  fprintf( df, "  2\r\n" );
  fprintf( df, "TABLES\r\n" );
/*  ' Table 1 - set up line type */
  fprintf( df, "  0\r\n" );
  fprintf( df, "TABLE\r\n" );
  fprintf( df, "  2\r\n" );
  fprintf( df, "LTYPE\r\n" );
  fprintf( df, " 70\r\n" );
  fprintf( df, "2\r\n" );
/*  ' Entry 1 of Table 1 */
  fprintf( df, "  0\r\n" );
  fprintf( df, "LTYPE\r\n" );
  fprintf( df, "  2\r\n" );
  fprintf( df, "CONTINUOUS\r\n" );
  fprintf( df, " 70\r\n" );
  fprintf( df, "64\r\n" );
  fprintf( df, "  3\r\n" );
  fprintf( df, "Solid line\r\n" );
  fprintf( df, " 72\r\n" );
  fprintf( df, "65\r\n" );
  fprintf( df, " 73\r\n" );
  fprintf( df, "0\r\n" );
  fprintf( df, " 40\r\n" );
  fprintf( df, "0.0\r\n" );
  fprintf( df, "  0\r\n" );
  fprintf( df, "ENDTAB\r\n" );
 /* End of TABLES section */
  fprintf( df, "  0\r\n" );
  fprintf( df, "ENDSEC\r\n" );

  /* Create BLOCKS section  */
  
  fprintf( df, "  0\r\n" );
  fprintf( df, "SECTION\r\n" );
  fprintf( df, "  2\r\n" );
  fprintf( df, "BLOCKS\r\n" );
  fprintf( df, "  0\r\n" );
  fprintf( df, "ENDSEC\r\n" );
  fprintf( df, "  0\r\n" );
  fprintf( df, "SECTION\r\n" );
  fprintf( df, "  2\r\n" );
  fprintf( df, "ENTITIES\r\n" );
  
}


void
dxf_ent_preamble (dxf_type, id, df)
int dxf_type;
char *id;
FILE *df;
{

  fprintf( df, "  0\r\n" );

  switch (dxf_type) {
    case  SHPT_POLYGON:
    case  SHPT_ARC:	  fprintf (df, "POLYLINE\r\n");
          break;
    default:  fprintf(df, "POINT\r\n");
  }	   

  fprintf( df, "  8\r\n");
  fprintf( df, "%s\r\n", id );
  switch ( dxf_type ) {
    case SHPT_ARC:
                    fprintf( df, "  6\r\n" );
                    fprintf( df, "CONTINUOUS\r\n" );
                    fprintf( df, " 66\r\n" );
                    fprintf( df, "1\r\n" );
                 break;
    case SHPT_POLYGON:
                    fprintf( df, "  6\r\n" );
                    fprintf( df, "CONTINUOUS\r\n" );
                    fprintf( df, " 66\r\n" );
                    fprintf( df, "1\r\n" );
                    fprintf( df, " 70\r\n");
		    fprintf (df, "1\r\n");
    default:     break;
  } 

}

void
dxf_ent (id, x, y, z, dxf_type, df)
char *id;
double x,y,z;
int dxf_type;
FILE *df;
{
  if ((dxf_type == SHPT_ARC) || ( dxf_type == SHPT_POLYGON))  {
    fprintf( df, "  0\r\n");
    fprintf( df, "VERTEX\r\n");
    fprintf( df, "  8\r\n");
    fprintf( df, "%s\r\n", id);
  }  
  fprintf( df, " 10\r\n" );
  fprintf( df, FLOAT_PREC, x );
  fprintf( df, " 20\r\n" );
  fprintf( df, FLOAT_PREC, y );
  fprintf( df, " 30\r\n" );
  if ( z != 0 ) 
    fprintf( df, FLOAT_PREC, z );
  else
    fprintf( df, "0.0\r\n" );	       
}


void
dxf_ent_postamble (dxf_type, df)
int dxf_type;
FILE *df;
{
  if ((dxf_type == SHPT_ARC) || ( dxf_type == SHPT_POLYGON)) 
    fprintf( df, "  0\r\nSEQEND\r\n  8\r\n0\r\n");
}


int
main (int argc, char **argv)

{
    char shpFileName[80] = "", dbfFileName[80] = "";
    char dxfFileName[80] = "";
    char idfldName[15];
    char zfldName[6] = "ELEV";
    char fldName[15];
    char id[255];
    double elev;
    int   parts, *panParts, nParts, nVertices;
    FILE	*dxf;
    SHPHandle shp;
    DBFHandle dbf;
    DBFFieldType  idfld_type;
    double adfBoundsMin[4], adfBoundsMax[4];
    int	vrtx, shp_type, shp_numrec, zfld, idfld, nflds, recNum, part;
    unsigned int MaxElem = -1;
 
    if ( argc < 2 )  {
        printf ("usage: shpdxf shapefile {idfield}\r\n");
        exit (-1);
    }
   
    strcpy (shpFileName,argv[1]);
    strncpy (dbfFileName, shpFileName, strlen(shpFileName)-3);
    strcat (dbfFileName,"dbf"); 
  
    strncpy (dxfFileName, shpFileName,strlen(shpFileName)-3);
    strcat( dxfFileName, "dxf");
  
    shp = SHPOpen (shpFileName, "rb");
    dbf = DBFOpen (dbfFileName, "rb");
    dxf = fopen( dxfFileName, "w");

    printf("Starting conversion %s(%s) -> %s\r\n",
           shpFileName,dbfFileName,dxfFileName);
  
    SHPGetInfo (shp, &shp_numrec, &shp_type, adfBoundsMin, adfBoundsMax );
    printf ("file has %d objects\r\n", shp_numrec);
    dxf_hdr(adfBoundsMin[0], adfBoundsMin[1], adfBoundsMax[0], adfBoundsMax[1],
            dxf);
      
/* Before proceeding, allow the user to specify the ID field to use or default to the record number.... */

    if ( argc > 3 )  MaxElem = atoi(argv[3]);
  
    nflds = DBFGetFieldCount(dbf);
    if ( argc > 2 ) {
        strcpy (idfldName, argv[2]);
        for ( idfld=0; idfld < nflds; idfld++ ) {
            idfld_type = DBFGetFieldInfo( dbf, idfld, fldName, NULL, NULL);
            if (!strcmp (idfldName, fldName ))
                break;
        }
        if ( idfld >= nflds ) {
            printf ("Id field %s not found, using default\r\n",idfldName); 
            idfld = -1;  
        } else
            printf ("proceeding with field %s for LayerNames\r\n",fldName);
    }
    else
        idfld = -1;  
    
    for ( zfld=0; zfld < nflds; zfld++ ) {
        DBFGetFieldInfo( dbf, zfld, fldName, NULL, NULL);
        if (!strcmp (zfldName, fldName  ))
            break;
    }
    if ( zfld >= nflds ) 
        zfld = -1;
//  printf ("proceeding with id = %d, elevation = %d\r\n",idfld, zfld);
  
/* Proceed to process data..... */
  
    for ( recNum = 0; (recNum < shp_numrec) && (recNum < MaxElem); recNum++)  {

        SHPObject	*shape;

        if ( idfld >= 0 )
            switch (idfld_type) {
              case FTString:  sprintf (id, "lvl_%s",DBFReadStringAttribute ( dbf, recNum, idfld ));
                break;
              default:    sprintf(id, "%-20.0lf", DBFReadDoubleAttribute (dbf, recNum, idfld));
            }
        else
            sprintf (id,"lvl_%-20d",(recNum +1 )); 

    
        if ( zfld >= 0 ) 
            elev = 0;
        else  
            elev = DBFReadDoubleAttribute ( dbf, recNum, zfld );
  
#ifdef DEBUG
        printf("\r\nworking on obj %d", recNum);
#endif

        shape = SHPReadObject( shp, recNum );
   
        nVertices = shape->nVertices;
        nParts = shape->nParts;
        panParts = shape->panPartStart;
        part = 0;
        for (vrtx=0; vrtx < nVertices; vrtx ++ ) { 
#ifdef DEBUG
        printf("\rworking on part %d, vertex %d", part,vrtx);     
#endif 
            if ( panParts[part] == vrtx ) {
#ifdef DEBUG
       printf ("object preamble\r\n");
#endif
                dxf_ent_preamble (shp_type, id, dxf);
            }
     
            dxf_ent (id, shape->padfX[vrtx], shape->padfY[vrtx],
                     elev, shp_type, dxf);
 
            if ((panParts[part] == (vrtx + 1))|| (vrtx == (nVertices -1)) ) {
                dxf_ent_postamble (shp_type, dxf);
                part ++;
            }
        }
        SHPDestroyObject( shape );
    }

/* close out DXF file */
    fprintf( dxf, "0\r\n" );
    fprintf( dxf, "ENDSEC\r\n" );
    fprintf( dxf, "0\r\n" );
    fprintf( dxf, "EOF\r\n" );


    SHPClose (shp);
    DBFClose (dbf);
    fclose (dxf);
}






