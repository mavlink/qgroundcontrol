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
 *   gcc shpproj shpopen.o dbfopen.o -lm -lproj -o shpproj
 * 
 * this may require linking with the PROJ4 projection library available from
 *
 * http://www.remotesensing.org/proj
 *
 * use -DPROJ4 to compile in Projection support
 *
 * $Log: shpgeo.c,v $
 * Revision 1.16  2017-07-10 18:01:35  erouault
 * * contrib/shpgeo.c: fix compilation on _MSC_VER < 1800 regarding lack
 * of NAN macro.
 *
 * Revision 1.15  2016-12-06 21:13:33  erouault
 * * configure.ac: change soname to 2:1:0 to be in sync with Debian soname.
 * http://bugzilla.maptools.org/show_bug.cgi?id=2628
 * Patch by Bas Couwenberg
 *
 * * contrib/doc/Shape_PointInPoly_README.txt, contrib/shpgeo.c: typo fixes.
 * http://bugzilla.maptools.org/show_bug.cgi?id=2629
 * Patch by Bas Couwenberg
 *
 * * web/*: use a local .css file to avoid a privacy breach issue reported
 * by the lintian QA tool.
 * http://bugzilla.maptools.org/show_bug.cgi?id=2630
 * Patch by Bas Couwenberg
 *
 *
 * Contributed by Sandro Mani: https://github.com/manisandro/shapelib/tree/autotools
 *
 * Revision 1.14  2016-12-05 12:44:07  erouault
 * * Major overhaul of Makefile build system to use autoconf/automake.
 *
 * * Warning fixes in contrib/
 *
 * Revision 1.13  2011-07-24 03:17:46  fwarmerdam
 * include string.h and stdlib.h where needed in contrib (#2146)
 *
 * Revision 1.12  2007-09-03 23:17:46  fwarmerdam
 * fix SHPDimension() function
 *
 * Revision 1.11  2006/11/06 20:45:58  fwarmerdam
 * Fixed SHPProject.
 *
 * Revision 1.10  2006/11/06 20:44:58  fwarmerdam
 * SHPProject() uses pj_transform now
 *
 * Revision 1.9  2006/01/25 15:33:50  fwarmerdam
 * fixed ppsC assignment maptools bug 1263
 *
 * Revision 1.8  2002/01/15 14:36:56  warmerda
 * upgrade to use proj_api.h
 *
 * Revision 1.7  2002/01/11 15:22:04  warmerda
 * fix many warnings.  Lots of this code is cruft.
 *
 * Revision 1.6  2001/08/30 13:42:31  warmerda
 * avoid use of auto initialization of PT for VC++
 *
 * Revision 1.5  2000/04/26 13:24:06  warmerda
 * made projUV handling safer
 *
 * Revision 1.4  2000/04/26 13:17:15  warmerda
 * check if projUV or UV
 *
 * Revision 1.3  2000/03/17 14:15:16  warmerda
 * Don't try to use system nan.h ... doesn't always exist.
 *
 * Revision 1.2  1999/05/26 02:56:31  candrsn
 * updates to shpdxf, dbfinfo, port from Shapelib 1.1.5 of dbfcat and shpinfo
 *
 */

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "shapefil.h"

#include "shpgeo.h"

#if defined(_MSC_VER) && _MSC_VER < 1800
#include <float.h>
#define INFINITY (DBL_MAX + DBL_MAX)
#define NAN (INFINITY - INFINITY)
#endif


 /* I'm using some shorthand throughout this file
 *      R+ is a Clockwise Ring and is the positive portion of an object
 *      R- is a CounterClockwise Ring and is a hole in a R+
 *      A complex object is one having at least one R-
 *      A compound object is one having more than one R+
 *	A simple object has one and only one element (R+ or R-)
 *
 *	The closed ring constraint is for polygons and assumed here
 *	Arcs or LineStrings I am calling Rings (generically open or closed)
 *	Point types are vertices or lists of vertices but not Rings
 *
 *   SHPT_POLYGON, SHPT_POLYGONZ, SHPT_POLYGONM and SHPT_MULTIPATCH
 *   can have SHPObjects that are compound as well as complex
 *  
 *   SHP_POINT and its Z and M derivatives are strictly simple
 *   MULTI_POINT, SHPT_ARC and their derivatives may be simple or compound
 *
 */


/* **************************************************************************
 * asFileName
 *
 * utility function, toss part of filename after last dot
 *
 * **************************************************************************/ 
char * asFileName ( const char *fil, char *ext ) {
  char	pszBasename[120];
  static char	pszFullname[120];
  int	i;
/* -------------------------------------------------------------------- */
/*	Compute the base (layer) name.  If there is any extension	*/
/*	on the passed in filename we will strip it off.			*/
/* -------------------------------------------------------------------- */
//    pszFullname = (char*) malloc(( strlen(fil)+5 ));
//    pszBasename = (char *) malloc(strlen(fil)+5);
    strcpy( pszBasename, fil );
    for( i = strlen(pszBasename)-1; 
	 i > 0 && pszBasename[i] != '.' && pszBasename[i] != '/'
	       && pszBasename[i] != '\\';
	 i-- ) {}

    if( pszBasename[i] == '.' )
        pszBasename[i] = '\0';

/* -------------------------------------------------------------------- */
/*	Note that files pulled from										*/
/*	a PC to Unix with upper case filenames won't work!				*/
/* -------------------------------------------------------------------- */
//    pszFullname = (char *) malloc(strlen(pszBasename) + 5);
    sprintf( pszFullname, "%s.%s", pszBasename, ext );

    return ( pszFullname );
}


/************************************************************************/
/*                             SfRealloc()                              */
/*                                                                      */
/*      A realloc cover function that will access a NULL pointer as     */
/*      a valid input.                                                  */
/************************************************************************/
/* copied directly from shpopen.c -- maybe expose this in shapefil.h	*/  
static void * SfRealloc( void * pMem, int nNewSize )

{
    if( pMem == NULL )
        return( (void *) malloc(nNewSize) );
    else
        return( (void *) realloc(pMem,nNewSize) );
}


/* **************************************************************************
 * SHPPRoject
 *
 * Project points using projection handles, for use with PROJ4.3
 *
 * act as a wrapper to protect against library changes in PROJ
 * 
 * **************************************************************************/ 
int SHPProject ( SHPObject *psCShape, projPJ inproj, projPJ outproj ) {
#ifdef	PROJ4

    int    j;

    if ( pj_is_latlong(inproj) ) {
        for(j=0; j < psCShape->nVertices; j++) {
            psCShape->padfX[j] *= DEG_TO_RAD;
            psCShape->padfY[j] *= DEG_TO_RAD;
        }
    }   

    pj_transform(inproj, outproj, psCShape->nVertices, 0, psCShape->padfX,
                 psCShape->padfY, NULL);

    if ( pj_is_latlong(outproj) ) {
        for(j=0; j < psCShape->nVertices; j++) {
            psCShape->padfX[j] *= RAD_TO_DEG;
            psCShape->padfY[j] *= RAD_TO_DEG;
        }
    }   

    /* Recompute new Extents of projected Object								*/
    SHPComputeExtents ( psCShape );
#endif  

    return ( 1 );
}


