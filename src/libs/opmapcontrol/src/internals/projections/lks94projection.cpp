/**
******************************************************************************
*
* @file       lks94projection.cpp
* @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
* @brief      
* @see        The GNU Public License (GPL) Version 3
* @defgroup   OPMapWidget
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
#include "lks94projection.h"


 
namespace projections {
LKS94Projection::LKS94Projection():MinLatitude  (53.33 ), MaxLatitude  (56.55 ), MinLongitude  (20.22 ),
MaxLongitude  (27.11 ), orignX  (5122000 ), orignY  (10000100 ),tileSize(256, 256)
{
}

Size LKS94Projection::TileSize() const
{
    return tileSize;
}
double LKS94Projection::Axis() const
{
    return 6378137;
}
double LKS94Projection::Flattening() const
{

    return (1.0 / 298.257222101);

}

Point LKS94Projection::FromLatLngToPixel(double lat, double lng, int const&  zoom)
{
    Point ret;

    lat = Clip(lat, MinLatitude, MaxLatitude);
    lng = Clip(lng, MinLongitude, MaxLongitude);
    QVector <double> lks(3);
    lks[0]=lng;
    lks[1]=lat;
    lks = DTM10(lks);
    lks = MTD10(lks);
    lks = DTM00(lks);

    double res = GetTileMatrixResolution(zoom);

    ret.SetX((int) floor((lks[0] + orignX) / res));
    ret.SetY((int) floor((orignY - lks[1]) / res));

    return ret;
}

internals::PointLatLng LKS94Projection::FromPixelToLatLng(int const& x, int const&  y, int const&  zoom)
{
    internals::PointLatLng ret;// = internals::PointLatLng::Empty;

    double res = GetTileMatrixResolution(zoom);

    QVector <double> lks(2);
    lks[0]=(x * res) - orignX;
    lks[1]=-(y * res) + orignY;
    lks = MTD11(lks);
    lks = DTM10(lks);
    lks = MTD10(lks);
    ret.SetLat(Clip(lks[1], MinLatitude, MaxLatitude));
    ret.SetLng(Clip(lks[0], MinLongitude, MaxLongitude));
    return ret;
}

QVector <double> LKS94Projection::DTM10(const QVector <double>& lonlat)
{
    double es;              // Eccentricity squared : (a^2 - b^2)/a^2
    double semiMajor = 6378137.0;		// major axis
    double semiMinor = 6356752.3142451793;		// minor axis
    double ab;				// Semi_major / semi_minor
    double ba;				// Semi_minor / semi_major
    double ses;             // Second eccentricity squared : (a^2 - b^2)/b^2

    es = 1.0 - (semiMinor * semiMinor) / (semiMajor * semiMajor); //e^2
    ses = (pow(semiMajor, 2) - pow(semiMinor, 2)) / pow(semiMinor, 2);
    ba = semiMinor / semiMajor;
    ab = semiMajor / semiMinor;

    // ...

    double lon = DegreesToRadians(lonlat[0]);
    double lat = DegreesToRadians(lonlat[1]);
    double h = lonlat.count() < 3 ? 0 : std::isnan(lonlat[2]) ? 0 : lonlat[2];//TODO NAN
    double v = semiMajor / sqrt(1 - es * pow(sin(lat), 2));
    double x = (v + h) * cos(lat) * cos(lon);
    double y = (v + h) * cos(lat) * sin(lon);
    double z = ((1 - es) * v + h) * sin(lat);
    QVector <double> ret(3);
    ret[0]=x;
    ret[1]=y;
    ret[2]=z;
    return  ret;
}
QVector <double>  LKS94Projection::MTD10(QVector <double>&  pnt)
{
    QVector <double> ret(3);
    const double COS_67P5 = 0.38268343236508977;    // cosine of 67.5 degrees
    const double AD_C = 1.0026000;                  // Toms region 1 constant

    double es;                             // Eccentricity squared : (a^2 - b^2)/a^2
    double semiMajor = 6378137.0;		    // major axis
    double semiMinor = 6356752.3141403561;	// minor axis
    double ab;				// Semi_major / semi_minor
    double ba;				// Semi_minor / semi_major
    double ses;            // Second eccentricity squared : (a^2 - b^2)/b^2

    es = 1.0 - (semiMinor * semiMinor) / (semiMajor * semiMajor); //e^2
    ses = (pow(semiMajor, 2) - pow(semiMinor, 2)) / pow(semiMinor, 2);
    ba = semiMinor / semiMajor;
    ab = semiMajor / semiMinor;

    // ...

    bool AtPole = false; // is location in polar region
    double Z = pnt.count() < 3 ? 0 : std::isnan(pnt[2]) ? 0 : pnt[2];//TODO NaN

    double lon = 0;
    double lat = 0;
    double Height = 0;
    if(pnt[0] != 0.0)
    {
        lon = atan2(pnt[1], pnt[0]);
    }
    else
    {
        if(pnt[1] > 0)
        {
            lon = M_PI / 2;
        }
        else
            if(pnt[1] < 0)
            {
            lon = -M_PI * 0.5;
        }
        else
        {
            AtPole = true;
            lon = 0.0;
            if(Z > 0.0) // north pole
            {
                lat = M_PI * 0.5;
            }
            else
                if(Z < 0.0) // south pole
                {
                lat = -M_PI * 0.5;
            }
            else // center of earth
            {
                ret[0]=RadiansToDegrees(lon);
                ret[1]=RadiansToDegrees(M_PI * 0.5);
                ret[2]=-semiMinor;
                return ret;
            }
        }
    }
    double W2 = pnt[0] * pnt[0] + pnt[1] * pnt[1]; // Square of distance from Z axis
    double W = sqrt(W2); // distance from Z axis
    double T0 = Z * AD_C; // initial estimate of vertical component
    double S0 = sqrt(T0 * T0 + W2); // initial estimate of horizontal component
    double Sin_B0 = T0 / S0; // sin(B0), B0 is estimate of Bowring aux variable
    double Cos_B0 = W / S0; // cos(B0)
    double Sin3_B0 = pow(Sin_B0, 3);
    double T1 = Z + semiMinor * ses * Sin3_B0; // corrected estimate of vertical component
    double Sum = W - semiMajor * es * Cos_B0 * Cos_B0 * Cos_B0; // numerator of cos(phi1)
    double S1 = sqrt(T1 * T1 + Sum * Sum); // corrected estimate of horizontal component
    double Sin_p1 = T1 / S1; // sin(phi1), phi1 is estimated latitude
    double Cos_p1 = Sum / S1; // cos(phi1)
    double Rn = semiMajor / sqrt(1.0 - es * Sin_p1 * Sin_p1); // Earth radius at location
    if(Cos_p1 >= COS_67P5)
    {
        Height = W / Cos_p1 - Rn;
    }
    else
        if(Cos_p1 <= -COS_67P5)
        {
        Height = W / -Cos_p1 - Rn;
    }
    else
    {
        Height = Z / Sin_p1 + Rn * (es - 1.0);
    }

    if(!AtPole)
    {
        lat = atan(Sin_p1 / Cos_p1);
    }
    ret[0]=RadiansToDegrees(lon);
    ret[1]=RadiansToDegrees(lat);
    ret[2]=Height;
    return ret;
}
QVector <double> LKS94Projection::DTM00(QVector <double>& lonlat)
{
    double scaleFactor = 0.9998;	                // scale factor
    double centralMeridian = 0.41887902047863912;	// Center qlonglongitude (projection center) */
    double latOrigin = 0.0;	                // center latitude
    double falseNorthing = 0.0;	            // y offset in meters
    double falseEasting = 500000.0;	        // x offset in meters
    double semiMajor = 6378137.0;		        // major axis
    double semiMinor = 6356752.3141403561;		// minor axis
    double metersPerUnit = 1.0;

    double e0, e1, e2, e3;	// eccentricity constants
    double e, es, esp;		// eccentricity constants
    double ml0;		    // small value m

    es = 1.0 - pow(semiMinor / semiMajor, 2);
    e = sqrt(es);
    e0 = e0fn(es);
    e1 = e1fn(es);
    e2 = e2fn(es);
    e3 = e3fn(es);
    ml0 = semiMajor * mlfn(e0, e1, e2, e3, latOrigin);
    esp = es / (1.0 - es);

    // ...

    double lon = DegreesToRadians(lonlat[0]);
    double lat = DegreesToRadians(lonlat[1]);

    double delta_lon = 0.0;  // Delta qlonglongitude (Given qlonglongitude - center)
    double sin_phi, cos_phi; // sin and cos value
    double al, als;		  // temporary values
    double c, t, tq;	      // temporary values
    double con, n, ml;	      // cone constant, small m

    delta_lon = LKS94Projection::AdjustLongitude(lon - centralMeridian);
    LKS94Projection::SinCos(lat,  sin_phi,  cos_phi);

    al = cos_phi * delta_lon;
    als = pow(al, 2);
    c = pow(cos_phi, 2);
    tq = tan(lat);
    t = pow(tq, 2);
    con = 1.0 - es * pow(sin_phi, 2);
    n = semiMajor / sqrt(con);
    ml = semiMajor * mlfn(e0, e1, e2, e3, lat);

    double x = scaleFactor * n * al * (1.0 + als / 6.0 * (1.0 - t + c + als / 20.0 *
                                                          (5.0 - 18.0 * t + pow(t, 2) + 72.0 * c - 58.0 * esp))) + falseEasting;

    double y = scaleFactor * (ml - ml0 + n * tq * (als * (0.5 + als / 24.0 *
                                                          (5.0 - t + 9.0 * c + 4.0 * pow(c, 2) + als / 30.0 * (61.0 - 58.0 * t
                                                                                                               + pow(t, 2) + 600.0 * c - 330.0 * esp))))) + falseNorthing;

    if(lonlat.count() < 3)
    {
        QVector <double> ret(2);
        ret[0]= x / metersPerUnit;
        ret[1]= y / metersPerUnit;
        return ret;
    }
    else
    {
        QVector <double> ret(3);
        ret[0]= x / metersPerUnit;
        ret[1]= y / metersPerUnit;
        ret[2]=lonlat[2];
        return ret;
    }
}

