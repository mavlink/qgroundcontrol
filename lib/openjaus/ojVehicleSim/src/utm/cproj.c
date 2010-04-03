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
NAME                Projection support routines listed below.

PURPOSE:	The following functions are included in CPROJ.C.

		SINCOS:	  Calculates the sine and cosine.
		ASINZ:	  Eliminates roundoff errors.
		MSFNZ:	  Computes the constant small m for Oblique Equal Area.
		QSFNZ:	  Computes the constant small q for Oblique Equal Area.
		PHI1Z:	  Computes phi1 for Albers Conical Equal-Area.
		PHI2Z:	  Computes the latitude angle, phi2, for Lambert
			  Conformal Conic and Polar Stereographic.
		PHI3Z:	  Computes the latitude, phi3, for Equidistant Conic.
		PHI4Z:	  Computes the latitude, phi4, for Polyconic.
		PAKCZ:	  Converts a 2 digit alternate packed DMS format to
			  standard packed DMS format.
		PAKR2DM:  Converts radians to 3 digit packed DMS format.
		TSFNZ:	  Computes the small t for Lambert Conformal Conic and
			  Polar Stereographic.
		SIGN:	  Returns the sign of an argument.
		ADJUST_LON:  Adjusts a longitude angle to range -180 to 180.
		E0FN, E1FN, E2FN, E3FN:
			  Computes the constants e0,e1,e2,and e3 for
			  calculating the distance along a meridian.
		E4FN:	  Computes e4 used for Polar Stereographic.
		MLFN:	  Computes M, the distance along a meridian.
		CALC_UTM_ZONE:	Calculates the UTM zone number.

PROGRAMMER              DATE		REASON
----------              ----		------
D. Steinwand, EROS      July, 1991	Initial development
T. Mittan, EROS		May, 1993	Modified from Fortran GCTP for C GCTP
S. Nelson, EROS		June, 1993	Added inline comments
S. Nelson, EROS		Nov, 1993	Added loop counter in ADJUST_LON
S. Nelson, EROS		Jan, 1998	Changed misspelled error message

*******************************************************************************/
#include "utm/utmLib.h"

#define MAX_VAL 4
#define MAXLONG 2147483647.
#define DBLLONG 4.61168601e18

/* Function to calculate the sine and cosine in one call.  Some computer
   systems have implemented this function, resulting in a faster implementation
   than calling each function separately.  It is provided here for those
   computer systems which don`t implement this function
  ----------------------------------------------------*/
void sincos(val, sin_val, cos_val)
double val;
double *sin_val;
double *cos_val;
{
*sin_val = sin(val);
*cos_val = cos(val);
return;
}

/* Function to eliminate roundoff errors in asin
----------------------------------------------*/
double asinz (con)

double con;
{
 if (fabs(con) > 1.0)
   {
   if (con > 1.0)
     con = 1.0;
   else
     con = -1.0;
   }
 return(asin(con));
}

/* Function to compute the constant small m which is the radius of
   a parallel of latitude, phi, divided by the semimajor axis.
---------------------------------------------------------------*/
double msfnz (eccent,sinphi,cosphi)
  double eccent;
  double sinphi;
  double cosphi;
{
double con;

      con = eccent * sinphi;
      return((cosphi / (sqrt (1.0 - con * con))));
}

/* Function to compute constant small q which is the radius of a 
   parallel of latitude, phi, divided by the semimajor axis. 
------------------------------------------------------------*/
double qsfnz (eccent,sinphi,cosphi)
   double eccent;
   double sinphi;
   double cosphi;
{
double con;

   if (eccent > 1.0e-7)
     {
     con = eccent * sinphi;
     return (( 1.0- eccent * eccent) * (sinphi /(1.0 - con * con) - (.5/eccent)*
             log((1.0 - con)/(1.0 + con))));
     }
   else
     return(2.0 * sinphi);
}

/* Function to compute phi1, the latitude for the inverse of the
   Albers Conical Equal-Area projection.
-------------------------------------------*/
double phi1z (eccent,qs,flag)
     double eccent;	/* Eccentricity angle in radians		*/
     double qs;		/* Angle in radians				*/
     long  *flag;	/* Error flag number				*/
{
double eccnts;
double dphi;
double con;
double com;
double sinpi;
double cospi;
double phi;
long i;

      phi = asinz(.5 * qs);
      if (eccent < EPSLN) 
         return(phi);
      eccnts = eccent * eccent; 
      for (i = 1; i <= 25; i++)
        {
        sincos(phi,&sinpi,&cospi);
        con = eccent * sinpi; 
        com = 1.0 - con * con;
        dphi = .5 * com * com / cospi * (qs / (1.0 - eccnts) - sinpi / com + 
               .5 / eccent * log ((1.0 - con) / (1.0 + con)));
       phi = phi + dphi;
       if (fabs(dphi) <= 1e-7)
          return(phi);
        }
  p_error ("Convergence error","phi1z-conv");
  *flag = 001;
  return(UTM_ERROR);
}