/* **************************************************************************
 * SHPSetProjection
 *
 * establish a projection handle for use with PROJ4.3
 *
 * act as a wrapper to protect against library changes in PROJ
 *
 * **************************************************************************/
projPJ SHPSetProjection ( int param_cnt, char **params ) {
#ifdef PROJ4
  projPJ	*p = NULL;

  if ( param_cnt > 0 && params[0] )
  {
      p = pj_init ( param_cnt, params );
  }
  else
  {
      char* params_local[] = { "+proj=longlat", NULL };
      p = pj_init ( 1, params_local );
  }

  return ( p );
#else
  return ( NULL );
#endif
}


/* **************************************************************************
 * SHPFreeProjection
 *
 * release a projection handle for use with PROJ4.3
 *
 * act as a wrapper to protect against library changes in PROJ
 * 
 * **************************************************************************/
int SHPFreeProjection ( projPJ p) {
#ifdef PROJ4
  if ( p )
    pj_free ( p );
#endif
  return ( 1 );
}


/* **************************************************************************
 * SHPOGisType
 *
 * Convert Both ways from and to OGIS Geometry Types
 *
 * **************************************************************************/
int SHPOGisType ( int GeomType, int toOGis) {

    if ( toOGis == 0 )  /* connect OGis -> SHP types  					*/
	switch (GeomType) {
            case (OGIST_POINT):		return ( SHPT_POINT );      break;
            case (OGIST_LINESTRING):	return ( SHPT_ARC );        break;
            case (OGIST_POLYGON):		return ( SHPT_POLYGON );    break;
            case (OGIST_MULTIPOINT):	return ( SHPT_MULTIPOINT ); break;
            case (OGIST_MULTILINE):	return ( SHPT_ARC );	    break;
            case (OGIST_MULTIPOLYGON):	return ( SHPT_POLYGON );    break;
        }
    else  /* ok so its SHP->OGis types 									*/
        switch (GeomType) {
	    case (SHPT_POINT):		return ( OGIST_POINT );	    break;
	    case (SHPT_POINTM):		return ( OGIST_POINT );	    break;
	    case (SHPT_POINTZ):		return ( OGIST_POINT );	    break;
	    case (SHPT_ARC):		return ( OGIST_LINESTRING );break;
	    case (SHPT_ARCZ):		return ( OGIST_LINESTRING );break;
	    case (SHPT_ARCM):		return ( OGIST_LINESTRING );break;
	    case (SHPT_POLYGON):	return ( OGIST_MULTIPOLYGON );break;
	    case (SHPT_POLYGONZ):	return ( OGIST_MULTIPOLYGON );break;
	    case (SHPT_POLYGONM):	return ( OGIST_MULTIPOLYGON );break;
	    case (SHPT_MULTIPOINT):	return ( OGIST_MULTIPOINT );break;
	    case (SHPT_MULTIPOINTZ):	return ( OGIST_MULTIPOINT );break;
	    case (SHPT_MULTIPOINTM):	return ( OGIST_MULTIPOINT );break;
	    case (SHPT_MULTIPATCH):	return ( OGIST_GEOMCOLL );  break;
        } 	  	  	  

    return 0;
}


/* **************************************************************************
 * SHPReadSHPStream
 *
 * Encapsulate entire SHPObject for use with Postgresql
 *
 * **************************************************************************/
int SHPReadSHPStream ( SHPObject *psCShape, char *stream_obj) {

int	obj_storage;
int	my_order, need_swap =0, GeoType ;
int	use_Z = 0;
int	use_M = 0;

  need_swap = stream_obj[0];
  my_order = 1;
  my_order = ((char*) (&my_order))[0];
  need_swap = need_swap & my_order;

  if ( need_swap )
     swapW (stream_obj, (void*) &GeoType, sizeof (GeoType) );
   else
     memcpy (stream_obj, &GeoType, sizeof (GeoType) );

           
  if ( need_swap ) {
  
  } else {
    memcpy (stream_obj, &(psCShape->nSHPType),  sizeof (psCShape->nSHPType) );
    memcpy (stream_obj, &(psCShape->nShapeId),  sizeof (psCShape->nShapeId) );
    memcpy (stream_obj, &(psCShape->nVertices), sizeof (psCShape->nVertices) );
    memcpy (stream_obj, &(psCShape->nParts), 	 sizeof (psCShape->nParts) );
    memcpy (stream_obj, &(psCShape->dfXMin), 	 sizeof (psCShape->dfXMin) );
    memcpy (stream_obj, &(psCShape->dfYMin), 	 sizeof (psCShape->dfYMin) );
    memcpy (stream_obj, &(psCShape->dfXMax), 	 sizeof (psCShape->dfXMax) );
    memcpy (stream_obj, &(psCShape->dfYMax), 	 sizeof (psCShape->dfYMax) );
    if ( use_Z ) {
      memcpy (stream_obj, &(psCShape->dfZMin), 	 sizeof (psCShape->dfZMin) );
      memcpy (stream_obj, &(psCShape->dfZMax), 	 sizeof (psCShape->dfZMax) );
    }
   
    memcpy (stream_obj, psCShape->panPartStart, psCShape->nParts * sizeof (int) );
    memcpy (stream_obj, psCShape->panPartType,  psCShape->nParts * sizeof (int) );
    
/* get X and Y coordinate arrarys */    
    memcpy (stream_obj, psCShape->padfX, psCShape->nVertices * 2 * sizeof (double) );

/* get Z coordinate array if used */
    if ( use_Z ) 
      memcpy (stream_obj, psCShape->padfZ, psCShape->nVertices * 2 * sizeof (double) );
/* get Measure coordinate array if used */
    if ( use_M ) 
      memcpy (stream_obj, psCShape->padfM, psCShape->nVertices * 2 * sizeof (double) );
   }    /* end put data without swap */
   
   return (0);
}


/* **************************************************************************
 * SHPWriteSHPStream
 *
 * Encapsulate entire SHPObject for use with Postgresql
 *
 * **************************************************************************/
int SHPWriteSHPStream ( WKBStreamObj *stream_obj, SHPObject *psCShape ) {

/*int	obj_storage = 0;*/
int	need_swap = 0, my_order, GeoType;
int	use_Z = 0;
int	use_M = 0;

  need_swap = 1;
  need_swap = ((char*) (&need_swap))[0];
  
  /*realloc (stream_obj, obj_storage );*/
  
  if ( need_swap ) {
  
  } else {
    memcpy (stream_obj, psCShape, 4 * sizeof (int) );
    memcpy (stream_obj, psCShape, 4 * sizeof (double) );
    if ( use_Z ) 
      memcpy (stream_obj, psCShape, 2 * sizeof (double) );
    if ( use_M ) 
      memcpy (stream_obj, psCShape, 2 * sizeof (double) );
          
    memcpy (stream_obj, psCShape, psCShape->nParts * 2 * sizeof (int) );
    memcpy (stream_obj, psCShape, psCShape->nVertices * 2 * sizeof (double) );
    if ( use_Z ) 
      memcpy (stream_obj, psCShape, psCShape->nVertices * 2 * sizeof (double) );
    if ( use_M ) 
      memcpy (stream_obj, psCShape, psCShape->nVertices * 2 * sizeof (double) );
   }
   
  return (0);
}


