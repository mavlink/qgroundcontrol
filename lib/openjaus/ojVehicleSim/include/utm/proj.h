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
/* Projection codes


   0 = Geographic
   1 = Universal Transverse Mercator (UTM)
   2 = State Plane Coordinates
   3 = Albers Conical Equal Area
   4 = Lambert Conformal Conic
   5 = Mercator
   6 = Polar Stereographic
   7 = Polyconic
   8 = Equidistant Conic
   9 = Transverse Mercator
  10 = Stereographic
  11 = Lambert Azimuthal Equal Area
  12 = Azimuthal Equidistant
  13 = Gnomonic
  14 = Orthographic
  15 = General Vertical Near-Side Perspective
  16 = Sinusiodal
  17 = Equirectangular
  18 = Miller Cylindrical
  19 = Van der Grinten
  20 = (Hotine) Oblique Mercator 
  21 = Robinson
  22 = Space Oblique Mercator (SOM)
  23 = Alaska Conformal
  24 = Interrupted Goode Homolosine 
  25 = Mollweide
  26 = Interrupted Mollweide
  27 = Hammer
  28 = Wagner IV
  29 = Wagner VII
  30 = Oblated Equal Area
  99 = User defined
*/

/* Define projection codes */
#define GEO 0
#define UTM 1
#define SPCS 2
#define ALBERS 3
#define LAMCC 4
#define MERCAT 5
#define PS 6
#define POLYC 7
#define EQUIDC 8
#define TM 9
#define STEREO 10
#define LAMAZ 11
#define AZMEQD 12
#define GNOMON 13
#define ORTHO 14
#define GVNSP 15
#define SNSOID 16
#define EQRECT 17
#define MILLER 18
#define VGRINT 19
#define HOM 20
#define ROBIN 21
#define SOM 22
#define ALASKA 23
#define GOOD 24
#define MOLL 25
#define IMOLL 26
#define HAMMER 27
#define WAGIV 28
#define WAGVII 29
#define OBEQA 30
#define USDEF 99 

/* Define unit code numbers to their names */

#define RADIAN 0		/* Radians */
#define FEET 1			/* Feed */
#define METER 2			/* Meters */
#define SECOND 3		/* Seconds */
#define DEGREE 4		/* Decimal degrees */
#define INT_FEET 5		/* International Feet */

/* The STPLN_TABLE unit value is specifically used for State Plane -- if units
   equals STPLN_TABLE and Datum is NAD83--actual units are retrieved from
   a table according to the zone.  If Datum is NAD27--actual units will be feet.
   An error will occur with this unit if the projection is not State Plane.  */

#define STPLN_TABLE 6

/* General code numbers */

#define IN_BREAK -2		/*  Return status if the interupted projection
				    point lies in the break area */
#define COEFCT 15		/*  projection coefficient count */
#define PROJCT 30		/*  projection count */
#define SPHDCT 31		/*  spheroid count */

#define MAXPROJ 31		/*  Maximum projection number */
#define MAXUNIT 5		/*  Maximum unit code number */
#define GEO_TERM 0		/*  Array index for print-to-term flag */
#define GEO_FILE 1		/*  Array index for print-to-file flag */
#define GEO_TRUE 1		/*  True value for geometric true/false flags */
#define GEO_FALSE -1		/*  False val for geometric true/false flags */

/* GCTP Function prototypes */

long alberforint(double r_maj, double r_min, double lat1, double lat2,
	double lon0, double lat0, double false_east, double false_north);
long alberfor(double lon, double lat, double *x, double *y);
long alberinvint( double r_maj, double r_min, double lat1, double lat2,
	double lon0, double lat0, double false_east, double false_north);
long alberinv( double x, double y, double *lon, double *lat);
long alconforint( double r_maj, double r_min, double false_east,
	double false_north);
long alconfor(double lon, double lat, double *x, double *y);
long alconinvint( double r_maj, double r_min, double false_east,
	double false_north);
long alconinv( double x, double y, double *lon, double *lat);
long azimforint( double r_maj, double center_lon, double center_lat,
	double false_east, double false_north);
long azimfor( double lon, double lat, double *x, double *y);
long aziminvint( double r_maj, double center_lon, double center_lat,
	double false_east, double false_north);
long aziminv( double x, double y, double *lon, double *lat);

/* Functions residing in cproj.c */
void sincos( double val, double *sin_val, double *cos_val);
double asinz (double con);
double msfnz (double eccent, double sinphi, double cosphi);
double qsfnz (double eccent, double sinphi, double cosphi);
double phi1z (double eccent, double qs, long  *flag);
double phi2z(double eccent, double ts, long *flag);
double phi3z(double ml, double e0, double e1, double e2, double e3,
	long *flag);
double phi4z (double eccent, double e0, double e1, double e2, double e3,
	double a, double b, double *c, double *phi);