/* Function to compute the latitude angle, phi2, for the inverse of the
   Lambert Conformal Conic and Polar Stereographic projections.
----------------------------------------------------------------*/
double phi2z(eccent,ts,flag)

double eccent;		/* Spheroid eccentricity		*/
double ts;		/* Constant value t			*/
long *flag;		/* Error flag number			*/
 
{
double eccnth;
double phi;
double con;
double dphi;
double sinpi;
long i;

  *flag = 0;
  eccnth = .5 * eccent;
  phi = HALF_PI - 2 * atan(ts);
  for (i = 0; i <= 15; i++)
    {
    sinpi = sin(phi);
    con = eccent * sinpi;
    dphi = HALF_PI - 2 * atan(ts *(pow(((1.0 - con)/(1.0 + con)),eccnth))) - 
	   phi;
    phi += dphi; 
    if (fabs(dphi) <= .0000000001)
       return(phi);
    }
  p_error ("Convergence error","phi2z-conv");
  *flag = 002;
  return(002);
}
 
/* Function to compute latitude, phi3, for the inverse of the Equidistant
   Conic projection.
-----------------------------------------------------------------*/
double phi3z(ml,e0,e1,e2,e3,flag)

double ml;		/* Constant 			*/
double e0;		/* Constant			*/
double e1;		/* Constant			*/
double e2;		/* Constant			*/
double e3;		/* Constant			*/
long *flag;		/* Error flag number		*/

{
double phi;
double dphi;
long i;

phi = ml;
for (i = 0; i < 15; i++)
  {
  dphi = (ml + e1 * sin(2.0 * phi) - e2 * sin(4.0 * phi) + e3 * sin(6.0 * phi))
         / e0 - phi;
  phi += dphi;
  if (fabs(dphi) <= .0000000001)
     {
     *flag = 0;
     return(phi);
     }
  }
p_error("Latitude failed to converge after 15 iterations","PHI3Z-CONV");
*flag = 3;
return(3);
}

/* Function to compute, phi4, the latitude for the inverse of the
   Polyconic projection.
------------------------------------------------------------*/
double phi4z (eccent,e0,e1,e2,e3,a,b,c,phi) 

double eccent;		/* Spheroid eccentricity squared	*/
double e0;
double e1;
double e2;
double e3;
double a;
double b;
double *c;
double *phi;
{
double sinphi;
double sin2ph;
double tanphi;
double ml;
double mlp;
double con1;
double con2;
double con3;
double dphi;
long i;

      *phi = a;
      for (i = 1; i <= 15; i++)
        {
        sinphi = sin(*phi);
        tanphi = tan(*phi);
        *c = tanphi * sqrt (1.0 - eccent * sinphi * sinphi);
        sin2ph = sin (2.0 * *phi);
/*
        ml = e0 * *phi - e1 * sin2ph + e2 * sin (4.0 *  *phi);
        mlp = e0 - 2.0 * e1 * cos (2.0 *  *phi) + 4.0 * e2 *
              cos (4.0 *  *phi);
*/
        ml = e0 * *phi - e1 * sin2ph + e2 * sin (4.0 *  *phi) - e3 * 
 	     sin (6.0 *  *phi);
        mlp = e0 - 2.0 * e1 * cos (2.0 *  *phi) + 4.0 * e2 *
              cos (4.0 *  *phi) - 6.0 * e3 * cos (6.0 *  *phi);
        con1 = 2.0 * ml + *c * (ml * ml + b) - 2.0 * a *  (*c * ml + 1.0);
        con2 = eccent * sin2ph * (ml * ml + b - 2.0 * a * ml) / (2.0 * *c);
        con3 = 2.0 * (a - ml) * (*c * mlp - 2.0 / sin2ph) - 2.0 * mlp;
        dphi = con1 / (con2 + con3);
        *phi += dphi;
        if (fabs(dphi) <= .0000000001 )
 	    return(OK);
        }
p_error("Latitude failed to converge","phi4z-conv");
return(004);
}

/* Function to convert 2 digit alternate packed DMS format (+/-)DDDMMSS.SSS
   to 3 digit standard packed DMS format (+/-)DDDMMMSSS.SSS.
-----------------------------------------------------------------*/
double pakcz(pak)

      double pak;	/* Angle in alternate packed DMS format	*/
      {
      double con;
      double secs;
      long degs,mins;
      char sgna;

      sgna = ' ';
      if (pak < 0.0) 
	 sgna = '-';
      con = fabs (pak);
      degs = (long) ((con / 10000.0) + .001);
      con =  con  - degs * 10000;
      mins = (long) ((con / 100.0) + .001);
      secs = con  - mins * 100;
      con = (double) (degs) * 1000000.0 + (double) (mins) * 1000.0 + secs;
      if (sgna == '-') 
	  con = - con;
      return(con); 
      }

