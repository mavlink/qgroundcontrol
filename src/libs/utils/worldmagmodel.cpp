/**
 ******************************************************************************
 *
 * @file       worldmagmodel.cpp
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @brief      Utilities to find the location of openpilot GCS files:
 *             - Plugins Share directory path
 *
 * @brief      Source file for the World Magnetic Model
 *             This is a port of code available from the US NOAA.
 *
 *             The hard coded coefficients should be valid until 2015.
 *
 *             Updated coeffs from ..
 *             http://www.ngdc.noaa.gov/geomag/WMM/wmm_ddownload.shtml
 *
 *             NASA C source code ..
 *             http://www.ngdc.noaa.gov/geomag/WMM/wmm_wdownload.shtml
 *
 *             Major changes include:
 *                - No geoid model (altitude must be geodetic WGS-84)
 *                - Floating point calculation (not double precision)
 *                - Hard coded coefficients for model
 *                - Elimination of user interface
 *                - Elimination of dynamic memory allocation
 *
 * @see        The GNU Public License (GPL) Version 3
 *
 *****************************************************************************/
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include "worldmagmodel.h"

#include <stdint.h>
#include <QDebug>
#include <math.h>

#define RAD2DEG(rad)   ((rad) * (180.0 / M_PI))
#define DEG2RAD(deg)   ((deg) * (M_PI / 180.0))

// updated coeffs available from http://www.ngdc.noaa.gov/geomag/WMM/wmm_ddownload.shtml
const double CoeffFile[91][6] = {
    {0, 0, 0, 0, 0, 0},
    {1, 0, -29496.6, 0.0, 11.6, 0.0},
    {1, 1, -1586.3, 4944.4, 16.5, -25.9},
    {2, 0, -2396.6, 0.0, -12.1, 0.0},
    {2, 1, 3026.1, -2707.7, -4.4, -22.5},
    {2, 2, 1668.6, -576.1, 1.9, -11.8},
    {3, 0, 1340.1, 0.0, 0.4, 0.0},
    {3, 1, -2326.2, -160.2, -4.1, 7.3},
    {3, 2, 1231.9, 251.9, -2.9, -3.9},
    {3, 3, 634.0, -536.6, -7.7, -2.6},
    {4, 0, 912.6, 0.0, -1.8, 0.0},
    {4, 1, 808.9, 286.4, 2.3, 1.1},
    {4, 2, 166.7, -211.2, -8.7, 2.7},
    {4, 3, -357.1, 164.3, 4.6, 3.9},
    {4, 4, 89.4, -309.1, -2.1, -0.8},
    {5, 0, -230.9, 0.0, -1.0, 0.0},
    {5, 1, 357.2, 44.6, 0.6, 0.4},
    {5, 2, 200.3, 188.9, -1.8, 1.8},
    {5, 3, -141.1, -118.2, -1.0, 1.2},
    {5, 4, -163.0, 0.0, 0.9, 4.0},
    {5, 5, -7.8, 100.9, 1.0, -0.6},
    {6, 0, 72.8, 0.0, -0.2, 0.0},
    {6, 1, 68.6, -20.8, -0.2, -0.2},
    {6, 2, 76.0, 44.1, -0.1, -2.1},
    {6, 3, -141.4, 61.5, 2.0, -0.4},
    {6, 4, -22.8, -66.3, -1.7, -0.6},
    {6, 5, 13.2, 3.1, -0.3, 0.5},
    {6, 6, -77.9, 55.0, 1.7, 0.9},
    {7, 0, 80.5, 0.0, 0.1, 0.0},
    {7, 1, -75.1, -57.9, -0.1, 0.7},
    {7, 2, -4.7, -21.1, -0.6, 0.3},
    {7, 3, 45.3, 6.5, 1.3, -0.1},
    {7, 4, 13.9, 24.9, 0.4, -0.1},
    {7, 5, 10.4, 7.0, 0.3, -0.8},
    {7, 6, 1.7, -27.7, -0.7, -0.3},
    {7, 7, 4.9, -3.3, 0.6, 0.3},
    {8, 0, 24.4, 0.0, -0.1, 0.0},
    {8, 1, 8.1, 11.0, 0.1, -0.1},
    {8, 2, -14.5, -20.0, -0.6, 0.2},
    {8, 3, -5.6, 11.9, 0.2, 0.4},
    {8, 4, -19.3, -17.4, -0.2, 0.4},
    {8, 5, 11.5, 16.7, 0.3, 0.1},
    {8, 6, 10.9, 7.0, 0.3, -0.1},
    {8, 7, -14.1, -10.8, -0.6, 0.4},
    {8, 8, -3.7, 1.7, 0.2, 0.3},
    {9, 0, 5.4, 0.0, 0.0, 0.0},
    {9, 1, 9.4, -20.5, -0.1, 0.0},
    {9, 2, 3.4, 11.5, 0.0, -0.2},
    {9, 3, -5.2, 12.8, 0.3, 0.0},
    {9, 4, 3.1, -7.2, -0.4, -0.1},
    {9, 5, -12.4, -7.4, -0.3, 0.1},
    {9, 6, -0.7, 8.0, 0.1, 0.0},
    {9, 7, 8.4, 2.1, -0.1, -0.2},
    {9, 8, -8.5, -6.1, -0.4, 0.3},
    {9, 9, -10.1, 7.0, -0.2, 0.2},
    {10, 0, -2.0, 0.0, 0.0, 0.0},
    {10, 1, -6.3, 2.8, 0.0, 0.1},
    {10, 2, 0.9, -0.1, -0.1, -0.1},
    {10, 3, -1.1, 4.7, 0.2, 0.0},
    {10, 4, -0.2, 4.4, 0.0, -0.1},
    {10, 5, 2.5, -7.2, -0.1, -0.1},
    {10, 6, -0.3, -1.0, -0.2, 0.0},
    {10, 7, 2.2, -3.9, 0.0, -0.1},
    {10, 8, 3.1, -2.0, -0.1, -0.2},
    {10, 9, -1.0, -2.0, -0.2, 0.0},
    {10, 10, -2.8, -8.3, -0.2, -0.1},
    {11, 0, 3.0, 0.0, 0.0, 0.0},
    {11, 1, -1.5, 0.2, 0.0, 0.0},
    {11, 2, -2.1, 1.7, 0.0, 0.1},
    {11, 3, 1.7, -0.6, 0.1, 0.0},
    {11, 4, -0.5, -1.8, 0.0, 0.1},
    {11, 5, 0.5, 0.9, 0.0, 0.0},
    {11, 6, -0.8, -0.4, 0.0, 0.1},
    {11, 7, 0.4, -2.5, 0.0, 0.0},
    {11, 8, 1.8, -1.3, 0.0, -0.1},
    {11, 9, 0.1, -2.1, 0.0, -0.1},
    {11, 10, 0.7, -1.9, -0.1, 0.0},
    {11, 11, 3.8, -1.8, 0.0, -0.1},
    {12, 0, -2.2, 0.0, 0.0, 0.0},
    {12, 1, -0.2, -0.9, 0.0, 0.0},
    {12, 2, 0.3, 0.3, 0.1, 0.0},
    {12, 3, 1.0, 2.1, 0.1, 0.0},
    {12, 4, -0.6, -2.5, -0.1, 0.0},
    {12, 5, 0.9, 0.5, 0.0, 0.0},
    {12, 6, -0.1, 0.6, 0.0, 0.1},
    {12, 7, 0.5, 0.0, 0.0, 0.0},
    {12, 8, -0.4, 0.1, 0.0, 0.0},
    {12, 9, -0.4, 0.3, 0.0, 0.0},
    {12, 10, 0.2, -0.9, 0.0, 0.0},
    {12, 11, -0.8, -0.2, -0.1, 0.0},
    {12, 12, 0.0, 0.9, 0.1, 0.0}
};