double pakcz(double pak);
double pakr2dm(double pak);
double tsfnz(double eccent, double phi, double sinphi);
int sign(double x);
double adjust_lon(double x);
double e0fn(double x);
double e1fn(double x);
double e2fn(double x);
double e3fn(double x);
double e4fn(double x);
double mlfn(double e0,double e1,double e2,double e3,double phi);
long calc_utm_zone(double lon);
/* End of functions residing in cproj.h */

long eqconforint(double r_maj, double r_min, double lat1, double lat2,
	double center_lon, double center_lat, double false_east,
	double false_north,
	long mode);
long eqconfor(double lon, double lat, double *x, double *y);
long eqconinvint(double r_maj, double r_min, double lat1, double lat2,
	double center_lon, double center_lat, double false_east,
	double false_north, long mode);
long eqconinv(double x, double y, double *lon, double *lat);
long equiforint(double r_maj, double center_lon, double lat1,
	double false_east, double false_north);
long equifor(double lon, double lat, double *x, double *y);
long equiinvint(double r_maj, double center_lon, double lat1,
	double false_east, double false_north);
long equiinv(double x, double y, double *lon, double *lat);
void for_init(long outsys, long outzone, double *outparm, long outspheroid,
	char *fn27, char *fn83, long *iflg, long (*for_trans[])());
void gctp(double *incoor, long *insys, long *inzone, double *inparm,
	long *inunit, long *inspheroid, long *ipr, char *efile, long *jpr,
	char *pfile, double *outcoor, long *outsys, long *outzone,
	double *outparm, long *outunit, long *outspheroid, char fn27[],
	char fn83[], long *iflg);
long gnomforint(double r, double center_long, double center_lat,
	double false_east, double false_north);
long gnomfor(double lon, double lat, double *x, double *y);
long gnominvint(double r, double center_long, double center_lat,
	double false_east, double false_north);
long gnominv(double x, double y, double *lon, double *lat);
long goodforint(double r);
long goodfor(double lon, double lat, double *x, double *y);
long goodinvint(double r);
long goodinv(double x, double y, double *lon, double *lat);
long gvnspforint(double r, double h, double center_long, double center_lat,
	double false_east, double false_north);
long gvnspfor(double lon, double lat, double *x, double *y);
long gvnspinvint(double r, double h, double center_long, double center_lat,
	double false_east, double false_north);
long gvnspinv(double x, double y, double *lon, double *lat);
long hamforint(double r, double center_long, double false_east,
	double false_north);
long hamfor(double lon, double lat, double *x, double *y);
long haminvint(double r, double center_long, double false_east,
	double false_north);
long haminv(double x, double y, double *lon, double *lat);
long imolwforint(double r);
long imolwfor(double lon, double lat, double *x, double *y);
long imolwinvint(double r);
long imolwinv(double x, double y, double *lon, double *lat);
void inv_init(long insys, long inzone, double *inparm, long inspheroid,
	char *fn27, char *fn83, long *iflg, long (*inv_trans[])());
long lamazforint(double r, double center_long, double center_lat,
	double false_east, double false_north);
long lamazfor(double lon, double lat, double *x, double *y);
long lamazinvint(double r, double center_long, double center_lat,
	double false_east, double false_north);
long lamazinv(double x, double y, double *lon, double *lat);
long lamccforint(double r_maj, double r_min, double lat1, double lat2,
	double c_lon, double c_lat, double false_east, double false_north);
long lamccfor(double lon, double lat, double *x, double *y);
long lamccinvint(double r_maj, double r_min, double lat1, double lat2,
	double c_lon, double c_lat, double false_east, double false_north);
long lamccinv(double x, double y, double *lon, double *lat);
long merforint(double r_maj, double r_min, double center_lon, double center_lat,
	double false_east, double false_north);
long merfor(double lon, double lat, double *x, double *y);
long merinvint(double r_maj, double r_min, double center_lon, double center_lat,
	double false_east, double false_north);
long merinv(double x, double y, double *lon, double *lat);
long millforint(double r, double center_long, double false_east,
	double false_north);
long millfor(double lon, double lat, double *x, double *y);
long millinvint(double r, double center_long, double false_east,
	double false_north);
long millinv(double x, double y, double *lon, double *lat);
long molwforint(double r, double center_long, double false_east,
	double false_north);
long molwfor(double lon, double lat, double *x, double *y);
long molwinvint(double r, double center_long, double false_east,
	double false_north);
long molwinv(double x, double y, double *lon, double *lat);
long obleqforint(double r, double center_long, double center_lat,
	double shape_m, double shape_n, double angle, double false_east,
	double false_north);
long obleqfor(double lon, double lat, double *x, double *y);
long obleqinvint(double r, double center_long, double center_lat,
	double shape_m, double shape_n, double angle, double false_east,
	double false_north);
long omerforint(double r_maj, double r_min, double scale_fact,
	double azimuth, double lon_orig, double lat_orig, double false_east,
	double false_north, double lon1, double lat1, double lon2, double lat2,
	long mode);