/* **************************************************************************
 * WKBStreamWrite
 *
 * Encapsulate entire SHPObject for use with Postgresql
 *
 * **************************************************************************/
int WKBStreamWrite ( WKBStreamObj* wso, void* this, int tcount, int tsize ) {

   if ( wso->NeedSwap )
     SwapG ( &(wso->wStream[wso->StreamPos]), this, tcount, tsize );
   else
     memcpy ( &(wso->wStream[wso->StreamPos]), this, tsize * tcount );
     
   wso->StreamPos += tsize;

   return 0;
}



/* **************************************************************************
 * WKBStreamRead
 *
 * Encapsulate entire SHPObject for use with Postgresql
 *
 * **************************************************************************/
int WKBStreamRead ( WKBStreamObj* wso, void* this, int tcount, int tsize ) {

   if ( wso->NeedSwap )
     SwapG ( this, &(wso->wStream[wso->StreamPos]), tcount, tsize );
   else
     memcpy ( this, &(wso->wStream[wso->StreamPos]), tsize * tcount );
     
   wso->StreamPos += tsize;

   return 0;
}



/* **************************************************************************
 * SHPReadOGisWKB
 *
 * Encapsulate entire SHPObject for use with Postgresql
 *
 * **************************************************************************/
SHPObject* SHPReadOGisWKB ( WKBStreamObj *stream_obj) {
  SHPObject	*psCShape;
  char		WKB_order;
  int		need_swap = 0, my_order, GeoType = 0;
  int		use_Z = 0, use_M = 0;
  int		nSHPType, thisDim;

  WKBStreamRead ( stream_obj, &WKB_order, 1, sizeof(char));
  my_order = 1;
  my_order = ((char*) (&my_order))[0];
  stream_obj->NeedSwap = !(WKB_order & my_order);

  /* convert OGis Types to SHP types  */
  nSHPType = SHPOGisType ( GeoType, 0 );

  WKBStreamRead ( stream_obj, &GeoType, 1, sizeof(int));
     
  thisDim = SHPDimension ( nSHPType );
  
  if ( thisDim && SHPD_AREA ) 
    { psCShape = SHPReadOGisPolygon ( stream_obj ); } 
   else {
    if ( thisDim && SHPD_LINE )
      { psCShape = SHPReadOGisLine ( stream_obj ); }
    else {
      if ( thisDim && SHPD_POINT )
        { psCShape = SHPReadOGisPoint ( stream_obj ); }
      }
    }
   

   return (0);
}



/* **************************************************************************
 * SHPWriteOGisWKB
 *
 * Encapsulate entire SHPObject for use with Postgresql
 *
 * **************************************************************************/
int SHPWriteOGisWKB ( WKBStreamObj* stream_obj, SHPObject *psCShape ) {

  int		need_swap = 0, my_order, GeoType, thisDim;
  int		use_Z = 0, use_M = 0;  
  char		LSB = 1;
  /* indicate that this WKB is in LSB Order	*/

  /* OGis WKB can handle either byte order,  but if I get to choose I'd
  /* rather have it predicatable system-to-system							*/

  if ( stream_obj ) {
    if ( stream_obj->wStream )
      free ( stream_obj->wStream );
   } else 
    { stream_obj = calloc ( 3, sizeof (int ) ); }

  /* object size needs to be 9 bytes for the wrapper, and for each polygon	*/
  /* another 9 bytes all plus twice the total number of vertices 		*/
  /* times the sizeof (double) and just pad with 10 more chars for fun 		*/
  stream_obj->wStream = calloc (1, (9 * (psCShape->nParts + 1)) +
  	( sizeof(double) * 2 * psCShape->nVertices ) + 10 );
  
 #ifdef DEBUG2
   printf (" I just allocated %d bytes to wkbObj \n", 
  	(int)(sizeof (int) + sizeof (int) + sizeof(int) +
        ( sizeof(int) * psCShape->nParts + 1 ) +
  	( sizeof(double) * 2 * psCShape->nVertices ) + 10) );
 #endif 
 
  my_order = 1;
  my_order = ((char*) (&my_order))[0];
  /* Need to swap if this system is not  LSB (Intel Order)					*/
  stream_obj->NeedSwap = ( my_order != LSB );

  stream_obj->StreamPos = 0;

  
  #ifdef DEBUG2
    printf ("this system is (%d) LSB recorded as needSwap %d\n",my_order, stream_obj->NeedSwap);
  #endif

  WKBStreamWrite ( stream_obj, & LSB, 1, sizeof(char) );
  
  #ifdef DEBUG2
    printf ("this system in LSB \n");
  #endif

  
  /* convert SHP Types to OGis types  */
  GeoType = SHPOGisType ( psCShape->nSHPType, 1 );
  WKBStreamWrite ( stream_obj, &GeoType, 1, sizeof(int) );
     
  thisDim = SHPDimension ( psCShape->nSHPType );
  
  if ( thisDim && SHPD_AREA ) 
    { SHPWriteOGisPolygon ( stream_obj, psCShape ); } 
   else {
    if ( thisDim && SHPD_LINE )
      { SHPWriteOGisLine ( stream_obj, psCShape ); }
    else {
      if ( thisDim && SHPD_POINT )
        { SHPWriteOGisPoint ( stream_obj, psCShape ); }
      }
    }

#ifdef DEBUG2
  printf("(SHPWriteOGisWKB) outta here when stream pos is %d \n", stream_obj->StreamPos);
#endif   
  return (0);
}


/* **************************************************************************
 * SHPWriteOGisPolygon
 *
 * for this pass code to more generic OGis MultiPolygon Type
 * later add support for OGis Polygon Type
 *
 * Encapsulate entire SHPObject for use with Postgresql
 *
 * **************************************************************************/
int SHPWriteOGisPolygon ( WKBStreamObj *stream_obj, SHPObject *psCShape ) {
   SHPObject		**ppsC;
   SHPObject		*psC;
   int			rPart, ring, rVertices, cpart, cParts, nextring, i, j;
   char			Flag = 1;
   int			GeoType = OGIST_POLYGON;
   
   /* cant have more than nParts complex objects in this object */
   ppsC = calloc ( psCShape->nParts, sizeof(int) );

   
   nextring = 0;
   cParts=0;
   while ( nextring >= 0 ) {
       ppsC[cParts] = SHPUnCompound ( psCShape, &nextring ); 
       cParts++;
    }
   
#ifdef DEBUG2
     printf ("(SHPWriteOGisPolygon) Uncompounded into %d parts \n", cParts);
#endif
      
   WKBStreamWrite ( stream_obj, &cParts, 1, sizeof(int) );

   for ( cpart = 0; cpart < cParts; cpart++) {

     WKBStreamWrite ( stream_obj, & Flag, 1, sizeof(char) );
     WKBStreamWrite ( stream_obj, & GeoType, 1, sizeof(int) );
  
     psC = (SHPObject*) ppsC[cpart];
     WKBStreamWrite ( stream_obj, &(psC->nParts), 1, sizeof(int) );    
     
     for ( ring = 0; (ring < (psC->nParts)) && (psC->nParts > 0); ring ++) {
       if ( ring < (psC->nParts-2) )
         { rVertices = psC->panPartStart[ring+1] - psC->panPartStart[ring]; }
       else
         { rVertices = psC->nVertices - psC->panPartStart[ring]; }
#ifdef DEBUG2
     printf ("(SHPWriteOGisPolygon) scanning part %d, ring %d %d vtxs \n", 
     		cpart, ring, rVertices);
#endif             
       rPart = psC->panPartStart[ring];
       WKBStreamWrite ( stream_obj, &rVertices, 1, sizeof(int) );       
       for ( j=rPart; j < (rPart + rVertices); j++ ) {
         WKBStreamWrite ( stream_obj, &(psC->padfX[j]), 1, sizeof(double) );
         WKBStreamWrite ( stream_obj, &(psC->padfY[j]), 1, sizeof(double) );
        } /* for each vertex */
      }  /* for each ring */
    }  /* for each complex part */

#ifdef DEBUG2
     printf ("(SHPWriteOGisPolygon) outta here \n");
#endif   
  return (1);
}


