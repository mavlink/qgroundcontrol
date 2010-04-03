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
/* **************************************************************** */
/* utmLib.c															*/
/*																	*/
/* A library of functions to convert from Latitude and Longitude	*/
/* to X and Y within a UTM Zone using the WGS84 Spheroid.			*/
/*																	*/
/* This Library is based on the GPTC Projection Algorithms			*/
/* avalible from USGS at:											*/
/* ftp://edcftp.cr.usgs.gov/pub/software/gctpc/gctpc20.tar.Z		*/
/*																	*/
/* Adapted by:	Daniel A. Kent (jaus AT dannykent DOT com)			*/
/*				2004												*/
/*																	*/
/* Functions:	utmConversionInit() - Initialize Converstions		*/
/*				pointLlaToPointUtm() - (Lat,Lon) to (X,Y)			*/
/*				pointUtmToPointLla() - (X,Y) to (Lat,Lon)			*/
/*																	*/
/* Library Files:	cproj.c											*/
/*					utmfor.c										*/
/*					utminv.c										*/
/*					utmLib.c										*/
/*					cproj.h											*/
/*					proj.h											*/
/*					utmLib.h										*/
/*																	*/
/* **************************************************************** */

#include <stdlib.h>
#include <stdio.h>
#include "utm/utmLib.h"

static unsigned char utmLibInitFlag = FALSE;	// Place holder to test if the library has been initialized
static long utmLibZone = 0;						// Place holder to remember the last zone the library was initialized to

/* **************************************************************** */
/* Function: utmInitCheck()											*/	
/* Inputs:	none													*/
/* Outputs: TRUE if the Library has been initialized				*/
/* Description: Used to check the status of the UTM Engine			*/
/* **************************************************************** */
unsigned char utmInitCheck(void)
{
	return utmLibInitFlag;
}

/* **************************************************************** */
/* Function: utmZoneCheck()											*/	
/* Inputs:	PointLLA												*/
/* Outputs: TRUE if zones are equal									*/
/* Description: Function takes a value for Longitude,				*/
/*				calculates the UTM Zone and checks whether the		*/
/* 				utmLib has been setup for that zone.				*/
/*				Use this to detect changes in zone during operation	*/
/* Units:	LAT and LON in RADIANS									*/
/* **************************************************************** */
unsigned char utmZoneCheck(PointLla point)
{
	long zone = 0;
	
	if(point->longitudeRadians > PI || point->longitudeRadians < -PI)
	{
		p_error("Invalid Test Point for UTM Zone Check. Check Radians??","");
		return FALSE;
	}
	
	zone = calc_utm_zone(point->longitudeRadians * DEG_PER_RAD);
	
	if(zone == utmLibZone)
		return TRUE;
	else
		return FALSE;
}

/* **************************************************************** */
/* Function: utmConversionInit()									*/	
/* Inputs:	PointLLA												*/
/* Outputs: 0 if sucessful											*/
/* Description: Function takes a seed value for Longitude,			*/
/*				calculates the UTM Zone and intializes the			*/
/*				utmForward and utmInverse functions.				*/
/*				Constructs the conversion for the WGS84 Spheroid	*/
/* Units:	LAT and LON in RADIANS									*/
/* **************************************************************** */
long utmConversionInit(PointLla point)
{
	long zone = 0;
	double r_maj = 6378137;			// Magic Number: These are the values
	double r_min = 6356752.3142;	// needed for the UTM ellipsoid
	double scale_fact = .9996;
//	long val;
	
	if(point->longitudeRadians > PI || point->longitudeRadians < -PI)
	{
		p_error("Invalid Seed Point for UTM Init. Check Radians??","");
		return(-1);
	}

	zone = calc_utm_zone(point->longitudeRadians*R2D);

	// Check if the zone is different than what was done before
	if(zone != utmLibZone)
	{		
		if(utmforint(r_maj, r_min, scale_fact, zone) != OK) return -1;
		if(utminvint(r_maj, r_min, scale_fact, zone) != OK) return -1;
		
		utmLibInitFlag = 1;
		utmLibZone = zone;
		return OK;
	}
	else
	{
		// Zone is the same, no need to re-init
		return OK;
	}
}

/* **************************************************************** */
/* Function: UTM2LLA()												*/	
/* Inputs:	PointXYZ												*/
/* Outputs: PointLLA												*/
/* Description: Converts the point given in X and Y to Lat			*/
/*				and Lon using the UTM projection for the			*/
/*				WGS84 Spheroid and the seeded UTM zone.				*/
/* Units:	LAT and LON in RADIANS									*/
/*			X and Y in METERS										*/
/* **************************************************************** */
PointLla pointUtmToPointLla(PointUtm pointUtm)
{
	PointLla pointLla = pointLlaCreate();
	
	utminv(pointUtm->xMeters, pointUtm->yMeters, &pointLla->longitudeRadians, &pointLla->latitudeRadians);
	
	return pointLla;
}

/* **************************************************************** */
/* Function: LLA2UTM()												*/	
/* Inputs:	PointXYZ												*/
/* Outputs: PointLLA												*/
/* Description: Converts the point given in Lat and Lon to 			*/
/*				Lat and Lon using the UTM projection for 			*/
/*				the WGS84 Spheroid and the seeded UTM Zone.			*/
/* Units:	LAT and LON in RADIANS									*/
/*			X and Y in METERS										*/
/* **************************************************************** */
PointUtm pointLlaToPointUtm(PointLla pointLla)
{
	PointUtm pointUtm = pointUtmCreate();
	
	utmfor(pointLla->longitudeRadians, pointLla->latitudeRadians, &pointUtm->xMeters, &pointUtm->yMeters);
	
	return pointUtm;
}

/* **************************************************************** */
/* Function: p_error()												*/	
/* Inputs:	char what												*/
/* Outputs: void													*/
/* Description: Provides p_error functions used in GCTP functions	*/
/* **************************************************************** */
void p_error(char *what, char *where)
{
	printf(what); printf("\n");
}
