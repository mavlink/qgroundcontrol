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

PURPOSE:	Transforms input Easting and Northing to longitude and
		latitude for the Universal Transverse Mercator projection.
		The Easting and Northing must be in meters.  The longitude
		and latitude values will be returned in radians.

PROGRAMMER              DATE		REASON
----------              ----		------
D. Steinwand, EROS      Nov, 1991
T. Mittan		Mar, 1993
S. Nelson		Feb, 1995	Divided tminv.c into two files, one
					for UTM (utminv.c) and one for
					TM (tminv.c).  This was a necessary
					change to run inverse projection
					conversions for both UTM and TM
					in the same process.

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
static double r_major;          /* major axis                           */
static double r_minor;          /* minor axis                           */
static double scale_factor;     /* scale factor                         */
static double lon_center;       /* Center longitude (projection center) */
static double lat_origin;       /* center latitude                      */
static double e0,e1,e2,e3;      /* eccentricity constants               */
static double e,es,esp;         /* eccentricity constants               */
static double ml0;              /* small value m                        */
static double false_northing;   /* y offset in meters                   */
static double false_easting; 	/* x offset in meters			*/
static long ind;		/* sphere flag value			*/


/* Initialize the Universal Transverse Mercator (UTM) projection
  -------------------------------------------------------------*/
long utminvint(r_maj,r_min,scale_fact,zone)

double r_maj;			/* major axis				*/
double r_min;			/* minor axis				*/
double scale_fact;		/* scale factor				*/
long zone;			/* zone number				*/
{
double temp;			/* temprorary variables			*/

if ((abs(zone) < 1) || (abs(zone) > 60))
   {
	p_error("Illegal zone number","utm-invint");
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
else 
   ind = 0;

/* Report parameters to the user
  -----------------------------*/

/* REMOVED FEB 2004 BY DK, CIMAR */
/*
ptitle("UNIVERSAL TRANSVERSE MERCATOR (UTM)"); 
genrpt_long(zone,   "Zone:     ");
radius2(r_major, r_minor);
genrpt(scale_factor,"Scale Factor at C. Meridian:     ");
cenlonmer(lon_center); */
return(OK);
}

/* Universal Transverse Mercator inverse equations--mapping x,y to lat,long 
   Note:  The algorithm for UTM is exactly the same as TM and therefore
	  if a change is implemented, also make the change to TMINV.c
  -----------------------------------------------------------------------*/
long utminv(x, y, lon, lat)
double x;		/* (I) X projection coordinate 			*/
double y;		/* (I) Y projection coordinate 			*/
double *lon;		/* (O) Longitude 				*/
double *lat;		/* (O) Latitude 				*/
{
double con,phi;		/* temporary angles				*/
double delta_phi;	/* difference between longitudes		*/
long i;			/* counter variable				*/
double sin_phi, cos_phi, tan_phi;	/* sin cos and tangent values	*/
double c, cs, t, ts, n, r, d, ds;	/* temporary variables		*/
double f, h, g, temp;			/* temporary variables		*/
long max_iter = 6;			/* maximun number of iterations	*/

/* fortran code for spherical form 
--------------------------------*/
if (ind != 0)
   {
   f = exp(x/(r_major * scale_factor));
   g = .5 * (f - 1/f);
   temp = lat_origin + y/(r_major * scale_factor);
   h = cos(temp);
   con = sqrt((1.0 - h * h)/(1.0 + g * g));
   *lat = asinz(con);
   if (temp < 0)
     *lat = -*lat;
   if ((g == 0) && (h == 0))
     {
     *lon = lon_center;
     return(OK);
     }
   else
     {
     *lon = adjust_lon(atan2(g,h) + lon_center);
     return(OK);
     }
   }

/* Inverse equations
  -----------------*/
x = x - false_easting;
y = y - false_northing;

con = (ml0 + y / scale_factor) / r_major;
phi = con;
for (i=0;;i++)
   {
   delta_phi=((con + e1 * sin(2.0*phi) - e2 * sin(4.0*phi) + e3 * sin(6.0*phi))
               / e0) - phi;
/*
   delta_phi = ((con + e1 * sin(2.0*phi) - e2 * sin(4.0*phi)) / e0) - phi;
*/
   phi += delta_phi;
   if (fabs(delta_phi) <= EPSLN) break;
   if (i >= max_iter) 
      { 
      p_error("Latitude failed to converge","UTM-INVERSE"); 
      return(95);
      }
   }
if (fabs(phi) < HALF_PI)
   {
   sincos(phi, &sin_phi, &cos_phi);
   tan_phi = tan(phi);
   c    = esp * SQUARE(cos_phi);
   cs   = SQUARE(c);
   t    = SQUARE(tan_phi);
   ts   = SQUARE(t);
   con  = 1.0 - es * SQUARE(sin_phi); 
   n    = r_major / sqrt(con);
   r    = n * (1.0 - es) / con;
   d    = x / (n * scale_factor);
   ds   = SQUARE(d);
   *lat = phi - (n * tan_phi * ds / r) * (0.5 - ds / 24.0 * (5.0 + 3.0 * t + 
          10.0 * c - 4.0 * cs - 9.0 * esp - ds / 30.0 * (61.0 + 90.0 * t +
          298.0 * c + 45.0 * ts - 252.0 * esp - 3.0 * cs)));
   *lon = adjust_lon(lon_center + (d * (1.0 - ds / 6.0 * (1.0 + 2.0 * t +
          c - ds / 20.0 * (5.0 - 2.0 * c + 28.0 * t - 3.0 * cs + 8.0 * esp +
          24.0 * ts))) / cos_phi));
   }
else
   {
   *lat = HALF_PI * sign(y);
   *lon = lon_center;
   }
return(OK);
}