/* **************************************************************************
 * SHPWriteOGisLine
 *
 * for this pass code to more generic OGis MultiXXXXXXX Type
 * later add support for OGis LineString Type
 *
 * Encapsulate entire SHPObject for use with Postgresql
 *
 * **************************************************************************/
int SHPWriteOGisLine ( WKBStreamObj *stream_obj, SHPObject *psCShape ) {

  return ( SHPWriteOGisPolygon( stream_obj, psCShape ));
}


/* **************************************************************************
 * SHPWriteOGisPoint
 *
 * for this pass code to more generic OGis MultiPoint Type
 * later add support for OGis Point Type
 *
 * Encapsulate entire SHPObject for use with Postgresql
 *
 * **************************************************************************/
int SHPWriteOGisPoint ( WKBStreamObj *stream_obj, SHPObject *psCShape ) {
   int			j;
   
   WKBStreamWrite ( stream_obj, &(psCShape->nVertices), 1, sizeof(int) );

   for ( j=0; j < psCShape->nVertices; j++ ) {
     WKBStreamWrite ( stream_obj, &(psCShape->padfX[j]), 1, sizeof(double) );
     WKBStreamWrite ( stream_obj, &(psCShape->padfY[j]), 1, sizeof(double) );
    } /* for each vertex */

  return (1);
}



/* **************************************************************************
 * SHPReadOGisPolygon
 *
 * for this pass code to more generic OGis MultiPolygon Type
 * later add support for OGis Polygon Type
 *
 * Encapsulate entire SHPObject for use with Postgresql
 *
 * **************************************************************************/
SHPObject* SHPReadOGisPolygon ( WKBStreamObj *stream_obj ) {
   SHPObject		**ppsC;
   SHPObject		*psC;
   int			rPart, ring, rVertices, cpart, cParts, nextring, i, j;
   int			totParts, totVertices, pRings, nParts;
   
   psC = SHPCreateObject ( SHPT_POLYGON, -1, 0, NULL, NULL, 0,
        	NULL, NULL, NULL, NULL ); 
   /* initialize a blank SHPObject 											*/        	

   WKBStreamRead ( stream_obj, &cParts, 1, sizeof(char) );

   totParts = cParts;
   totVertices = 0;
   
   SfRealloc ( psC->panPartStart, cParts * sizeof(int));
   SfRealloc ( psC->panPartType, cParts * sizeof(int));

   for ( cpart = 0; cpart < cParts; cpart++) {
     WKBStreamRead ( stream_obj, &nParts, 1, sizeof(int) );
     pRings = nParts;     
     /* pRings is the number of rings prior to the Ring loop below			*/
     
     if ( nParts > 1 ) {
       totParts += nParts - 1;
       SfRealloc ( psC->panPartStart, totParts * sizeof(int));
       SfRealloc ( psC->panPartType, totParts * sizeof(int));
      }

     rPart = 0;
     for ( ring = 0; ring < (nParts - 1); ring ++) {
       WKBStreamRead ( stream_obj, &rVertices, 1, sizeof(int) );
       totVertices += rVertices;
               
       psC->panPartStart[ring+pRings] = rPart;
       if ( ring == 0 )
         { psC->panPartType[ring + pRings] = SHPP_OUTERRING; }
        else
         { psC->panPartType[ring + pRings] = SHPP_INNERRING; }

       SfRealloc ( psC->padfX, totVertices * sizeof (double));
       SfRealloc ( psC->padfY, totVertices * sizeof (double));
              
       for ( j=rPart; j < (rPart + rVertices); j++ ) {
         WKBStreamRead ( stream_obj, &(psC->padfX[j]), 1, sizeof(double) );
         WKBStreamRead ( stream_obj, &(psC->padfY[j]), 1, sizeof(double) );        
        } /* for each vertex */
       rPart += rVertices;

      }  /* for each ring */
      
    }  /* for each complex part */
    
    return ( psC );
    
}


/* **************************************************************************
 * SHPReadOGisLine
 *
 * for this pass code to more generic OGis MultiLineString Type
 * later add support for OGis LineString Type
 * 
 * Encapsulate entire SHPObject for use with Postgresql
 *
 * **************************************************************************/
SHPObject* SHPReadOGisLine ( WKBStreamObj *stream_obj ) {
   SHPObject		**ppsC;
   SHPObject		*psC;
   int			rPart, ring, rVertices, cpart, cParts, nextring, i, j;
   int			totParts, totVertices, pRings, nParts;
   
   psC = SHPCreateObject ( SHPT_ARC, -1, 0, NULL, NULL, 0,
        	NULL, NULL, NULL, NULL ); 
   /* initialize a blank SHPObject 											*/        	

   WKBStreamRead ( stream_obj, &cParts, 1, sizeof(int) );

   totParts = cParts;
   totVertices = 0;
   
   SfRealloc ( psC->panPartStart, cParts * sizeof(int));
   SfRealloc ( psC->panPartType, cParts * sizeof(int));

   for ( cpart = 0; cpart < cParts; cpart++) {
     WKBStreamRead ( stream_obj, &nParts, 1, sizeof(int) );
     pRings = totParts;     
     /* pRings is the number of rings prior to the Ring loop below			*/
     
     if ( nParts > 1 ) {
       totParts += nParts - 1;
       SfRealloc ( psC->panPartStart, totParts * sizeof(int));
       SfRealloc ( psC->panPartType, totParts * sizeof(int));
      }

     rPart = 0;
     for ( ring = 0; ring < (nParts - 1); ring ++) {
       WKBStreamRead ( stream_obj, &rVertices, 1, sizeof(int) );
       totVertices += rVertices;
               
       psC->panPartStart[ring+pRings] = rPart;
       if ( ring == 0 )
         { psC->panPartType[ring + pRings] = SHPP_OUTERRING; }
        else
         { psC->panPartType[ring + pRings] = SHPP_INNERRING; }

       SfRealloc ( psC->padfX, totVertices * sizeof (double));
       SfRealloc ( psC->padfY, totVertices * sizeof (double));
              
       for ( j=rPart; j < (rPart + rVertices); j++ ) {
         WKBStreamRead ( stream_obj, &(psC->padfX[j]), 1, sizeof(double) );
         WKBStreamRead ( stream_obj, &(psC->padfY[j]), 1, sizeof(double) );        
        } /* for each vertex */
       rPart += rVertices;

      }  /* for each ring */
      
    }  /* for each complex part */
    
    return ( psC );
}