namespace Utils {

    WorldMagModel::WorldMagModel()
    {
        Initialize();
    }

    int WorldMagModel::GetMagVector(double LLA[3], int Month, int Day, int Year, double Be[3])
    {
        double Lat = LLA[0];
        double Lon = LLA[1];
        double AltEllipsoid = LLA[2];

        // ***********
        // range check supplied params

        if (Lat <  -90) return -1;  // error
        if (Lat >   90) return -2;  // error

        if (Lon < -180) return -3;  // error
        if (Lon >  180) return -4;  // error

        // ***********

        WMMtype_CoordSpherical CoordSpherical;
        WMMtype_CoordGeodetic CoordGeodetic;
        WMMtype_GeoMagneticElements GeoMagneticElements;

        Initialize();

        CoordGeodetic.lambda = Lon;
        CoordGeodetic.phi = Lat;
        CoordGeodetic.HeightAboveEllipsoid = AltEllipsoid;

        // Convert from geodeitic to Spherical Equations: 17-18, WMM Technical report
        GeodeticToSpherical(&CoordGeodetic, &CoordSpherical);

        if (DateToYear(Month, Day, Year) < 0)
            return -5;  // error

        // Compute the geoMagnetic field elements and their time change
        if (Geomag(&CoordSpherical, &CoordGeodetic, &GeoMagneticElements) < 0)
            return -6;  // error

        // set the returned values
        Be[0] = GeoMagneticElements.X * 1e-2;
        Be[1] = GeoMagneticElements.Y * 1e-2;
        Be[2] = GeoMagneticElements.Z * 1e-2;

        // ***********

        return 0;   // OK
    }

    void WorldMagModel::Initialize()
    {   //      Sets default values for WMM subroutines.
        //      UPDATES : Ellip and MagneticModel

        // Sets WGS-84 parameters
        Ellip.a = 6378.137;	// semi-major axis of the ellipsoid in km
        Ellip.b = 6356.7523142;	// semi-minor axis of the ellipsoid in km
        Ellip.fla = 1 / 298.257223563;	// flattening
        Ellip.eps = sqrt(1 - (Ellip.b * Ellip.b) / (Ellip.a * Ellip.a));	// first eccentricity
        Ellip.epssq = (Ellip.eps * Ellip.eps);	// first eccentricity squared
        Ellip.re = 6371.2;	// Earth's radius in km

        // Sets Magnetic Model parameters
        MagneticModel.nMax = WMM_MAX_MODEL_DEGREES;
        MagneticModel.nMaxSecVar = WMM_MAX_SECULAR_VARIATION_MODEL_DEGREES;
        MagneticModel.SecularVariationUsed = 0;

        // Really, Really needs to be read from a file - out of date in 2015 at latest
        MagneticModel.EditionDate = 5.7863328170559505e-307;
        MagneticModel.epoch = 2010.0;
        sprintf(MagneticModel.ModelName, "WMM-2010");
    }


    int WorldMagModel::Geomag(WMMtype_CoordSpherical *CoordSpherical, WMMtype_CoordGeodetic *CoordGeodetic, WMMtype_GeoMagneticElements *GeoMagneticElements)
    /*
      The main subroutine that calls a sequence of WMM sub-functions to calculate the magnetic field elements for a single point.
      The function expects the model coefficients and point coordinates as input and returns the magnetic field elements and
      their rate of change. Though, this subroutine can be called successively to calculate a time series, profile or grid
      of magnetic field, these are better achieved by the subroutine WMM_Grid.

      INPUT: Ellip
      CoordSpherical
      CoordGeodetic
      TimedMagneticModel

      OUTPUT : GeoMagneticElements
    */
    {
        WMMtype_MagneticResults             MagneticResultsSph;
        WMMtype_MagneticResults             MagneticResultsGeo;
        WMMtype_MagneticResults             MagneticResultsSphVar;
        WMMtype_MagneticResults             MagneticResultsGeoVar;
        WMMtype_LegendreFunction            LegendreFunction;
        WMMtype_SphericalHarmonicVariables  SphVariables;

        // Compute Spherical Harmonic variables
        ComputeSphericalHarmonicVariables(CoordSpherical, MagneticModel.nMax, &SphVariables);

        // Compute ALF
        if (AssociatedLegendreFunction(CoordSpherical, MagneticModel.nMax, &LegendreFunction) < 0)
            return -1; // error

        // Accumulate the spherical harmonic coefficients
        Summation(&LegendreFunction, &SphVariables, CoordSpherical, &MagneticResultsSph);

        // Sum the Secular Variation Coefficients
        SecVarSummation(&LegendreFunction, &SphVariables, CoordSpherical, &MagneticResultsSphVar);

        // Map the computed Magnetic fields to Geodeitic coordinates
        RotateMagneticVector(CoordSpherical, CoordGeodetic, &MagneticResultsSph, &MagneticResultsGeo);

        // Map the secular variation field components to Geodetic coordinates
        RotateMagneticVector(CoordSpherical, CoordGeodetic, &MagneticResultsSphVar, &MagneticResultsGeoVar);

        // Calculate the Geomagnetic elements, Equation 18 , WMM Technical report
        CalculateGeoMagneticElements(&MagneticResultsGeo, GeoMagneticElements);

        // Calculate the secular variation of each of the Geomagnetic elements
        CalculateSecularVariation(&MagneticResultsGeoVar, GeoMagneticElements);

        return 0;   // OK
    }