long omerfor(double lon, double lat, double *x, double *y);
long omerinvint(double r_maj, double r_min, double scale_fact, double azimuth,
	double lon_orig, double lat_orig, double false_east, double false_north,
	double lon1, double lat1, double lon2, double lat2, long mode);
long omerinv(double x, double y, double *lon, double *lat);
long orthforint(double r_maj, double center_lon, double center_lat,
	double false_east, double false_north);
long orthfor( double lon, double lat, double *x, double *y);
long orthinvint(double r_maj, double center_lon, double center_lat,
	double false_east, double false_north);
long orthinv(double x, double y, double *lon, double *lat);
double paksz(double ang, long *iflg);
long polyforint(double r_maj, double r_min, double center_lon,
	double center_lat, double false_east, double false_north);
long polyfor(double lon, double lat, double *x, double *y);
long polyinvint(double r_maj, double r_min, double center_lon,
	double center_lat, double false_east, double false_north);
long polyinv(double x, double y, double *lon, double *lat);
long psforint(double r_maj, double r_min, double c_lon, double c_lat,
	double false_east, double false_north);
long psfor( double lon, double lat, double *x, double *y);
long psinvint(double r_maj, double r_min, double c_lon, double c_lat,
	double false_east, double false_north);
long psinv( double x, double y, double *lon, double *lat);

/* functions in report.c */
void close_file();
long init(long ipr, long jpr, char *efile, char *pfile);
void ptitle(char *A);
void radius(double A);
void radius2(double A, double B);
void cenlon( double A);
void cenlonmer(double A);
void cenlat(double A);
void origin(double A);
void stanparl(double A,double B);
void stparl1(double A);
void offsetp(double A, double B);
void genrpt(double A, char *S);
void genrpt_long(long A, char *S);
void pblank();
void p_error(char *what, char *where);
/* End of the report.c functions */

long robforint(double r, double center_long, double false_east,
	double false_north);
long robfor( double lon, double lat, double *x, double *y);
long robinvint(double r, double center_long, double false_east,
	double false_north);
long robinv(double x, double y, double *lon, double *lat);
long sinforint(double r, double center_long, double false_east,
	double false_north);
long sinfor(double lon, double lat, double *x, double *y);
long sininvint(double r, double center_long, double false_east,
	double false_north);
long sininv(double x, double y, double *lon, double *lat);
long somforint(double r_major, double r_minor, long satnum, long path,
	double alf_in, double lon, double false_east, double false_north,
	double time, long start1, long flag);
long somfor(double lon, double lat, double *x, double *y);
/*static double som_series(double *fb, double *fa2, double *fa4, double *fc1,
	double *fc3,double *dlam);*/
long sominvint(double  r_major, double  r_minor, long satnum, long path,
	double alf_in, double lon, double false_east, double false_north,
	double time, long start1, long flag);
long sominv(double x, double y, double *lon, double *lat);
void sphdz(long isph, double *parm, double *r_major, double *r_minor,
	double *radius);
long sterforint(double r_maj, double center_lon, double center_lat,
	double false_east, double false_north);
long sterfor(double lon, double lat, double *x, double *y);
long sterinvint(double r_maj, double center_lon, double center_lat,
	double false_east, double false_north);
long sterinv(double x, double y, double *lon, double *lat);
long stplnforint(long zone, long sphere, char *fn27, char *fn83);
long stplnfor(double lon, double lat, double *x, double *y);
long stplninvint(long zone, long sphere, char *fn27, char *fn83);
long stplninv(double x, double y, double *lon, double *lat);
long tmforint(double r_maj, double r_min, double scale_fact,
	double center_lon, double center_lat, double false_east,
	double false_north);
long tmfor(double lon, double lat, double *x, double *y);
long tminvint(double r_maj, double r_min, double scale_fact,
	double center_lon, double center_lat, double false_east,
	double false_north);
long tminv(double x, double y, double *lon, double *lat);
long untfz(long inunit, long outunit, double *factor);
long utmforint(double r_maj, double r_min, double scale_fact, long zone); 
long utmfor(double lon, double lat, double *x, double *y);
long utminvint(double r_maj, double r_min, double scale_fact, long zone);
long utminv(double x, double y, double *lon, double *lat);
long vandgforint(double r, double center_long, double false_east,
	double false_north);
long vandgfor( double lon, double lat, double *x, double *y);
long vandginvint(double r, double center_long, double false_east,
	double false_north);
long vandginv(double x, double y, double *lon, double *lat);
long wivforint(double r, double center_long, double false_east,
	double false_north);
long wivfor(double lon, double lat, double *x, double *y);
long wivinvint(double r, double center_long, double false_east,
	double false_north);
long wivinv(double x, double y, double *lon, double *lat);
long wviiforint(double r, double center_long, double false_east,
	double false_north);
long wviifor(double lon, double lat, double *x, double *y);
long wviiinvint( double r, double center_long, double false_east,
	double false_north);
long wviiinv(double x, double y, double *lon, double *lat);
