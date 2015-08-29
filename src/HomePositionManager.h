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

#ifndef _UASMANAGER_H_
#define _UASMANAGER_H_

#include "UASInterface.h"

#include <QList>
#include <QMutex>

#include <Eigen/Eigen>

#include "QGCGeo.h"
#include "QGCSingleton.h"

/// Manages an offline home position as well as performance coordinate transformations
/// around a home position.
class HomePositionManager : public QObject
{
    Q_OBJECT
    
    DECLARE_QGC_SINGLETON(HomePositionManager, HomePositionManager)

public:
    /** @brief Get home position latitude */
    double getHomeLatitude() const {
        return homeLat;
    }
    /** @brief Get home position longitude */
    double getHomeLongitude() const {
        return homeLon;
    }
    /** @brief Get home position altitude */
    double getHomeAltitude() const {
        return homeAlt;
    }

    /** @brief Get the home position coordinate frame */
    int getHomeFrame() const
    {
        return homeFrame;
    }

    /** @brief Convert WGS84 coordinates to earth centric frame */
    Eigen::Vector3d wgs84ToEcef(const double & latitude, const double & longitude, const double & altitude);
    /** @brief Convert earth centric frame to EAST-NORTH-UP frame (x-y-z directions */
    Eigen::Vector3d ecefToEnu(const Eigen::Vector3d & ecef);
    /** @brief Convert WGS84 lat/lon coordinates to carthesian coordinates with home position as origin */
    void wgs84ToEnu(const double& lat, const double& lon, const double& alt, double* east, double* north, double* up);
    /** @brief Convert x,y,z coordinates to lat / lon / alt coordinates in east-north-up frame */
    void enuToWgs84(const double& x, const double& y, const double& z, double* lat, double* lon, double* alt);
    /** @brief Convert x,y,z coordinates to lat / lon / alt coordinates in north-east-down frame */
    void nedToWgs84(const double& x, const double& y, const double& z, double* lat, double* lon, double* alt);

public slots:
    /** @brief Set the current home position, but do not change it on the UAVs */
    bool setHomePosition(double lat, double lon, double alt);

    /** @brief Set the current home position on all UAVs*/
    bool setHomePositionAndNotify(double lat, double lon, double alt);


    /** @brief Load settings */
    void loadSettings();
    /** @brief Store settings */
    void storeSettings();

signals:
    /** @brief Current home position changed */
    void homePositionChanged(double lat, double lon, double alt);
    
protected:
    double homeLat;
    double homeLon;
    double homeAlt;
    int homeFrame;
    Eigen::Quaterniond ecef_ref_orientation_;
    Eigen::Vector3d ecef_ref_point_;

    void initReference(const double & latitude, const double & longitude, const double & altitude);
    
private:
    /// @brief All access to HomePositionManager singleton is through HomePositionManager::instance
    HomePositionManager(QObject* parent = NULL);
    ~HomePositionManager();

public:
    /* Need to align struct pointer to prevent a memory assertion:
     * See http://eigen.tuxfamily.org/dox-devel/TopicUnalignedArrayAssert.html
     * for details
     */
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW
};

#endif // _UASMANAGER_H_
