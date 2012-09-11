/**
 ******************************************************************************
 *
 * @file       worldmagmodel.h
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 *             Parts by Nokia Corporation (qt-info@nokia.com) Copyright (C) 2009.
 * @brief
 * @see        The GNU Public License (GPL) Version 3
 * @defgroup
 * @{
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

#ifndef WORLDMAGMODEL_H
#define WORLDMAGMODEL_H

#include "utils_global.h"

// ******************************
// internal structure definitions

#define WMM_MAX_MODEL_DEGREES                       12
#define WMM_MAX_SECULAR_VARIATION_MODEL_DEGREES     12
#define	WMM_NUMTERMS                                91		// ((WMM_MAX_MODEL_DEGREES + 1) * (WMM_MAX_MODEL_DEGREES + 2) / 2);
#define WMM_NUMPCUP                                 92		// NUMTERMS + 1
#define WMM_NUMPCUPS                                13		// WMM_MAX_MODEL_DEGREES + 1

typedef struct
{
    double EditionDate;
    double epoch;		//Base time of Geomagnetic model epoch (yrs)
    char ModelName[20];
//	double Main_Field_Coeff_G[WMM_NUMTERMS];	// C - Gauss coefficients of main geomagnetic model (nT)
//	double Main_Field_Coeff_H[WMM_NUMTERMS];	// C - Gauss coefficients of main geomagnetic model (nT)
//	double Secular_Var_Coeff_G[WMM_NUMTERMS];	// CD - Gauss coefficients of secular geomagnetic model (nT/yr)
//	double Secular_Var_Coeff_H[WMM_NUMTERMS];	// CD - Gauss coefficients of secular geomagnetic model (nT/yr)
    int nMax;		// Maximum degree of spherical harmonic model
    int nMaxSecVar;	// Maxumum degree of spherical harmonic secular model
    int SecularVariationUsed;	// Whether or not the magnetic secular variation vector will be needed by program
} WMMtype_MagneticModel;

typedef struct
{
    double a;		// semi-major axis of the ellipsoid
    double b;		// semi-minor axis of the ellipsoid
    double fla;		// flattening
    double epssq;	// first eccentricity squared
    double eps;		// first eccentricity
    double re;		// mean radius of  ellipsoid
} WMMtype_Ellipsoid;

typedef struct
{
    double lambda;	// longitude
    double phi;		// geodetic latitude
    double HeightAboveEllipsoid;	// height above the ellipsoid (HaE)
} WMMtype_CoordGeodetic;

typedef struct
{
    double lambda;	// longitude
    double phig;	// geocentric latitude
    double r;		// distance from the center of the ellipsoid
} WMMtype_CoordSpherical;

typedef struct
{
    int Year;
    int Month;
    int Day;
    double DecimalYear;
} WMMtype_Date;

typedef struct
{
    double Pcup[WMM_NUMPCUP];	// Legendre Function
    double dPcup[WMM_NUMPCUP];	// Derivative of Lagendre fn
} WMMtype_LegendreFunction;

typedef struct
{
    double Bx;		// North
    double By;		// East
    double Bz;		// Down
} WMMtype_MagneticResults;

typedef struct
{
    double RelativeRadiusPower[WMM_MAX_MODEL_DEGREES + 1];	// [earth_reference_radius_km / sph. radius ]^n
    double cos_mlambda[WMM_MAX_MODEL_DEGREES + 1];          // cp(m)  - cosine of (m*spherical coord. longitude
    double sin_mlambda[WMM_MAX_MODEL_DEGREES + 1];          // sp(m)  - sine of (m*spherical coord. longitude)
} WMMtype_SphericalHarmonicVariables;

typedef struct
{
    double Decl;		/*1. Angle between the magnetic field vector and true north, positive east */
    double Incl;		/*2. Angle between the magnetic field vector and the horizontal plane, positive down */
    double F;           /*3. Magnetic Field Strength */
    double H;           /*4. Horizontal Magnetic Field Strength */
    double X;           /*5. Northern component of the magnetic field vector */
    double Y;           /*6. Eastern component of the magnetic field vector */
    double Z;           /*7. Downward component of the magnetic field vector */
    double GV;          /*8. The Grid Variation */
    double Decldot;		/*9. Yearly Rate of change in declination */
    double Incldot;		/*10. Yearly Rate of change in inclination */
    double Fdot;		/*11. Yearly rate of change in Magnetic field strength */
    double Hdot;		/*12. Yearly rate of change in horizontal field strength */
    double Xdot;		/*13. Yearly rate of change in the northern component */
    double Ydot;		/*14. Yearly rate of change in the eastern component */
    double Zdot;		/*15. Yearly rate of change in the downward component */
    double GVdot;		/*16. Yearly rate of chnage in grid variation */
} WMMtype_GeoMagneticElements;