    void WorldMagModel::ComputeSphericalHarmonicVariables(WMMtype_CoordSpherical *CoordSpherical, int nMax, WMMtype_SphericalHarmonicVariables *SphVariables)
    {
       /* Computes Spherical variables
      Variables computed are (a/r)^(n+2), cos_m(lamda) and sin_m(lambda) for spherical harmonic
      summations. (Equations 10-12 in the WMM Technical Report)
      INPUT   Ellip  data  structure with the following elements
      float a; semi-major axis of the ellipsoid
      float b; semi-minor axis of the ellipsoid
      float fla;  flattening
      float epssq; first eccentricity squared
      float eps;  first eccentricity
      float re; mean radius of  ellipsoid
      CoordSpherical    A data structure with the following elements
      float lambda; ( longitude)
      float phig; ( geocentric latitude )
      float r;            ( distance from the center of the ellipsoid)
      nMax   integer     ( Maxumum degree of spherical harmonic secular model)\

      OUTPUT  SphVariables  Pointer to the   data structure with the following elements
      float RelativeRadiusPower[WMM_MAX_MODEL_DEGREES+1];   [earth_reference_radius_km  sph. radius ]^n
      float cos_mlambda[WMM_MAX_MODEL_DEGREES+1]; cp(m)  - cosine of (mspherical coord. longitude)
      float sin_mlambda[WMM_MAX_MODEL_DEGREES+1];  sp(m)  - sine of (mspherical coord. longitude)
    */
        double cos_lambda = cos(DEG2RAD(CoordSpherical->lambda));
        double sin_lambda = sin(DEG2RAD(CoordSpherical->lambda));

        /* for n = 0 ... model_order, compute (Radius of Earth / Spherica radius r)^(n+2)
           for n  1..nMax-1 (this is much faster than calling pow MAX_N+1 times).      */

        SphVariables->RelativeRadiusPower[0] = (Ellip.re / CoordSpherical->r) * (Ellip.re / CoordSpherical->r);
        for (int n = 1; n <= nMax; n++)
            SphVariables->RelativeRadiusPower[n] = SphVariables->RelativeRadiusPower[n - 1] * (Ellip.re / CoordSpherical->r);

        /*
           Compute cos(m*lambda), sin(m*lambda) for m = 0 ... nMax
           cos(a + b) = cos(a)*cos(b) - sin(a)*sin(b)
           sin(a + b) = cos(a)*sin(b) + sin(a)*cos(b)
         */
        SphVariables->cos_mlambda[0] = 1.0;
        SphVariables->sin_mlambda[0] = 0.0;

        SphVariables->cos_mlambda[1] = cos_lambda;
        SphVariables->sin_mlambda[1] = sin_lambda;
        for (int m = 2; m <= nMax; m++)
        {
            SphVariables->cos_mlambda[m] = SphVariables->cos_mlambda[m - 1] * cos_lambda - SphVariables->sin_mlambda[m - 1] * sin_lambda;
            SphVariables->sin_mlambda[m] = SphVariables->cos_mlambda[m - 1] * sin_lambda + SphVariables->sin_mlambda[m - 1] * cos_lambda;
        }
    }

    int WorldMagModel::AssociatedLegendreFunction(WMMtype_CoordSpherical *CoordSpherical, int nMax, WMMtype_LegendreFunction *LegendreFunction)
    {
        /* Computes  all of the Schmidt-semi normalized associated Legendre
           functions up to degree nMax. If nMax <= 16, function WMM_PcupLow is used.
           Otherwise WMM_PcupHigh is called.
           INPUT  CoordSpherical        A data structure with the following elements
           float lambda; ( longitude)
           float phig; ( geocentric latitude )
           float r;       ( distance from the center of the ellipsoid)
           nMax         integer          ( Maxumum degree of spherical harmonic secular model)
           LegendreFunction Pointer to data structure with the following elements
           float *Pcup;  (  pointer to store Legendre Function  )
           float *dPcup; ( pointer to store  Derivative of Lagendre function )

           OUTPUT  LegendreFunction  Calculated Legendre variables in the data structure
         */

        double sin_phi = sin(DEG2RAD(CoordSpherical->phig));	// sin  (geocentric latitude)

        if (nMax <= 16 || (1 - fabs(sin_phi)) < 1.0e-10)	/* If nMax is less tha 16 or at the poles */
            PcupLow(LegendreFunction->Pcup, LegendreFunction->dPcup, sin_phi, nMax);
        else
        {
            if (PcupHigh(LegendreFunction->Pcup, LegendreFunction->dPcup, sin_phi, nMax) < 0)
                return -1;  // error
        }

        return 0;   // OK
    }

