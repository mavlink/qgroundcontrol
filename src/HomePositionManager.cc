/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009 - 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
 This file is part of the QGROUNDCONTROL project
 
 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.
 
 ======================================================================*/

#include <QList>
#include <QApplication>
#include <QTimer>
#include <QSettings>

#include "UAS.h"
#include "UASInterface.h"
#include "HomePositionManager.h"
#include "QGC.h"
#include "QGCMessageBox.h"
#include "QGCApplication.h"
#include "MultiVehicleManager.h"

#define PI 3.1415926535897932384626433832795
#define MEAN_EARTH_DIAMETER	12756274.0
#define UMR	0.017453292519943295769236907684886

IMPLEMENT_QGC_SINGLETON(HomePositionManager, HomePositionManager)

HomePositionManager::HomePositionManager(QObject* parent) :
    QObject(parent),
    homeLat(47.3769),
    homeLon(8.549444),
    homeAlt(470.0),
    homeFrame(MAV_FRAME_GLOBAL)
{
    loadSettings();
}

HomePositionManager::~HomePositionManager()
{
    storeSettings();
}

void HomePositionManager::storeSettings()
{
    QSettings settings;
    settings.beginGroup("QGC_UASMANAGER");
    settings.setValue("HOMELAT", homeLat);
    settings.setValue("HOMELON", homeLon);
    settings.setValue("HOMEALT", homeAlt);
    settings.endGroup();
}

void HomePositionManager::loadSettings()
{
    QSettings settings;
    settings.beginGroup("QGC_UASMANAGER");
    bool changed =  setHomePosition(settings.value("HOMELAT", homeLat).toDouble(),
                                    settings.value("HOMELON", homeLon).toDouble(),
                                    settings.value("HOMEALT", homeAlt).toDouble());

    // Make sure to fire the change - this will
    // make sure widgets get the signal once
    if (!changed)
    {
        emit homePositionChanged(homeLat, homeLon, homeAlt);
    }

    settings.endGroup();
}

bool HomePositionManager::setHomePosition(double lat, double lon, double alt)
{
    // Checking for NaN and infitiny
    // and checking for borders
    bool changed = false;
    if (!isnan(lat) && !isnan(lon) && !isnan(alt)
        && !isinf(lat) && !isinf(lon) && !isinf(alt)
        && lat <= 90.0 && lat >= -90.0 && lon <= 180.0 && lon >= -180.0)
        {

        if (fabs(homeLat - lat) > 1e-7) changed = true;
        if (fabs(homeLon - lon) > 1e-7) changed = true;
        if (fabs(homeAlt - alt) > 0.5f) changed = true;

        // Initialize conversion reference in any case
        initReference(lat, lon, alt);

        if (changed)
        {
            homeLat = lat;
            homeLon = lon;
            homeAlt = alt;

            emit homePositionChanged(homeLat, homeLon, homeAlt);
        }
    }
    return changed;
}

bool HomePositionManager::setHomePositionAndNotify(double lat, double lon, double alt)
{
    // Checking for NaN and infitiny
    // and checking for borders
    bool changed = setHomePosition(lat, lon, alt);

    if (changed) {
        MultiVehicleManager::instance()->setHomePositionForAllVehicles(homeLat, homeLon, homeAlt);
    }

	return changed;
}

void HomePositionManager::initReference(const double & latitude, const double & longitude, const double & altitude)
{
    Eigen::Matrix3d R;
    double s_long, s_lat, c_long, c_lat;
    sincos(latitude * DEG2RAD, &s_lat, &c_lat);
    sincos(longitude * DEG2RAD, &s_long, &c_long);

    R(0, 0) = -s_long;
    R(0, 1) = c_long;
    R(0, 2) = 0;

    R(1, 0) = -s_lat * c_long;
    R(1, 1) = -s_lat * s_long;
    R(1, 2) = c_lat;

    R(2, 0) = c_lat * c_long;
    R(2, 1) = c_lat * s_long;
    R(2, 2) = s_lat;

    ecef_ref_orientation_ = Eigen::Quaterniond(R);

    ecef_ref_point_ = wgs84ToEcef(latitude, longitude, altitude);
}

Eigen::Vector3d HomePositionManager::wgs84ToEcef(const double & latitude, const double & longitude, const double & altitude)
{
    const double a = 6378137.0; // semi-major axis
    const double e_sq = 6.69437999014e-3; // first eccentricity squared

    double s_long, s_lat, c_long, c_lat;
    sincos(latitude * DEG2RAD, &s_lat, &c_lat);
    sincos(longitude * DEG2RAD, &s_long, &c_long);

    const double N = a / sqrt(1 - e_sq * s_lat * s_lat);

    Eigen::Vector3d ecef;

    ecef[0] = (N + altitude) * c_lat * c_long;
    ecef[1] = (N + altitude) * c_lat * s_long;
    ecef[2] = (N * (1 - e_sq) + altitude) * s_lat;

    return ecef;
}

Eigen::Vector3d HomePositionManager::ecefToEnu(const Eigen::Vector3d & ecef)
{
    return ecef_ref_orientation_ * (ecef - ecef_ref_point_);
}

void HomePositionManager::wgs84ToEnu(const double& lat, const double& lon, const double& alt, double* east, double* north, double* up)
{
    Eigen::Vector3d ecef = wgs84ToEcef(lat, lon, alt);
    Eigen::Vector3d enu = ecefToEnu(ecef);
    *east = enu.x();
    *north = enu.y();
    *up = enu.z();
}

void HomePositionManager::enuToWgs84(const double& x, const double& y, const double& z, double* lat, double* lon, double* alt)
{
    *lat=homeLat+y/MEAN_EARTH_DIAMETER*360./PI;
    *lon=homeLon+x/MEAN_EARTH_DIAMETER*360./PI/cos(homeLat*UMR);
    *alt=homeAlt+z;
}

void HomePositionManager::nedToWgs84(const double& x, const double& y, const double& z, double* lat, double* lon, double* alt)
{
    *lat=homeLat+x/MEAN_EARTH_DIAMETER*360./PI;
    *lon=homeLon+y/MEAN_EARTH_DIAMETER*360./PI/cos(homeLat*UMR);
    *alt=homeAlt-z;
}