// ******************************

namespace Utils {

    class QTCREATOR_UTILS_EXPORT WorldMagModel
    {
        public:
            WorldMagModel();

            int GetMagVector(double LLA[3], int Month, int Day, int Year, double Be[3]);

        private:
            WMMtype_Ellipsoid       Ellip;
            WMMtype_MagneticModel   MagneticModel;

            double                  decimal_date;

            void Initialize();
            int Geomag(WMMtype_CoordSpherical *CoordSpherical, WMMtype_CoordGeodetic *CoordGeodetic, WMMtype_GeoMagneticElements *GeoMagneticElements);
            void ComputeSphericalHarmonicVariables(WMMtype_CoordSpherical *CoordSpherical, int nMax, WMMtype_SphericalHarmonicVariables *SphVariables);
            int AssociatedLegendreFunction(WMMtype_CoordSpherical *CoordSpherical, int nMax, WMMtype_LegendreFunction *LegendreFunction);
            void Summation(  WMMtype_LegendreFunction *LegendreFunction,
                             WMMtype_SphericalHarmonicVariables *SphVariables,
                             WMMtype_CoordSpherical *CoordSpherical,
                             WMMtype_MagneticResults *MagneticResults);
            void SecVarSummation(    WMMtype_LegendreFunction *LegendreFunction,
                                     WMMtype_SphericalHarmonicVariables *SphVariables,
                                     WMMtype_CoordSpherical *CoordSpherical,
                                     WMMtype_MagneticResults *MagneticResults);
            void RotateMagneticVector(   WMMtype_CoordSpherical *CoordSpherical,
                                         WMMtype_CoordGeodetic *CoordGeodetic,
                                         WMMtype_MagneticResults *MagneticResultsSph,
                                         WMMtype_MagneticResults *MagneticResultsGeo);
            void CalculateGeoMagneticElements(WMMtype_MagneticResults *MagneticResultsGeo, WMMtype_GeoMagneticElements *GeoMagneticElements);
            void CalculateSecularVariation(WMMtype_MagneticResults *MagneticVariation, WMMtype_GeoMagneticElements *MagneticElements);
            int PcupHigh(double *Pcup, double *dPcup, double x, int nMax);
            void PcupLow(double *Pcup, double *dPcup, double x, int nMax);
            void SummationSpecial(WMMtype_SphericalHarmonicVariables *SphVariables, WMMtype_CoordSpherical *CoordSpherical, WMMtype_MagneticResults *MagneticResults);
            void SecVarSummationSpecial(WMMtype_SphericalHarmonicVariables *SphVariables, WMMtype_CoordSpherical *CoordSpherical, WMMtype_MagneticResults *MagneticResults);
            double get_main_field_coeff_g(int index);
            double get_main_field_coeff_h(int index);
            double get_secular_var_coeff_g(int index);
            double get_secular_var_coeff_h(int index);
            int DateToYear(int month, int day, int year);
            void GeodeticToSpherical(WMMtype_CoordGeodetic *CoordGeodetic, WMMtype_CoordSpherical *CoordSpherical);
    };

}

// ******************************

#endif