    void WorldMagModel::Summation(  WMMtype_LegendreFunction *LegendreFunction,
                                    WMMtype_SphericalHarmonicVariables *SphVariables,
                                    WMMtype_CoordSpherical *CoordSpherical,
                                    WMMtype_MagneticResults *MagneticResults)
    {
        /* Computes Geomagnetic Field Elements X, Y and Z in Spherical coordinate system using spherical harmonic summation.

           The vector Magnetic field is given by -grad V, where V is Geomagnetic scalar potential
           The gradient in spherical coordinates is given by:

           dV ^     1 dV ^        1     dV ^
           grad V = -- r  +  - -- t  +  -------- -- p
           dr       r dt       r sin(t) dp

           INPUT :  LegendreFunction
           MagneticModel
           SphVariables
           CoordSpherical
           OUTPUT : MagneticResults

           Manoj Nair, June, 2009 Manoj.C.Nair@Noaa.Gov
         */

        MagneticResults->Bz = 0.0;
        MagneticResults->By = 0.0;
        MagneticResults->Bx = 0.0;

        for (int n = 1; n <= MagneticModel.nMax; n++)
        {
            for (int m = 0; m <= n; m++)
            {
                int index = (n * (n + 1) / 2 + m);

/*		    nMax  	(n+2) 	  n     m            m           m
    Bz =   -SUM (a/r)   (n+1) SUM  [g cos(m p) + h sin(m p)] P (sin(phi))
            n=1      	      m=0   n            n           n  */
/* Equation 12 in the WMM Technical report.  Derivative with respect to radius.*/
                MagneticResults->Bz -=
                    SphVariables->RelativeRadiusPower[n] *
                     (get_main_field_coeff_g(index) *
                     SphVariables->cos_mlambda[m] + get_main_field_coeff_h(index) * SphVariables->sin_mlambda[m])
                    * (double)(n + 1) * LegendreFunction->Pcup[index];

/*		  1 nMax  (n+2)    n     m            m           m
    By =    SUM (a/r) (m)  SUM  [g cos(m p) + h sin(m p)] dP (sin(phi))
           n=1             m=0   n            n           n  */
/* Equation 11 in the WMM Technical report. Derivative with respect to longitude, divided by radius. */
                MagneticResults->By +=
                    SphVariables->RelativeRadiusPower[n] *
                    (get_main_field_coeff_g(index) *
                     SphVariables->sin_mlambda[m] - get_main_field_coeff_h(index) * SphVariables->cos_mlambda[m])
                    * (double)(m) * LegendreFunction->Pcup[index];
/*		   nMax  (n+2) n     m            m           m
    Bx = - SUM (a/r)   SUM  [g cos(m p) + h sin(m p)] dP (sin(phi))
           n=1         m=0   n            n           n  */
/* Equation 10  in the WMM Technical report. Derivative with respect to latitude, divided by radius. */

                MagneticResults->Bx -=
                    SphVariables->RelativeRadiusPower[n] *
                    (get_main_field_coeff_g(index) *
                     SphVariables->cos_mlambda[m] + get_main_field_coeff_h(index) * SphVariables->sin_mlambda[m])
                    * LegendreFunction->dPcup[index];

            }
        }

        double cos_phi = cos(DEG2RAD(CoordSpherical->phig));
        if (fabs(cos_phi) > 1.0e-10)
        {
            MagneticResults->By = MagneticResults->By / cos_phi;
        }
        else
        {
            /* Special calculation for component - By - at Geographic poles.
             * If the user wants to avoid using this function,  please make sure that
             * the latitude is not exactly +/-90. An option is to make use the function
             * WMM_CheckGeographicPoles.
             */
            SummationSpecial(SphVariables, CoordSpherical, MagneticResults);
        }
    }

    void WorldMagModel::SecVarSummation(    WMMtype_LegendreFunction *LegendreFunction,
                                            WMMtype_SphericalHarmonicVariables *SphVariables,
                                            WMMtype_CoordSpherical *CoordSpherical,
                                            WMMtype_MagneticResults *MagneticResults)
    {
        /*This Function sums the secular variation coefficients to get the secular variation of the Magnetic vector.
           INPUT :  LegendreFunction
           MagneticModel
           SphVariables
           CoordSpherical
           OUTPUT : MagneticResults
         */

        MagneticModel.SecularVariationUsed = true;

        MagneticResults->Bz = 0.0;
        MagneticResults->By = 0.0;
        MagneticResults->Bx = 0.0;

        for (int n = 1; n <= MagneticModel.nMaxSecVar; n++)
        {
            for (int m = 0; m <= n; m++)
            {
                int index = (n * (n + 1) / 2 + m);

/*		    nMax  	(n+2) 	  n     m            m           m
    Bz =   -SUM (a/r)   (n+1) SUM  [g cos(m p) + h sin(m p)] P (sin(phi))
            n=1      	      m=0   n            n           n  */
/*  Derivative with respect to radius.*/
                MagneticResults->Bz -=
                    SphVariables->RelativeRadiusPower[n] *
                    (get_secular_var_coeff_g(index) *
                     SphVariables->cos_mlambda[m] + get_secular_var_coeff_h(index) * SphVariables->sin_mlambda[m])
                    * (double)(n + 1) * LegendreFunction->Pcup[index];

/*		  1 nMax  (n+2)    n     m            m           m
    By =    SUM (a/r) (m)  SUM  [g cos(m p) + h sin(m p)] dP (sin(phi))
           n=1             m=0   n            n           n  */
/* Derivative with respect to longitude, divided by radius. */
                MagneticResults->By +=
                    SphVariables->RelativeRadiusPower[n] *
                    (get_secular_var_coeff_g(index) *
                     SphVariables->sin_mlambda[m] - get_secular_var_coeff_h(index) * SphVariables->cos_mlambda[m])
                    * (double)(m) * LegendreFunction->Pcup[index];
/*		   nMax  (n+2) n     m            m           m
    Bx = - SUM (a/r)   SUM  [g cos(m p) + h sin(m p)] dP (sin(phi))
           n=1         m=0   n            n           n  */
/* Derivative with respect to latitude, divided by radius. */

                MagneticResults->Bx -=
                    SphVariables->RelativeRadiusPower[n] *
                    (get_secular_var_coeff_g(index) *
                     SphVariables->cos_mlambda[m] + get_secular_var_coeff_h(index) * SphVariables->sin_mlambda[m])
                    * LegendreFunction->dPcup[index];
            }
        }

        double cos_phi = cos(DEG2RAD(CoordSpherical->phig));
        if (fabs(cos_phi) > 1.0e-10)
        {
            MagneticResults->By = MagneticResults->By / cos_phi;
        }
        else
        {    /* Special calculation for component By at Geographic poles */
            SecVarSummationSpecial(SphVariables, CoordSpherical, MagneticResults);
        }
    }