/* Function to convert radians to 3 digit packed DMS format (+/-)DDDMMMSSS.SSS
----------------------------------------------------------------------------*/
double pakr2dm(pak)

      double pak;	/* Angle in radians			*/
      {
      double con;
      double secs;
      long degs,mins;
      char sgna;

      sgna = ' ';
      pak *= R2D;
      if (pak < 0.0) 
	 sgna = '-';
      con = fabs (pak);
      degs = (long) (con);
      con =  (con  - degs) * 60;
      mins = (long) con;
      secs = (con  - mins) * 60;
      con = (double) (degs) * 1000000.0 + (double) (mins) * 1000.0 + secs;
      if (sgna == '-') 
	  con = - con;
      return(con); 
      }

/* Function to compute the constant small t for use in the forward
   computations in the Lambert Conformal Conic and the Polar
   Stereographic projections.
--------------------------------------------------------------*/
double tsfnz(eccent,phi,sinphi)
  double eccent;	/* Eccentricity of the spheroid		*/
  double phi;		/* Latitude phi				*/
  double sinphi;	/* Sine of the latitude			*/
  {
  double con;
  double com;
  
  con = eccent * sinphi;
  com = .5 * eccent; 
  con = pow(((1.0 - con) / (1.0 + con)),com);
  return (tan(.5 * (HALF_PI - phi))/con);
  }


/* Function to return the sign of an argument
  ------------------------------------------*/
int sign(x)
double x;
{
if (x < 0.0)
    return(-1);
else
    return(1);
}

/* Function to adjust a longitude angle to range from -180 to 180 radians
   added if statments 
  -----------------------------------------------------------------------*/
double adjust_lon(x) 

double x;		/* Angle in radians			*/
{
//long temp;
long count = 0;
for(;;)
  {
  if (fabs(x)<=PI)
     break;
  else
  if (((long) fabs(x / PI)) < 2)
     x = x-(sign(x) *TWO_PI);
  else
  if (((long) fabs(x / TWO_PI)) < MAXLONG)
     {
     x = x-(((long)(x / TWO_PI))*TWO_PI);
     }
  else
  if (((long) fabs(x / (MAXLONG * TWO_PI))) < MAXLONG)
     {
     x = x-(((long)(x / (MAXLONG * TWO_PI))) * (TWO_PI * MAXLONG));
     }
  else
  if (((long) fabs(x / (DBLLONG * TWO_PI))) < MAXLONG)
     {
     x = x-(((long)(x / (DBLLONG * TWO_PI))) * (TWO_PI * DBLLONG));
     }
  else
     x = x-(sign(x) *TWO_PI);
  count++;
  if (count > MAX_VAL)
     break;
  }

return(x);
}

/* Functions to compute the constants e0, e1, e2, and e3 which are used
   in a series for calculating the distance along a meridian.  The
   input x represents the eccentricity squared.
----------------------------------------------------------------*/
double e0fn(x)
double x;
{
return(1.0-0.25*x*(1.0+x/16.0*(3.0+1.25*x)));
}
double e1fn(x)
double x;
{
return(0.375*x*(1.0+0.25*x*(1.0+0.46875*x)));
}
double e2fn(x)
double x;
{
return(0.05859375*x*x*(1.0+0.75*x));
}
double e3fn(x) 
double x;
{
return(x*x*x*(35.0/3072.0));
}

/* Function to compute the constant e4 from the input of the eccentricity
   of the spheroid, x.  This constant is used in the Polar Stereographic
   projection.
--------------------------------------------------------------------*/
double e4fn(x)
double x;
{
 double con;
 double com;
 con = 1.0 + x;
 com = 1.0 - x;
 return (sqrt((pow(con,con))*(pow(com,com))));
 }

/* Function computes the value of M which is the distance along a meridian
   from the Equator to latitude phi.
------------------------------------------------*/
double mlfn(e0,e1,e2,e3,phi)
double e0,e1,e2,e3,phi;
{
return(e0*phi-e1*sin(2.0*phi)+e2*sin(4.0*phi)-e3*sin(6.0*phi));
}

/* Function to calculate UTM zone number--NOTE Longitude entered in DEGREES!!!
  ---------------------------------------------------------------------------*/
long calc_utm_zone(lon)
double lon;
{
	return((long)(((lon + 180.0) / 6.0) + 1.0));
}
