/*****************************************************************************
 *  Copyright (c) 2008, University of Florida.
 *  All rights reserved.
 *  
 *  This file is part of OpenJAUS.  OpenJAUS is distributed under the BSD 
 *  license.  See the LICENSE file for details.
 * 
 *  Redistribution and use in source and binary forms, with or without 
 *  modification, are permitted provided that the following conditions 
 *  are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of the University of Florida nor the names of its 
 *       contributors may be used to endorse or promote products derived from 
 *       this software without specific prior written permission.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 *   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 *   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 *   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 *   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 ****************************************************************************/
/*******************************************************************************
NAME                            UNIVERSAL TRANSVERSE MERCATOR

PURPOSE:	Transforms input longitude and latitude to Easting and
		Northing for the Universal Transverse Mercator projection.
		The longitude and latitude must be in radians.  The Easting
		and Northing values will be returned in meters.

PROGRAMMER              DATE		REASON
----------              ----		------
D. Steinwand, EROS      Nov, 1991
T. Mittan		Mar, 1993
S. Nelson		Feb, 1995	Divided tmfor.c into two files, one
					for UTM (utmfor.c) and one for 
					TM (tmfor.c).  This was a
					necessary change to run forward
					projection conversions for both
					UTM and TM in the same process.

ALGORITHM REFERENCES

1.  Snyder, John P., "Map Projections--A Working Manual", U.S. Geological
    Survey Professional Paper 1395 (Supersedes USGS Bulletin 1532), United
    State Government Printing Office, Washington D.C., 1987.

2.  Snyder, John P. and Voxland, Philip M., "An Album of Map Projections",
    U.S. Geological Survey Professional Paper 1453 , United State Government
    Printing Office, Washington D.C., 1989.
*******************************************************************************/
#include <stdlib.h>
#include "utm/utmLib.h"

/* Variables common to all subroutines in this code file
  -----------------------------------------------------*/
static double r_major;		/* major axis 				*/
static double r_minor;		/* minor axis 				*/
static double scale_factor;	/* scale factor				*/
static double lon_center;	/* Center longitude (projection center) */
static double lat_origin;	/* center latitude			*/
static double e0,e1,e2,e3;	/* eccentricity constants		*/
static double e,es,esp;		/* eccentricity constants		*/
static double ml0;		/* small value m			*/
static double false_northing;	/* y offset in meters			*/
static double false_easting;	/* x offset in meters			*/
static double ind;		/* spherical flag			*/

/* Initialize the Universal Transverse Mercator (UTM) projection
  -------------------------------------------------------------*/
long utmforint(r_maj,r_min,scale_fact,zone)

double r_maj;			/* major axis				*/
double r_min;			/* minor axis				*/
double scale_fact;		/* scale factor				*/
long   zone;			/* zone number				*/
{
double temp;			/* temporary variable			*/

if ((abs(zone) < 1) || (abs(zone) > 60))
   {
	p_error("Illegal zone number","utm-forint");
	return(11);
   }
r_major = r_maj;
r_minor = r_min;
scale_factor = scale_fact;
lat_origin = 0.0;
lon_center = ((6 * abs(zone)) - 183) * D2R;
false_easting = 500000.0;
false_northing = (zone < 0) ? 10000000.0 : 0.0;

temp = r_minor / r_major;
es = 1.0 - SQUARE(temp);
e = sqrt(es);
e0 = e0fn(es);
e1 = e1fn(es);
e2 = e2fn(es);
e3 = e3fn(es);
ml0 = r_major * mlfn(e0, e1, e2, e3, lat_origin);
esp = es / (1.0 - es);

if (es < .00001)
   ind = 1;

/* Report parameters to the user
  -----------------------------*/
/* REMOVED FEB 2004 BY DK, CIMAR */
/*
ptitle("UNIVERSAL TRANSVERSE MERCATOR (UTM)"); 
genrpt_long(zone,   "Zone:     ");
radius2(r_major, r_minor);
genrpt(scale_factor,"Scale Factor at C. Meridian:     ");
cenlonmer(lon_center);
*/
return(OK);
}


/* Universal Transverse Mercator forward equations--mapping lat,long to x,y
   Note:  The algorithm for UTM is exactly the same as TM and therefore
	  if a change is implemented, also make the change to TMFOR.c
  -----------------------------------------------------------------------*/
long utmfor(lon, lat, x, y)
double lon;			/* (I) Longitude 		*/
double lat;			/* (I) Latitude 		*/
double *x;			/* (O) X projection coordinate 	*/
double *y;			/* (O) Y projection coordinate 	*/
{
double delta_lon;	/* Delta longitude (Given longitude - center 	*/
//double theta;		/* angle					*/
//double delta_theta;	/* adjusted longitude				*/
double sin_phi, cos_phi;/* sin and cos value				*/
double al, als;		/* temporary values				*/
double b;		/* temporary values				*/
double c, t, tq;	/* temporary values				*/
double con, n, ml;	/* cone constant, small m			*/

/* Forward equations
  -----------------*/
delta_lon = adjust_lon(lon - lon_center);
sincos(lat, &sin_phi, &cos_phi);

/* This part was in the fortran code and is for the spherical form 
----------------------------------------------------------------*/
if (ind != 0)
  {
  b = cos_phi * sin(delta_lon);
  if ((fabs(fabs(b) - 1.0)) < .0000000001)
     {
     p_error("Point projects into infinity","utm-for");
	 return(93);
     }
  else
     {
     *x = .5 * r_major * scale_factor * log((1.0 + b)/(1.0 - b));
     con = acos(cos_phi * cos(delta_lon)/sqrt(1.0 - b*b));
     if (lat < 0)
        con = - con;
     *y = r_major * scale_factor * (con - lat_origin); 
     return(OK);
     }
  }

al  = cos_phi * delta_lon;
als = SQUARE(al);
c   = esp * SQUARE(cos_phi);
tq  = tan(lat);
t   = SQUARE(tq);
con = 1.0 - es * SQUARE(sin_phi);
n   = r_major / sqrt(con);
ml  = r_major * mlfn(e0, e1, e2, e3, lat);

*x  = scale_factor * n * al * (1.0 + als / 6.0 * (1.0 - t + c + als / 20.0 *
      (5.0 - 18.0 * t + SQUARE(t) + 72.0 * c - 58.0 * esp))) + false_easting;

*y  = scale_factor * (ml - ml0 + n * tq * (als * (0.5 + als / 24.0 *
      (5.0 - t + 9.0 * c + 4.0 * SQUARE(c) + als / 30.0 * (61.0 - 58.0 * t
      + SQUARE(t) + 600.0 * c - 330.0 * esp))))) + false_northing;

return(OK);
}