    void WorldMagModel::RotateMagneticVector(   WMMtype_CoordSpherical *CoordSpherical,
                                                WMMtype_CoordGeodetic *CoordGeodetic,
                                                WMMtype_MagneticResults *MagneticResultsSph,
                                                WMMtype_MagneticResults *MagneticResultsGeo)
    {
        /* Rotate the Magnetic Vectors to Geodetic Coordinates
           Manoj Nair, June, 2009 Manoj.C.Nair@Noaa.Gov
           Equation 16, WMM Technical report

           INPUT : CoordSpherical : Data structure WMMtype_CoordSpherical with the following elements
           float lambda; ( longitude)
           float phig; ( geocentric latitude )
           float r;       ( distance from the center of the ellipsoid)

           CoordGeodetic : Data structure WMMtype_CoordGeodetic with the following elements
           float lambda; (longitude)
           float phi; ( geodetic latitude)
           float HeightAboveEllipsoid; (height above the ellipsoid (HaE) )
           float HeightAboveGeoid;(height above the Geoid )

           MagneticResultsSph : Data structure WMMtype_MagneticResults with the following elements
           float Bx;     North
           float By;       East
           float Bz;    Down

           OUTPUT: MagneticResultsGeo Pointer to the data structure WMMtype_MagneticResults, with the following elements
           float Bx;     North
           float By;       East
           float Bz;    Down
         */

        /* Difference between the spherical and Geodetic latitudes */
        double Psi = DEG2RAD(CoordSpherical->phig - CoordGeodetic->phi);

        /* Rotate spherical field components to the Geodeitic system */
        MagneticResultsGeo->Bz = MagneticResultsSph->Bx * sin(Psi) + MagneticResultsSph->Bz * cos(Psi);
        MagneticResultsGeo->Bx = MagneticResultsSph->Bx * cos(Psi) - MagneticResultsSph->Bz * sin(Psi);
        MagneticResultsGeo->By = MagneticResultsSph->By;
    }

    void WorldMagModel::CalculateGeoMagneticElements(WMMtype_MagneticResults *MagneticResultsGeo, WMMtype_GeoMagneticElements *GeoMagneticElements)
    {
        /* Calculate all the Geomagnetic elements from X,Y and Z components
           INPUT     MagneticResultsGeo   Pointer to data structure with the following elements
           float Bx;    ( North )
           float By;      ( East )
           float Bz;    ( Down )
           OUTPUT    GeoMagneticElements    Pointer to data structure with the following elements
           float Decl; (Angle between the magnetic field vector and true north, positive east)
           float Incl; Angle between the magnetic field vector and the horizontal plane, positive down
           float F; Magnetic Field Strength
           float H; Horizontal Magnetic Field Strength
           float X; Northern component of the magnetic field vector
           float Y; Eastern component of the magnetic field vector
           float Z; Downward component of the magnetic field vector
         */

        GeoMagneticElements->X = MagneticResultsGeo->Bx;
        GeoMagneticElements->Y = MagneticResultsGeo->By;
        GeoMagneticElements->Z = MagneticResultsGeo->Bz;

        GeoMagneticElements->H = sqrt(MagneticResultsGeo->Bx * MagneticResultsGeo->Bx + MagneticResultsGeo->By * MagneticResultsGeo->By);
        GeoMagneticElements->F = sqrt(GeoMagneticElements->H * GeoMagneticElements->H + MagneticResultsGeo->Bz * MagneticResultsGeo->Bz);
        GeoMagneticElements->Decl = RAD2DEG(atan2(GeoMagneticElements->Y, GeoMagneticElements->X));
        GeoMagneticElements->Incl = RAD2DEG(atan2(GeoMagneticElements->Z, GeoMagneticElements->H));
    }

    void WorldMagModel::CalculateSecularVariation(WMMtype_MagneticResults *MagneticVariation, WMMtype_GeoMagneticElements *MagneticElements)
    {
        /* This takes the Magnetic Variation in x, y, and z and uses it to calculate the secular variation of each of the Geomagnetic elements.
        INPUT     MagneticVariation   Data structure with the following elements
                    float Bx;    ( North )
                    float By;	  ( East )
                    float Bz;    ( Down )
        OUTPUT   MagneticElements   Pointer to the data  structure with the following elements updated
                float Decldot; Yearly Rate of change in declination
                float Incldot; Yearly Rate of change in inclination
                float Fdot; Yearly rate of change in Magnetic field strength
                float Hdot; Yearly rate of change in horizontal field strength
                float Xdot; Yearly rate of change in the northern component
                float Ydot; Yearly rate of change in the eastern component
                float Zdot; Yearly rate of change in the downward component
                float GVdot;Yearly rate of chnage in grid variation
        */

        MagneticElements->Xdot = MagneticVariation->Bx;
        MagneticElements->Ydot = MagneticVariation->By;
        MagneticElements->Zdot = MagneticVariation->Bz;
        MagneticElements->Hdot = (MagneticElements->X * MagneticElements->Xdot + MagneticElements->Y * MagneticElements->Ydot) / MagneticElements->H;	//See equation 19 in the WMM technical report
        MagneticElements->Fdot =
            (MagneticElements->X * MagneticElements->Xdot +
             MagneticElements->Y * MagneticElements->Ydot + MagneticElements->Z * MagneticElements->Zdot) / MagneticElements->F;
        MagneticElements->Decldot =
            180.0 / M_PI * (MagneticElements->X * MagneticElements->Ydot -
                    MagneticElements->Y * MagneticElements->Xdot) / (MagneticElements->H * MagneticElements->H);
        MagneticElements->Incldot =
            180.0 / M_PI * (MagneticElements->H * MagneticElements->Zdot -
                    MagneticElements->Z * MagneticElements->Hdot) / (MagneticElements->F * MagneticElements->F);
        MagneticElements->GVdot = MagneticElements->Decldot;
    }