QVector <double> LKS94Projection::DTM01(QVector <double>& lonlat)
{
    double es;                             // Eccentricity squared : (a^2 - b^2)/a^2
    double semiMajor = 6378137.0;		    // major axis
    double semiMinor = 6356752.3141403561;	// minor axis
    double ab;				                // Semi_major / semi_minor
    double ba;				                // Semi_minor / semi_major
    double ses;                            // Second eccentricity squared : (a^2 - b^2)/b^2

    es = 1.0 - (semiMinor * semiMinor) / (semiMajor * semiMajor);
    ses = (pow(semiMajor, 2) -pow(semiMinor, 2)) / pow(semiMinor, 2);
    ba = semiMinor / semiMajor;
    ab = semiMajor / semiMinor;

    // ...

    double lon = DegreesToRadians(lonlat[0]);
    double lat = DegreesToRadians(lonlat[1]);
    double h = lonlat.count() < 3 ? 0 : std::isnan(lonlat[2]) ? 0 : lonlat[2];//TODO NaN
    double v = semiMajor / sqrt(1 - es * pow(sin(lat), 2));
    double x = (v + h) * cos(lat) * cos(lon);
    double y = (v + h) * cos(lat) * sin(lon);
    double z = ((1 - es) * v + h) * sin(lat);
    QVector <double> ret(3);
    ret[0]=x;
    ret[1]=y;
    ret[2]=z;
    return ret;
}
QVector <double> LKS94Projection::MTD01(QVector <double>& pnt)
{
    const double COS_67P5 = 0.38268343236508977; // cosine of 67.5 degrees
    const double AD_C = 1.0026000;               // Toms region 1 constant

    double es;                             // Eccentricity squared : (a^2 - b^2)/a^2
    double semiMajor = 6378137.0;		    // major axis
    double semiMinor = 6356752.3142451793;	// minor axis
    double ab;		                        // Semi_major / semi_minor
    double ba;				                // Semi_minor / semi_major
    double ses;                            // Second eccentricity squared : (a^2 - b^2)/b^2

    es = 1.0 - (semiMinor * semiMinor) / (semiMajor * semiMajor);
    ses = (pow(semiMajor, 2) - pow(semiMinor, 2)) / pow(semiMinor, 2);
    ba = semiMinor / semiMajor;
    ab = semiMajor / semiMinor;

    // ...

    bool At_Pole = false; // is location in polar region
    double Z = pnt.count() < 3 ? 0 : std::isnan(pnt[2]) ? 0 : pnt[2];//TODO NaN

    double lon = 0;
    double lat = 0;
    double Height = 0;
    if(pnt[0] != 0.0)
    {
        lon = atan2(pnt[1], pnt[0]);
    }
    else
    {
        if(pnt[1] > 0)
        {
            lon = M_PI / 2;
        }
        else
            if(pnt[1] < 0)
            {
            lon = -M_PI * 0.5;
        }
        else
        {
            At_Pole = true;
            lon = 0.0;
            if(Z > 0.0) // north pole
            {
                lat = M_PI * 0.5;
            }
            else
                if(Z < 0.0) // south pole
                {
                lat = -M_PI * 0.5;
            }
            else // center of earth
            {
                QVector<double> ret(3);
                ret[0]=RadiansToDegrees(lon);
                ret[1]=RadiansToDegrees(M_PI * 0.5);
                ret[2]=-semiMinor;
                return ret;
            }
        }
    }

    double W2 = pnt[0] * pnt[0] + pnt[1] * pnt[1]; // Square of distance from Z axis
    double W = sqrt(W2);                      // distance from Z axis
    double T0 = Z * AD_C;                // initial estimate of vertical component
    double S0 = sqrt(T0 * T0 + W2); //initial estimate of horizontal component
    double Sin_B0 = T0 / S0;             // sin(B0), B0 is estimate of Bowring aux variable
    double Cos_B0 = W / S0;              // cos(B0)
    double Sin3_B0 = pow(Sin_B0, 3);
    double T1 = Z + semiMinor * ses * Sin3_B0; //corrected estimate of vertical component
    double Sum = W - semiMajor * es * Cos_B0 * Cos_B0 * Cos_B0; // numerator of cos(phi1)
    double S1 = sqrt(T1 * T1 + Sum * Sum); // corrected estimate of horizontal component
    double Sin_p1 = T1 / S1;  // sin(phi1), phi1 is estimated latitude
    double Cos_p1 = Sum / S1; // cos(phi1)
    double Rn = semiMajor / sqrt(1.0 - es * Sin_p1 * Sin_p1); // Earth radius at location

    if(Cos_p1 >= COS_67P5)
    {
        Height = W / Cos_p1 - Rn;
    }
    else
        if(Cos_p1 <= -COS_67P5)
        {
        Height = W / -Cos_p1 - Rn;
    }
    else
    {
        Height = Z / Sin_p1 + Rn * (es - 1.0);
    }

    if(!At_Pole)
    {
        lat = atan(Sin_p1 / Cos_p1);
    }
    QVector<double> ret(3);
    ret[0]=RadiansToDegrees(lon);
    ret[1]=RadiansToDegrees(lat);
    ret[2]=Height;
    return ret;
}
QVector <double> LKS94Projection::MTD11(QVector <double>& p)
{
    double scaleFactor = 0.9998;	                // scale factor
    double centralMeridian = 0.41887902047863912;	// Center qlonglongitude (projection center)
    double latOrigin = 0.0;	                   // center latitude
    double falseNorthing = 0.0;	        // y offset in meters
    double falseEasting = 500000.0;	    // x offset in meters
    double semiMajor = 6378137.0;		    // major axis
    double semiMinor = 6356752.3141403561;	// minor axis
    double metersPerUnit = 1.0;

    double e0, e1, e2, e3;	// eccentricity constants
    double e, es, esp;		// eccentricity constants
    double ml0;		    // small value m

    es =(semiMinor * semiMinor) / (semiMajor * semiMajor);
    es=1.0-es;
    e = sqrt(es);
    e0 = e0fn(es);
    e1 = e1fn(es);
    e2 = e2fn(es);
    e3 = e3fn(es);
    ml0 = semiMajor * mlfn(e0, e1, e2, e3, latOrigin);
    esp = es / (1.0 - es);

    // ...

    double con, phi;
    double delta_phi;
    qlonglong i;
    double sin_phi, cos_phi, tan_phi;
    double c, cs, t, ts, n, r, d, ds;
    qlonglong max_iter = 6;

    double x = p[0] * metersPerUnit - falseEasting;
    double y = p[1] * metersPerUnit - falseNorthing;

    con = (ml0 + y / scaleFactor) / semiMajor;
    phi = con;
    for(i = 0; ; i++)
    {
        delta_phi = ((con + e1 * sin(2.0 * phi) - e2 * sin(4.0 * phi) + e3 * sin(6.0 * phi)) / e0) - phi;
        phi += delta_phi;
        if(fabs(delta_phi) <= EPSLoN)
            break;

        if(i >= max_iter)
            throw  "Latitude failed to converge";
    }

    if(fabs(phi) < HALF_PI)
    {
        SinCos(phi,  sin_phi,  cos_phi);
        tan_phi = tan(phi);
        c = esp * pow(cos_phi, 2);
        cs = pow(c, 2);
        t = pow(tan_phi, 2);
        ts = pow(t, 2);
        con = 1.0 - es * pow(sin_phi, 2);
        n = semiMajor / sqrt(con);
        r = n * (1.0 - es) / con;
        d = x / (n * scaleFactor);
        ds = pow(d, 2);

        double lat = phi - (n * tan_phi * ds / r) * (0.5 - ds / 24.0 * (5.0 + 3.0 * t +
                                                                        10.0 * c - 4.0 * cs - 9.0 * esp - ds / 30.0 * (61.0 + 90.0 * t +
                                                                                                                       298.0 * c + 45.0 * ts - 252.0 * esp - 3.0 * cs)));

        double lon = AdjustLongitude(centralMeridian + (d * (1.0 - ds / 6.0 * (1.0 + 2.0 * t +
                                                                               c - ds / 20.0 * (5.0 - 2.0 * c + 28.0 * t - 3.0 * cs + 8.0 * esp +
                                                                                                24.0 * ts))) / cos_phi));

        if(p.count() < 3)
        {
            QVector<double> ret(2);
            ret[0]= RadiansToDegrees(lon);
            ret[1]= RadiansToDegrees(lat);
            return ret;
        }
        else
        {
            QVector<double> ret(3);
            ret[0]= RadiansToDegrees(lon);
            ret[1]= RadiansToDegrees(lat);
            ret[2]=p[2];
            return ret;
            //return new double[] { RadiansToDegrees(lon), RadiansToDegrees(lat), p[2] };
        }
    }
    else
    {
        if(p.count() < 3)
        {
            QVector<double> ret(2);
            ret[0]= RadiansToDegrees(HALF_PI * Sign(y));
            ret[1]= RadiansToDegrees(centralMeridian);
            return ret;
        }

        else
        {
            QVector<double> ret(3);
            ret[0]= RadiansToDegrees(HALF_PI * Sign(y));
            ret[1]= RadiansToDegrees(centralMeridian);
            ret[2]=p[2];
            return ret;
        }

    }
}