/* **************************************************************************
 * SHPReadOGisPoint
 *
 * Encapsulate entire SHPObject for use with Postgresql
 *
 * **************************************************************************/
SHPObject* SHPReadOGisPoint ( WKBStreamObj *stream_obj ) {
   SHPObject		*psC;
   int			nVertices, j;
   
   psC = SHPCreateObject ( SHPT_MULTIPOINT, -1, 0, NULL, NULL, 0,
        	NULL, NULL, NULL, NULL ); 
   /* initialize a blank SHPObject 											*/        	

   WKBStreamRead ( stream_obj, &nVertices, 1, sizeof(int) );

   SfRealloc ( psC->padfX, nVertices * sizeof (double));
   SfRealloc ( psC->padfY, nVertices * sizeof (double));
              
   for ( j=0; j < nVertices; j++ ) {
     WKBStreamRead ( stream_obj, &(psC->padfX[j]), 1, sizeof(double) );
     WKBStreamRead ( stream_obj, &(psC->padfY[j]), 1, sizeof(double) );        
    } /* for each vertex */
    
    return ( psC );
}




/* **************************************************************************
 * RingReadOGisWKB  
 *
 * this accepts OGisLineStrings which are basic building blocks
 *
 * Encapsulate entire SHPObject for use with Postgresql
 *
 * **************************************************************************/
int RingReadOgisWKB ( SHPObject *psCShape, char *stream_obj) {
    return 0;
}


/* **************************************************************************
 * RingWriteOGisWKB
 *
 * this emits OGisLineStrings which are basic building blocks
 * 
 * Encapsulate entire SHPObject for use with Postgresql
 *
 * **************************************************************************/
int RingWriteOgisWKB ( SHPObject *psCShape, char *stream_obj) {

    return 0;
}


/* **************************************************************************
 * SHPDimension
 *
 * Return the Dimensionality of the SHPObject 
 * a handy utility function
 * 
 * **************************************************************************/
int SHPDimension ( int SHPType ) {
    int 	dimension;
    
    dimension = 0;
      
    switch ( SHPType ) {
      case  SHPT_POINT       :	dimension = SHPD_POINT; break;
      case  SHPT_ARC         :	dimension = SHPD_LINE; break;  
      case  SHPT_POLYGON     :	dimension = SHPD_AREA; break;   	
      case  SHPT_MULTIPOINT  :	dimension = SHPD_POINT; break;
      case  SHPT_POINTZ      :	dimension = SHPD_POINT | SHPD_Z; break;
      case  SHPT_ARCZ        :	dimension = SHPD_LINE | SHPD_Z; break;
      case  SHPT_POLYGONZ    :	dimension = SHPD_AREA | SHPD_Z; break;
      case  SHPT_MULTIPOINTZ :	dimension = SHPD_POINT | SHPD_Z; break;
      case  SHPT_POINTM      :	dimension = SHPD_POINT | SHPD_MEASURE; break;
      case  SHPT_ARCM        :	dimension = SHPD_LINE | SHPD_MEASURE; break;
      case  SHPT_POLYGONM    :	dimension = SHPD_AREA | SHPD_MEASURE; break;
      case  SHPT_MULTIPOINTM :	dimension = SHPD_POINT | SHPD_MEASURE; break;
      case  SHPT_MULTIPATCH  :	dimension = SHPD_AREA; break;
    }

    return ( dimension );
}


/* **************************************************************************
 * SHPPointinPoly_2d
 *
 * Return a Point inside an R+ of a potentially 
 * complex/compound SHPObject suitable for labelling
 * return only one point even if if is a compound object
 *
 * reject non area SHP Types
 * 
 * **************************************************************************/
PT	SHPPointinPoly_2d ( SHPObject *psCShape ) {
   PT	*sPT, rPT;
   
   if ( !(SHPDimension (psCShape->nSHPType) & SHPD_AREA) )
   {
       rPT.x = NAN;
       rPT.y = NAN;
       return rPT;
   }

   sPT = SHPPointsinPoly_2d ( psCShape );
   
   if ( sPT ) {
     rPT.x = sPT[0].x;
     rPT.y = sPT[0].y; 
    } else {
     rPT.x = NAN;
     rPT.y = NAN;
    }
   return ( rPT );
}


/* **************************************************************************
 * SHPPointsinPoly_2d
 *
 * Return a Point inside each R+ of a potentially 
 * complex/compound SHPObject suitable for labelling
 * return one point for each R+ even if it is a compound object
 *
 * reject non area SHP Types
 * 
 * **************************************************************************/
PT*	SHPPointsinPoly_2d ( SHPObject *psCShape ) {
   PT		*PIP = NULL;
   int		cRing;
   SHPObject	*psO, *psInt, *CLine;
   double	*CLx, *CLy;
   int		*CLstt, *CLst, nPIP, ring, rMpart, ring_vtx, ring_nVertices;
   double	rLen, rLenMax;

   if ( !(SHPDimension (psCShape->nSHPType) & SHPD_AREA) )  
      return ( NULL );

   while (  psO = SHPUnCompound (psCShape, &cRing)) {
     CLx = calloc ( 4, sizeof(double));
     CLy = calloc ( 4, sizeof(double));
     CLst = calloc ( 2, sizeof(int));
     CLstt = calloc ( 2, sizeof(int));
     
     /* a horizontal & vertical compound line though the middle of the 		*/
     /* extents 															*/
     CLx [0] = psO->dfXMin;
     CLy [0] = (psO->dfYMin + psO->dfYMax ) * 0.5;
     CLx [1] = psO->dfXMax;   
     CLy [1] = (psO->dfYMin + psO->dfYMax ) * 0.5;
   
     CLx [2] = (psO->dfXMin + psO->dfXMax ) * 0.5;
     CLy [2] = psO->dfYMin;   
     CLx [3] = (psO->dfXMin + psO->dfXMax ) * 0.5;
     CLy [3] = psO->dfYMax;
     
     CLst[0] = 0;   CLst[1] = 2; 
     CLstt[0] = SHPP_RING; CLstt[1] = SHPP_RING;      
     
     CLine = SHPCreateObject ( SHPT_POINT, -1, 2, CLst, CLstt, 4,
        	CLx, CLy, NULL, NULL ); 

     /* with the H & V centrline compound object, intersect it with the OBJ	*/
     psInt = SHPIntersect_2d ( CLine, psO );
     /* return SHP type is lowest common dimensionality of the input types 	*/


     /* find the longest linestring returned by the intersection			*/
     ring_vtx = psInt->nVertices ;
     for ( ring = (psInt->nParts - 1); ring >= 0; ring-- ) {
       ring_nVertices = ring_vtx - psInt->panPartStart[ring];

       rLen += RingLength_2d ( ring_nVertices, 
     	(double*) &(psInt->padfX [psInt->panPartStart[ring]]), 
     	(double*) &(psInt->padfY [psInt->panPartStart[ring]]) );
 
       if ( rLen > rLenMax )  
         { rLenMax = rLen;
           rMpart = psInt->panPartStart[ring];
         }
       ring_vtx = psInt->panPartStart[ring];
      } 

     /* add the centerpoint of the longest ARC of the intersection to the	*/
     /*	PIP list															*/
     nPIP ++;
     SfRealloc ( PIP, sizeof(double) * 2 * nPIP);
     PIP[nPIP].x = (psInt ->padfX [rMpart] + psInt ->padfX [rMpart]) * 0.5;
     PIP[nPIP].y = (psInt ->padfY [rMpart] + psInt ->padfY [rMpart]) * 0.5;
     
     SHPDestroyObject ( psO );
     SHPDestroyObject ( CLine );
     
     /* does SHPCreateobject use preallocated memory or does it copy the 	*/
     /* contents.  To be safe conditionally release CLx, CLy, CLst, CLstt	*/
     if ( CLx )   free ( CLx );
     if ( CLy )   free ( CLy );
     if ( CLst )  free ( CLst );
     if ( CLstt ) free ( CLstt );
    }
    
  return ( PIP );
}