    int WorldMagModel::PcupHigh(double *Pcup, double *dPcup, double x, int nMax)
    {
        /*	This function evaluates all of the Schmidt-semi normalized associated Legendre
            functions up to degree nMax. The functions are initially scaled by
            10^280 sin^m in order to minimize the effects of underflow at large m
            near the poles (see Holmes and Featherstone 2002, J. Geodesy, 76, 279-299).
            Note that this function performs the same operation as WMM_PcupLow.
            However this function also can be used for high degree (large nMax) models.

            Calling Parameters:
                INPUT
                    nMax:	 Maximum spherical harmonic degree to compute.
                    x:		cos(colatitude) or sin(latitude).

                OUTPUT
                    Pcup:	A vector of all associated Legendgre polynomials evaluated at
                            x up to nMax. The lenght must by greater or equal to (nMax+1)*(nMax+2)/2.
                  dPcup:   Derivative of Pcup(x) with respect to latitude
            Notes:

          Adopted from the FORTRAN code written by Mark Wieczorek September 25, 2005.

          Manoj Nair, Nov, 2009 Manoj.C.Nair@Noaa.Gov

          Change from the previous version
          The prevous version computes the derivatives as
          dP(n,m)(x)/dx, where x = sin(latitude) (or cos(colatitude) ).
          However, the WMM Geomagnetic routines requires dP(n,m)(x)/dlatitude.
          Hence the derivatives are multiplied by sin(latitude).
          Removed the options for CS phase and normalizations.

          Note: In geomagnetism, the derivatives of ALF are usually found with
          respect to the colatitudes. Here the derivatives are found with respect
          to the latitude. The difference is a sign reversal for the derivative of
          the Associated Legendre Functions.

          The derivates can't be computed for latitude = |90| degrees.
            */
        double  f1[WMM_NUMPCUP];
        double  f2[WMM_NUMPCUP];
        double  PreSqr[WMM_NUMPCUP];
        int m;

        if (fabs(x) == 1.0)
        {
            // printf("Error in PcupHigh: derivative cannot be calculated at poles\n");
            return -2;
        }

        double scalef = 1.0e-280;

        for (int n = 0; n <= 2 * nMax + 1; ++n)
            PreSqr[n] = sqrt((double)(n));

        int k = 2;

        for (int n = 2; n <= nMax; n++)
        {
            k = k + 1;
            f1[k] = (double)(2 * n - 1) / n;
            f2[k] = (double)(n - 1) / n;
            for (int m = 1; m <= n - 2; m++)
            {
                k = k + 1;
                f1[k] = (double)(2 * n - 1) / PreSqr[n + m] / PreSqr[n - m];
                f2[k] = PreSqr[n - m - 1] * PreSqr[n + m - 1] / PreSqr[n + m] / PreSqr[n - m];
            }
            k = k + 2;
        }

        /*z = sin (geocentric latitude) */
        double z = sqrt((1.0 - x) * (1.0 + x));
        double pm2 = 1.0;
        Pcup[0] = 1.0;
        dPcup[0] = 0.0;
        if (nMax == 0)
            return -3;
        double pm1 = x;
        Pcup[1] = pm1;
        dPcup[1] = z;
        k = 1;

        for (int n = 2; n <= nMax; n++)
        {
            k = k + n;
            double plm = f1[k] * x * pm1 - f2[k] * pm2;
            Pcup[k] = plm;
            dPcup[k] = (double)(n) * (pm1 - x * plm) / z;
            pm2 = pm1;
            pm1 = plm;
        }

        double pmm = PreSqr[2] * scalef;
        double rescalem = 1.0 / scalef;
        int kstart = 0;

        for (m = 1; m <= nMax - 1; ++m)
        {
            rescalem = rescalem * z;

            /* Calculate Pcup(m,m) */
            kstart = kstart + m + 1;
            pmm = pmm * PreSqr[2 * m + 1] / PreSqr[2 * m];
            Pcup[kstart] = pmm * rescalem / PreSqr[2 * m + 1];
            dPcup[kstart] = -((double)(m) * x * Pcup[kstart] / z);
            pm2 = pmm / PreSqr[2 * m + 1];
            /* Calculate Pcup(m+1,m) */
            k = kstart + m + 1;
            pm1 = x * PreSqr[2 * m + 1] * pm2;
            Pcup[k] = pm1 * rescalem;
            dPcup[k] = ((pm2 * rescalem) * PreSqr[2 * m + 1] - x * (double)(m + 1) * Pcup[k]) / z;
            /* Calculate Pcup(n,m) */
            for (int n = m + 2; n <= nMax; ++n)
            {
                k = k + n;
                double plm = x * f1[k] * pm1 - f2[k] * pm2;
                Pcup[k] = plm * rescalem;
                dPcup[k] = (PreSqr[n + m] * PreSqr[n - m] * (pm1 * rescalem) - (double)(n) * x * Pcup[k]) / z;
                pm2 = pm1;
                pm1 = plm;
            }
        }

        /* Calculate Pcup(nMax,nMax) */
        rescalem = rescalem * z;
        kstart = kstart + m + 1;
        pmm = pmm / PreSqr[2 * nMax];
        Pcup[kstart] = pmm * rescalem;
        dPcup[kstart] = -(double)(nMax) * x * Pcup[kstart] / z;

        // *********

        return 0;   // OK
    }