double LKS94Projection::Clip(double const& n, double const& minValue, double const& maxValue)
{
    return qMin(qMax(n, minValue), maxValue);
}
double LKS94Projection::GetTileMatrixResolution(int const& zoom)
{
    double ret = 0;

    switch(zoom)
    {
    case 0:
        {
            ret = 1587.50317500635;
        }
        break;

    case 1:
        {
            ret = 793.751587503175;
        }
        break;

    case 2:
        {
            ret = 529.167725002117;
        }
        break;

    case 3:
        {
            ret = 264.583862501058;
        }
        break;

    case 4:
        {
            ret = 132.291931250529;
        }
        break;

    case 5:
        {
            ret = 52.9167725002117;
        }
        break;

    case 6:
        {
            ret = 26.4583862501058;
        }
        break;

    case 7:
        {
            ret = 13.2291931250529;
        }
        break;

    case 8:
        {
            ret = 6.61459656252646;
        }
        break;

    case 9:
        {
            ret = 2.64583862501058;
        }
        break;

    case 10:
        {
            ret = 1.32291931250529;
        }
        break;

    case 11:
        {
            ret = 0.529167725002117;
        }
        break;

    }

    return ret;
}
double LKS94Projection::GetGroundResolution(int const& zoom, double const& latitude)
{
    Q_UNUSED(zoom);
    Q_UNUSED(latitude);
    return GetTileMatrixResolution(zoom);
}
Size LKS94Projection::GetTileMatrixMinXY(int const& zoom)
{
    Size ret;

    switch(zoom)
    {

    case 0:
        {
            ret =  Size(12, 8);
        }
        break;

    case 1:
        {
            ret =  Size(24, 17);
        }
        break;

    case 2:
        {
            ret =  Size(37, 25);
        }
        break;

    case 3:
        {
            ret =  Size(74, 51);
        }
        break;

    case 4:
        {
            ret =  Size(149, 103);
        }
        break;

    case 5:
        {
            ret =  Size(374, 259);
        }
        break;

    case 6:
        {
            ret =  Size(749, 519);
        }
        break;

    case 7:
        {
            ret =  Size(1594, 1100);
        }
        break;

    case 8:
        {
            ret =  Size(3188, 2201);
        }
        break;

    case 9:
        {
            ret =  Size(7971, 5502);
        }
        break;

    case 10:
        {
            ret =  Size(15943, 11005);
        }
        break;

    case 11:
        {
            ret =  Size(39858, 27514);
        }
        break;
    }

    return ret;
}

Size LKS94Projection::GetTileMatrixMaxXY(int const& zoom)
{
    Size ret;

    switch(zoom)
    {
    case 0:
        {
            ret =  Size(14, 10);
        }
        break;

    case 1:
        {
            ret =  Size(30, 20);
        }
        break;

    case 2:
        {
            ret =  Size(45, 31);
        }
        break;

    case 3:
        {
            ret =  Size(90, 62);
        }
        break;

    case 4:
        {
            ret =  Size(181, 125);
        }
        break;

    case 5:
        {
            ret =  Size(454, 311);
        }
        break;

    case 6:
        {
            ret =  Size(903, 623);
        }
        break;

    case 7:
        {
            ret =  Size(1718, 1193);
        }
        break;

    case 8:
        {
            ret =  Size(3437, 2386);
        }
        break;

    case 9:
        {
            ret =  Size(8594, 5966);
        }
        break;

    case 10:
        {
            ret =  Size(17189, 11932);
        }
        break;

    case 11:
        {
            ret =  Size(42972, 29831);
        }
        break;
    }

    return ret;
}

}