/* **************************************************************************
 * SHPCentrd_2d
 *
 * Return the single mathematical / geometric centroid of a potentially 
 * complex/compound SHPObject
 *
 * reject non area SHP Types
 * 
 * **************************************************************************/
PT SHPCentrd_2d ( SHPObject *psCShape ) {
    int		ring, ringPrev, ring_nVertices, rStart;
    double	Area, ringArea;
    PT		ringCentrd, C;
    
  
   if ( !(SHPDimension (psCShape->nSHPType) & SHPD_AREA) )
   {
       C.x = NAN;
       C.y = NAN;
       return C;
   }

#ifdef DEBUG
	printf ("for Object with %d vtx, %d parts [ %d, %d] \n",
		psCShape->nVertices, psCShape->nParts,
		psCShape->panPartStart[0],psCShape->panPartStart[1]);
#endif
   
   Area = 0;
   C.x = 0.0;
   C.y = 0.0;
   
   /* for each ring in compound / complex object calc the ring cntrd		*/
   
   ringPrev = psCShape->nVertices;
   for ( ring = (psCShape->nParts - 1); ring >= 0; ring-- ) {
     rStart = psCShape->panPartStart[ring];
     ring_nVertices = ringPrev - rStart;

     RingCentroid_2d ( ring_nVertices, (double*) &(psCShape->padfX [rStart]), 
     	(double*) &(psCShape->padfY [rStart]), &ringCentrd, &ringArea);  

#ifdef DEBUG
	printf ("(SHPCentrd_2d)  Ring %d, vtxs %d, area: %f, ring centrd %f, %f \n",
		ring, ring_nVertices, ringArea, ringCentrd.x, ringCentrd.y);
#endif

     /* use Superposition of these rings to build a composite Centroid		*/
     /* sum the ring centrds * ringAreas,  at the end divide by total area	*/
     C.x +=  ringCentrd.x * ringArea;
     C.y +=  ringCentrd.y * ringArea; 
     Area += ringArea; 
     ringPrev = rStart;
    }    

     /* hold on the division by AREA until were at the end					*/
     C.x = C.x / Area;
     C.y = C.y / Area;
#ifdef DEBUG
	printf ("SHPCentrd_2d) Overall Area: %f, Centrd %f, %f \n",
		Area, C.x, C.y);
#endif   
   return ( C );
}


/* **************************************************************************
 * RingCentroid_2d
 *
 * Return the mathematical / geometric centroid of a single closed ring
 *
 * **************************************************************************/
int RingCentroid_2d ( int nVertices, double *a, double *b, PT *C, double *Area ) {
  int		iv,jv;
  int		sign_x, sign_y;
  double	dy_Area, dx_Area, Cx_accum, Cy_accum, ppx, ppy;
  double 	x_base, y_base, x, y;

/* the centroid of a closed Ring is defined as
 *
 *      Cx = sum (cx * dArea ) / Total Area
 *  and
 *      Cy = sum (cy * dArea ) / Total Area
 */      
   
  x_base = a[0];
  y_base = b[0];
  
  Cy_accum = 0.0;
  Cx_accum = 0.0;

  ppx = a[1] - x_base;
  ppy = b[1] - y_base;
  *Area = 0;

/* Skip the closing vector */
  for ( iv = 2; iv <= nVertices - 2; iv++ ) {
    x = a[iv] - x_base;
    y = b[iv] - y_base;

    /* calc the area and centroid of triangle built out of an arbitrary 	*/
    /* base_point on the ring and each successive pair on the ring			*/
    
    /* Area of a triangle is the cross product of its defining vectors		*/
    /* Centroid of a triangle is the average of its vertices				*/

    dx_Area =  ((x * ppy) - (y * ppx)) * 0.5;
    *Area += dx_Area;
    
    Cx_accum += ( ppx + x ) * dx_Area;       
    Cy_accum += ( ppy + y ) * dx_Area;
#ifdef DEBUG2
    printf("(ringcentrd_2d)  Pp( %f, %f), P(%f, %f)\n", ppx, ppy, x, y);
    printf("(ringcentrd_2d)    dA: %f, sA: %f, Cx: %f, Cy: %f \n", 
		dx_Area, *Area, Cx_accum, Cy_accum);
#endif    
    ppx = x;
    ppy = y;
  }

#ifdef DEBUG2
  printf("(ringcentrd_2d)  Cx: %f, Cy: %f \n", 
  	( Cx_accum / ( *Area * 3) ), ( Cy_accum / (*Area * 3) ));
#endif

  /* adjust back to world coords 											*/
  C->x = ( Cx_accum / ( *Area * 3)) + x_base;
  C->y = ( Cy_accum / ( *Area * 3)) + y_base;
      
  return ( 1 );
}




/* **************************************************************************
 * SHPRingDir_2d
 *
 * Test Polygon for CW / CCW  ( R+ / R- )
 *
 * return 1  for R+
 * return -1 for R-
 * return 0  for error
 * **************************************************************************/
int SHPRingDir_2d ( SHPObject *psCShape, int Ring ) {
   int		i, ti, last_vtx;
   double	tX;
   double 	*a, *b;
   double	dx0, dx1, dy0, dy1, v1, v2 ,v3;
   
   tX = 0.0;
   a = psCShape->padfX;
   b = psCShape->padfY;
   
   if ( Ring >= psCShape->nParts ) return ( 0 );
   
   if ( Ring >= psCShape->nParts -1 )
     { last_vtx = psCShape->nVertices; }
    else
     { last_vtx = psCShape->panPartStart[Ring + 1]; }
      
   /* All vertices at the corners of the extrema (rightmost lowest, leftmost lowest, 	*/
   /* topmost rightest, ...) must be less than pi wide.  If they werent they couldnt be	*/
   /* extrema.																			*/   
   /* of course the following will fail if the Extents are even a little wrong 			*/      
   
   for ( i = psCShape->panPartStart[Ring]; i < last_vtx; i++ ) {
     if ( b[i] == psCShape->dfYMax && a[i] > tX ) 
      { ti = i; }
   }

#ifdef DEBUG2
   printf ("(shpgeo:SHPRingDir) highest Rightmost Pt is vtx %d (%f, %f)\n", ti, a[ti], b[ti]);
#endif   
   
   /* cross product */
   /* the sign of the cross product of two vectors indicates the right or left half-plane	*/
   /* which we can use to indicate Ring Dir													*/ 
   if ( ti > psCShape->panPartStart[Ring] & ti < last_vtx ) 
    { dx0 = a[ti-1] - a[ti];
      dx1 = a[ti+1] - a[ti];
      dy0 = b[ti-1] - b[ti];
      dy1 = b[ti+1] - b[ti];
   }
   else
   /* if the tested vertex is at the origin then continue from 0 */ 
   {  dx1 = a[1] - a[0];
      dx0 = a[last_vtx] - a[0];
      dy1 = b[1] - b[0];
      dy0 = b[last_vtx] - b[0];
   }
   
//   v1 = ( (dy0 * 0) - (0 * dy1) );
//   v2 = ( (0 * dx1) - (dx0 * 0) );
/* these above are always zero so why do the math */
   v3 = ( (dx0 * dy1) - (dx1 * dy0) );

#ifdef DEBUG2   
   printf ("(shpgeo:SHPRingDir)  cross product for vtx %d was %f \n", ti, v3); 
#endif

   if ( v3 > 0 )
    { return (1); }
   else
    { return (-1); }
}