    void WorldMagModel::PcupLow(double *Pcup, double *dPcup, double x, int nMax)
    {
        /*   This function evaluates all of the Schmidt-semi normalized associated Legendre functions up to degree nMax.

            Calling Parameters:
                INPUT
                    nMax:	 Maximum spherical harmonic degree to compute.
                    x:		cos(colatitude) or sin(latitude).

               OUTPUT
                   Pcup:	A vector of all associated Legendgre polynomials evaluated at
                           x up to nMax.
                  dPcup: Derivative of Pcup(x) with respect to latitude

           Notes: Overflow may occur if nMax > 20 , especially for high-latitudes.
           Use WMM_PcupHigh for large nMax.

          Writted by Manoj Nair, June, 2009 . Manoj.C.Nair@Noaa.Gov.

          Note: In geomagnetism, the derivatives of ALF are usually found with
          respect to the colatitudes. Here the derivatives are found with respect
          to the latitude. The difference is a sign reversal for the derivative of
          the Associated Legendre Functions.
        */

        double schmidtQuasiNorm[WMM_NUMPCUP];

        Pcup[0] = 1.0;
        dPcup[0] = 0.0;

        /*sin (geocentric latitude) - sin_phi */
        double z = sqrt((1.0 - x) * (1.0 + x));

        /*       First, Compute the Gauss-normalized associated Legendre  functions */
        for (int n = 1; n <= nMax; n++)
        {
            for (int m = 0; m <= n; m++)
            {
                int index = (n * (n + 1) / 2 + m);
                if (n == m)
                {
                    int index1 = (n - 1) * n / 2 + m - 1;
                    Pcup[index] = z * Pcup[index1];
                    dPcup[index] = z * dPcup[index1] + x * Pcup[index1];
                }
                else
                if (n == 1 && m == 0)
                {
                    int index1 = (n - 1) * n / 2 + m;
                    Pcup[index] = x * Pcup[index1];
                    dPcup[index] = x * dPcup[index1] - z * Pcup[index1];
                }
                else
                if (n > 1 && n != m)
                {
                    int index1 = (n - 2) * (n - 1) / 2 + m;
                    int index2 = (n - 1) * n / 2 + m;
                    if (m > n - 2)
                    {
                        Pcup[index] = x * Pcup[index2];
                        dPcup[index] = x * dPcup[index2] - z * Pcup[index2];
                    }
                    else
                    {
                        double k = (double)(((n - 1) * (n - 1)) - (m * m)) / (double)((2 * n - 1) * (2 * n - 3));
                        Pcup[index] = x * Pcup[index2] - k * Pcup[index1];
                        dPcup[index] = x * dPcup[index2] - z * Pcup[index2] - k * dPcup[index1];
                    }
                }
            }
        }

        /*Compute the ration between the Gauss-normalized associated Legendre
          functions and the Schmidt quasi-normalized version. This is equivalent to
        sqrt((m==0?1:2)*(n-m)!/(n+m!))*(2n-1)!!/(n-m)!  */

        schmidtQuasiNorm[0] = 1.0;
        for (int n = 1; n <= nMax; n++)
        {
            int index = (n * (n + 1) / 2);
            int index1 = (n - 1) * n / 2;
            /* for m = 0 */
            schmidtQuasiNorm[index] = schmidtQuasiNorm[index1] * (double)(2 * n - 1) / (double)n;

            for (int m = 1; m <= n; m++)
            {
                index = (n * (n + 1) / 2 + m);
                index1 = (n * (n + 1) / 2 + m - 1);
                schmidtQuasiNorm[index] = schmidtQuasiNorm[index1] * sqrt((double)((n - m + 1) * (m == 1 ? 2 : 1)) / (double)(n + m));
            }
        }

        /* Converts the  Gauss-normalized associated Legendre
              functions to the Schmidt quasi-normalized version using pre-computed
              relation stored in the variable schmidtQuasiNorm */

        for (int n = 1; n <= nMax; n++)
        {
            for (int m = 0; m <= n; m++)
            {
                int index = (n * (n + 1) / 2 + m);
                Pcup[index] = Pcup[index] * schmidtQuasiNorm[index];
                dPcup[index] = -dPcup[index] * schmidtQuasiNorm[index];
                /* The sign is changed since the new WMM routines use derivative with respect to latitude insted of co-latitude */
            }
        }
    }

    void WorldMagModel::SummationSpecial(WMMtype_SphericalHarmonicVariables *SphVariables, WMMtype_CoordSpherical *CoordSpherical, WMMtype_MagneticResults *MagneticResults)
    {
        /* Special calculation for the component By at Geographic poles.
           Manoj Nair, June, 2009 manoj.c.nair@noaa.gov
           INPUT: MagneticModel
           SphVariables
           CoordSpherical
           OUTPUT: MagneticResults
           CALLS : none
           See Section 1.4, "SINGULARITIES AT THE GEOGRAPHIC POLES", WMM Technical report
         */

        double PcupS[WMM_NUMPCUPS];

        PcupS[0] = 1;
        double schmidtQuasiNorm1 = 1.0;

        MagneticResults->By = 0.0;
        double sin_phi = sin(DEG2RAD(CoordSpherical->phig));

        for (int n = 1; n <= MagneticModel.nMax; n++)
        {
            /*Compute the ration between the Gauss-normalized associated Legendre
               functions and the Schmidt quasi-normalized version. This is equivalent to
               sqrt((m==0?1:2)*(n-m)!/(n+m!))*(2n-1)!!/(n-m)!  */

            int index = (n * (n + 1) / 2 + 1);
            double schmidtQuasiNorm2 = schmidtQuasiNorm1 * (double)(2 * n - 1) / (double)n;
            double schmidtQuasiNorm3 = schmidtQuasiNorm2 * sqrt((double)(n * 2) / (double)(n + 1));
            schmidtQuasiNorm1 = schmidtQuasiNorm2;
            if (n == 1)
            {
                PcupS[n] = PcupS[n - 1];
            }
            else
            {
                double k = (double)(((n - 1) * (n - 1)) - 1) / (double)((2 * n - 1) * (2 * n - 3));
                PcupS[n] = sin_phi * PcupS[n - 1] - k * PcupS[n - 2];
            }

/*		  1 nMax  (n+2)    n     m            m           m
    By =    SUM (a/r) (m)  SUM  [g cos(m p) + h sin(m p)] dP (sin(phi))
           n=1             m=0   n            n           n  */
/* Equation 11 in the WMM Technical report. Derivative with respect to longitude, divided by radius. */

            MagneticResults->By +=
                SphVariables->RelativeRadiusPower[n] *
                (get_main_field_coeff_g(index) *
                 SphVariables->sin_mlambda[1] - get_main_field_coeff_h(index) * SphVariables->cos_mlambda[1])
                * PcupS[n] * schmidtQuasiNorm3;
        }
    }