/* **************************************************************************
 * SHPArea_2d
 *
 * Calculate the XY Area of Polygon ( can be compound / complex )
 *
 * **************************************************************************/
double SHPArea_2d ( SHPObject *psCShape ) {
    double 	cArea;
    int		ring, ring_vtx, ringDir, ring_nVertices;
  
   cArea = 0;
   if ( !(SHPDimension (psCShape->nSHPType) & SHPD_AREA) )  
       return ( -1 );
 
 
   /* Walk each ring adding its signed Area,  R- will return a negative 	*/
   /* area, so we don't have to test for them								*/

   /* I just start at the last ring and work down to the first				*/
   ring_vtx = psCShape->nVertices ;
   for ( ring = (psCShape->nParts - 1); ring >= 0; ring-- ) {
     ring_nVertices = ring_vtx - psCShape->panPartStart[ring];

#ifdef DEBUG2
     printf("(shpgeo:SHPArea_2d) part %d, vtx %d \n", ring,  ring_nVertices);
#endif    
     cArea += RingArea_2d ( ring_nVertices, 
     	(double*) &(psCShape->padfX [psCShape->panPartStart[ring]]), 
     	(double*) &(psCShape->padfY [psCShape->panPartStart[ring]]) );

     ring_vtx = psCShape->panPartStart[ring];
    } 

#ifdef DEBUG2
    printf ("(shpgeo:SHPArea_2d) Area = %f \n", cArea);
#endif

    /* Area is signed, negative Areas are R-									*/    
    return ( cArea );
    
}


/* **************************************************************************
 * SHPLength_2d
 *
 * Calculate the Planar ( XY ) Length of Polygon ( can be compound / complex )
 *    or Polyline ( can be compound ).  Length on Polygon is its Perimeter
 *
 * **************************************************************************/
double SHPLength_2d ( SHPObject *psCShape ) {
    double 	Length;
    int		i, j;
    double 	dx, dy;
    
   if ( !(SHPDimension (psCShape->nSHPType) & (SHPD_AREA || SHPD_LINE)) )  
       return ( (double) -1 );    
    
    Length = 0;
    j = 1;
    for ( i = 1; i < psCShape->nVertices; i++ ) {  
      if ( psCShape->panPartStart[j] == i ) 
       { j ++; }
    /* skip the moves with "pen up" from ring to ring */       
      else
       { 
        dx = psCShape->padfX[i] - psCShape->padfX[i-1];
        dy = psCShape->padfY[i] - psCShape->padfY[i-1];
        Length += sqrt ( ( dx * dx ) + ( dy * dy ) );
       }
     /* simplify this equation */
     }
     
   return ( Length );
}


/* **************************************************************************
 * RingLength_2d
 *
 * Calculate the Planar ( XY ) Length of Polygon ( can be compound / complex )
 *    or Polyline ( can be compound ).  Length of Polygon is its Perimeter
 *
 * **************************************************************************/
double RingLength_2d ( int nVertices, double *a, double *b ) {
    double 	Length;
    int		i, j;
    double 	dx, dy;
    
    Length = 0;
    j = 1;
    for ( i = 1; i < nVertices; i++ ) {  
      dx = a[i] - b[i-1];
      dy = b[i] - b[i-1];
      Length += sqrt ( ( dx * dx ) + ( dy * dy ) );
     /* simplify this equation */
     }
     
   return ( Length );
}


/* **************************************************************************
 * RingArea_2d
 *
 * Calculate the Planar Area of a single closed ring 
 *
 * **************************************************************************/
double RingArea_2d ( int nVertices, double *a, double *b ) {
  int		iv,jv;
  double	ppx, ppy;
  static 	double	Area;
  double	dx_Area;
  double 	x_base, y_base, x, y;
  
  x_base = a[0];
  y_base = b[0];
  
  ppx = a[1] - x_base;
  ppy = b[1] - y_base;
  Area = 0.0;
#ifdef DEBUG2  
  printf("(shpgeo:RingArea) %d vertices \n", nVertices);
#endif  
  for ( iv = 2; iv <= ( nVertices - 1 ); iv++ ) {
    x = a[iv] - x_base;
    y = b[iv] - y_base;

    /* Area of a triangle is the cross product of its defining vectors		*/
    
    dx_Area = ((x * ppy) - (y * ppx)) * 0.5;

    Area += dx_Area;
#ifdef DEBUG2
    printf ("(shpgeo:RingArea)  dxArea %f  sArea %f for pt(%f, %f)\n", 
    		dx_Area, Area, x, y);
#endif
    		
    ppx = x;
    ppy = y;
  }

#ifdef DEBUG2
  printf ("(shpgeo:RingArea)  total RingArea %f \n", Area);
#endif  
  return ( Area );

}



/* **************************************************************************
 * SHPUnCompound
 *
 * ESRI calls this function explode
 * Return a non compound ( possibly complex ) object
 *
 * ring_number is R+ number corresponding to object
 *
 *
 * ignore complexity in Z dimension for now
 *
 * **************************************************************************/
SHPObject* SHPUnCompound  ( SHPObject *psCShape, int * ringNumber ) {
   int 		ringDir, ring, lRing;
   
   if ( (*ringNumber >= psCShape->nParts) || *ringNumber == -1 ) {
 	*ringNumber = -1;	
	return (NULL);
      }

   
   if ( *ringNumber == (psCShape->nParts - 1) )  {
        *ringNumber =  -1;
        return ( SHPClone(psCShape, (psCShape->nParts - 1), -1) );
      }

   lRing = *ringNumber;
   ringDir = -1;
   for ( ring = (lRing + 1); (ring < psCShape->nParts) && ( ringDir < 0 ); ring ++)
     ringDir = SHPRingDir_2d ( psCShape, ring);
   
   if ( ring ==  psCShape->nParts )
     *ringNumber = -1; 
   else
     *ringNumber = ring;
/*    I am strictly assuming that all R- parts of a complex object 
 *	   directly follow their R+, so when we hit a new R+ its a
 *	   new part of a compound object
 *         a SHPClean may be needed to enforce this as it is not part
 *	   of ESRI's definition of a SHPfile
 */

#ifdef DEBUG2    
    printf ("(SHPUnCompound) asked for ring %d, lastring is %d \n", lRing, ring);
#endif
    return ( SHPClone(psCShape, lRing, ring ) );

}


/* **************************************************************************
 * SHPIntersect_2d
 *
 * 
 * prototype only for now
 *
 * return object with lowest common dimensionality of objects
 * 
 * **************************************************************************/ 
SHPObject* SHPIntersect_2d ( SHPObject* a, SHPObject* b ) {
  SHPObject	*C;
  
  if ( (SHPDimension(a->nSHPType) && SHPD_POINT) || ( SHPDimension(b->nSHPType) && SHPD_POINT ) )
    return ( NULL );
  /* there is no intersect function like this for points  */
    
  C = SHPClone ( a, 0 , -1 );

  return ( C);

}



/* **************************************************************************
 * SHPClean
 *
 * Test and fix normalization problems in shapes
 * Different tests need to be implemented for different SHPTypes
 * 	SHPT_POLYGON	check ring directions CW / CCW   ( R+ / R- )
 *				put all R- after the R+ they are members of
 *				i.e. each complex object is completed before the
 *     				next object is started
 *				check for closed rings
 *				ring must not intersect itself, even on edge
 *
 *  no other types implemented yet
 * 
 * not sure why but return object in place
 * use for object casting and object verification						 
 * **************************************************************************/ 
int SHPClean ( SHPObject *psCShape ) {


    return (0);
}


/* **************************************************************************
 * SHPClone
 *
 * Clone a SHPObject, replicating all data
 * 
 * **************************************************************************/ 
SHPObject* SHPClone ( SHPObject *psCShape, int lowPart, int highPart ) {
    SHPObject	*psObject;
    int		newParts, newVertices;
#ifdef DEBUG
    int     	i;
#endif

    if ( highPart >= psCShape->nParts || highPart == -1 )
	highPart = psCShape->nParts ;

#ifdef DEBUG
    printf (" cloning SHP (%d parts) from ring %d to ring %d \n",
	 psCShape->nParts, lowPart, highPart);
#endif

    newParts = highPart - lowPart;
    if ( newParts == 0 ) { return ( NULL ); }
    
    psObject = (SHPObject *) calloc(1,sizeof(SHPObject));
    psObject->nSHPType = psCShape->nSHPType;
    psObject->nShapeId = psCShape->nShapeId;
    
    psObject->nParts = newParts;
    if ( psCShape->padfX ) {
        psObject->panPartStart = (int*) calloc (newParts, sizeof (int)); 
        memcpy ( psObject->panPartStart, psCShape->panPartStart, 
      		newParts * sizeof (int) );
     }
    if ( psCShape->padfX ) {
      psObject->panPartType = (int*) calloc (newParts, sizeof (int)); 
      memcpy ( psObject->panPartType, 
  		(int *) &(psCShape->panPartType[lowPart]), 
      		newParts * sizeof (int) );
     }

    if ( highPart != psCShape->nParts ) {
      newVertices = psCShape->panPartStart[highPart] -
	 psCShape->panPartStart[lowPart]; 
     }
    else 
     { newVertices = psCShape->nVertices - psCShape->panPartStart[lowPart]; }


#ifdef DEBUG
    if ( highPart = psCShape->nParts ) 
      i = psCShape->nVertices;
     else
      i = psCShape->panPartStart[highPart];
    printf (" from part %d (%d) to %d (%d) is %d vertices \n",
	 lowPart, psCShape->panPartStart[lowPart], highPart,
	 i, newVertices);
#endif
    psObject->nVertices = newVertices;
    if ( psCShape->padfX ) {
      psObject->padfX = (double*) calloc (newVertices, sizeof (double)); 
      memcpy ( psObject->padfX,
	 (double *) &(psCShape->padfX[psCShape->panPartStart[lowPart]]),
      		newVertices * sizeof (double) );
     } 
    if ( psCShape->padfY ) {
      psObject->padfY = (double*) calloc (newVertices, sizeof (double)); 
      memcpy ( psObject->padfY, 
	 (double *) &(psCShape->padfY[psCShape->panPartStart[lowPart]]),
      		newVertices * sizeof (double) );
     }
    if ( psCShape->padfZ ) {
      psObject->padfZ = (double*) calloc (newVertices, sizeof (double)); 
      memcpy ( psObject->padfZ, 
	 (double *) &(psCShape->padfZ[psCShape->panPartStart[lowPart]]),
      		newVertices * sizeof (double) );
     }
    if ( psCShape->padfM ) {
      psObject->padfM = (double*) calloc (newVertices, sizeof (double)); 
      memcpy ( psObject->padfM,
	(double *) &(psCShape->padfM[psCShape->panPartStart[lowPart]]),
      		newVertices * sizeof (double) );
     }

    psObject->dfXMin = psCShape->dfXMin;
    psObject->dfYMin = psCShape->dfYMin;
    psObject->dfZMin = psCShape->dfZMin;
    psObject->dfMMin = psCShape->dfMMin;

    psObject->dfXMax = psCShape->dfXMax;
    psObject->dfYMax = psCShape->dfYMax;
    psObject->dfZMax = psCShape->dfZMax;
    psObject->dfMMax = psCShape->dfMMax;

    SHPComputeExtents ( psObject );
    return ( psObject );
}



/************************************************************************/
/*  SwapG 							                              	*/
/*                                                                      */
/*      Swap a 2, 4 or 8 byte word.                                     */
/************************************************************************/
void SwapG( void *so, void *in, int this_cnt, int this_size ) {
    int		i, j;
    unsigned char	temp;

/* return to a new pointer otherwise it would invalidate existing data	*/
/* as prevent further use of it											*/

    for( j=0; j < this_cnt; j++ )
     {
      for( i=0; i < this_size/2; i++ )
      {
	((unsigned char *) so)[i] = ((unsigned char *) in)[this_size-i-1];
	((unsigned char *) so)[this_size-i-1] = ((unsigned char *) in)[i];
      }
    }
}


/* **************************************************************************
 * SwapW
 *
 * change byte order on an array of 16 bit words
 * need to change this over to shapelib, Frank Warmerdam's functions
 *
 * **************************************************************************/ 
void swapW (void *so, unsigned char *in, long bytes) {
  int i, j;
  unsigned char map[4] = {3,2,1,0};
  unsigned char *out;

  out = so;
  for (i=0; i <= (bytes/4); i++)
   for (j=0; j < 4; j++)
      out[(i*4)+map[j]] = in[(i*4)+j]; 
}


/* **************************************************************************
 * SwapD
 *
 * change byte order on an array of (double) 32 bit words
 * need to change this over to shapelib, Frank Warmerdam's functons
 *
 * **************************************************************************/ 
void swapD (void *so, unsigned char *in, long bytes) {
  int i, j;
  unsigned char map[8] = {7,6,5,4,3,2,1,0};
  unsigned char *out;

  out = so;
  for (i=0; i <= (bytes/8); i++)
   for (j=0; j < 8; j++)
      out[(i*8)+map[j]] = in[(i*8)+j]; 
}