    void WorldMagModel::SecVarSummationSpecial(WMMtype_SphericalHarmonicVariables *SphVariables, WMMtype_CoordSpherical *CoordSpherical, WMMtype_MagneticResults *MagneticResults)
    {
        /*Special calculation for the secular variation summation at the poles.

           INPUT: MagneticModel
           SphVariables
           CoordSpherical
           OUTPUT: MagneticResults
         */

        double PcupS[WMM_NUMPCUPS];

        PcupS[0] = 1;
        double schmidtQuasiNorm1 = 1.0;

        MagneticResults->By = 0.0;
        double sin_phi = sin(DEG2RAD(CoordSpherical->phig));

        for (int n = 1; n <= MagneticModel.nMaxSecVar; n++)
        {
            int index = (n * (n + 1) / 2 + 1);
            double schmidtQuasiNorm2 = schmidtQuasiNorm1 * (double)(2 * n - 1) / (double)n;
            double schmidtQuasiNorm3 = schmidtQuasiNorm2 * sqrt((double)(n * 2) / (double)(n + 1));
            schmidtQuasiNorm1 = schmidtQuasiNorm2;
            if (n == 1)
            {
                PcupS[n] = PcupS[n - 1];
            }
            else
            {
                double k = (double)(((n - 1) * (n - 1)) - 1) / (double)((2 * n - 1) * (2 * n - 3));
                PcupS[n] = sin_phi * PcupS[n - 1] - k * PcupS[n - 2];
            }

/*		  1 nMax  (n+2)    n     m            m           m
    By =    SUM (a/r) (m)  SUM  [g cos(m p) + h sin(m p)] dP (sin(phi))
           n=1             m=0   n            n           n  */
/* Derivative with respect to longitude, divided by radius. */

            MagneticResults->By +=
                SphVariables->RelativeRadiusPower[n] *
                (get_secular_var_coeff_g(index) *
                 SphVariables->sin_mlambda[1] - get_secular_var_coeff_h(index) * SphVariables->cos_mlambda[1])
                * PcupS[n] * schmidtQuasiNorm3;
        }
    }

    // brief Comput the MainFieldCoeffH accounting for the date
    double WorldMagModel::get_main_field_coeff_g(int index)
    {
        if (index >= WMM_NUMTERMS)
            return 0;

        double coeff = CoeffFile[index][2];

        int a = MagneticModel.nMaxSecVar;
        int b = (a * (a + 1) / 2 + a);
        for (int n = 1; n <= MagneticModel.nMax; n++)
        {
            for (int m = 0; m <= n; m++)
            {
                int sum_index = (n * (n + 1) / 2 + m);

                /* Hacky for now, will solve for which conditions need summing analytically */
                if (sum_index != index)
                    continue;

                if (index <= b)
                    coeff += (decimal_date - MagneticModel.epoch) * get_secular_var_coeff_g(sum_index);
            }
        }

        return coeff;
    }

    double WorldMagModel::get_main_field_coeff_h(int index)
    {
        if (index >= WMM_NUMTERMS)
            return 0;

        double coeff = CoeffFile[index][3];

        int a = MagneticModel.nMaxSecVar;
        int b = (a * (a + 1) / 2 + a);
        for (int n = 1; n <= MagneticModel.nMax; n++)
        {
            for (int m = 0; m <= n; m++)
            {
                int sum_index = (n * (n + 1) / 2 + m);

                /* Hacky for now, will solve for which conditions need summing analytically */
                if (sum_index != index)
                    continue;

                if (index <= b)
                    coeff += (decimal_date - MagneticModel.epoch) * get_secular_var_coeff_h(sum_index);
            }
        }

        return coeff;
    }

    double WorldMagModel::get_secular_var_coeff_g(int index)
    {
        if (index >= WMM_NUMTERMS)
            return 0;

        return CoeffFile[index][4];
    }

    double WorldMagModel::get_secular_var_coeff_h(int index)
    {
        if (index >= WMM_NUMTERMS)
            return 0;

        return CoeffFile[index][5];
    }

    int WorldMagModel::DateToYear(int month, int day, int year)
    {
        // Converts a given calendar date into a decimal year

        int temp = 0;	// Total number of days
        int MonthDays[13] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
        int ExtraDay = 0;

        if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0))
            ExtraDay = 1;
        MonthDays[2] += ExtraDay;

        /******************Validation********************************/

        if (month <= 0 || month > 12)
            return -1;  // error

        if (day <= 0 || day > MonthDays[month])
            return -2;  // error

        /****************Calculation of t***************************/
        for (int i = 1; i <= month; i++)
            temp += MonthDays[i - 1];
        temp += day;

        decimal_date = year + (temp - 1) / (365.0 + ExtraDay);

        return 0;   // OK
    }

    void WorldMagModel::GeodeticToSpherical(WMMtype_CoordGeodetic *CoordGeodetic, WMMtype_CoordSpherical *CoordSpherical)
    {
        // Converts Geodetic coordinates to Spherical coordinates
        // Convert geodetic coordinates, (defined by the WGS-84
        // reference ellipsoid), to Earth Centered Earth Fixed Cartesian
        // coordinates, and then to spherical coordinates.

        double CosLat = cos(DEG2RAD(CoordGeodetic->phi));
        double SinLat = sin(DEG2RAD(CoordGeodetic->phi));

        // compute the local radius of curvature on the WGS-84 reference ellipsoid
        double rc = Ellip.a / sqrt(1.0 - Ellip.epssq * SinLat * SinLat);

        // compute ECEF Cartesian coordinates of specified point (for longitude=0)
        double xp = (rc + CoordGeodetic->HeightAboveEllipsoid) * CosLat;
        double zp = (rc * (1.0 - Ellip.epssq) + CoordGeodetic->HeightAboveEllipsoid) * SinLat;

        // compute spherical radius and angle lambda and phi of specified point
        CoordSpherical->r = sqrt(xp * xp + zp * zp);
        CoordSpherical->phig = RAD2DEG(asin(zp / CoordSpherical->r));	// geocentric latitude
        CoordSpherical->lambda = CoordGeodetic->lambda;	// longitude
    }

}
