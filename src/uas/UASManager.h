/*=====================================================================

QGroundControl Open Source Ground Control Station

(c) 2009 - 2011 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

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

/**
 * @file
 *   @brief Definition of class UASManager
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *
 */

#ifndef _UASMANAGER_H_
#define _UASMANAGER_H_

#include "UASManagerInterface.h"
#include "UASInterface.h"

#include <QList>
#include <QMutex>

#include <Eigen/Eigen>

#include "QGCGeo.h"
#include "QGCSingleton.h"

/**
 * @brief Central manager for all connected aerial vehicles
 *
 * This class keeps a list of all connected / configured UASs. It also stores which
 * UAS is currently select with respect to user input or manual controls.
 **/
class UASManager : public UASManagerInterface
{
    Q_OBJECT
    
    DECLARE_QGC_SINGLETON(UASManager, UASManagerInterface)

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

    void getLocalNEDSafetyLimits(double* x1, double* y1, double* z1, double* x2, double* y2, double* z2)
    {
        *x1 = nedSafetyLimitPosition1.x();
        *y1 = nedSafetyLimitPosition1.y();
        *z1 = nedSafetyLimitPosition1.z();

        *x2 = nedSafetyLimitPosition2.x();
        *y2 = nedSafetyLimitPosition2.y();
        *z2 = nedSafetyLimitPosition2.z();
    }

    /** @brief Check if a position is in the local NED safety limits */
    bool isInLocalNEDSafetyLimits(double x, double y, double z)
    {
        if (x < nedSafetyLimitPosition1.x() &&
            y > nedSafetyLimitPosition1.y() &&
            z < nedSafetyLimitPosition1.z() &&
            x > nedSafetyLimitPosition2.x() &&
            y < nedSafetyLimitPosition2.y() &&
            z > nedSafetyLimitPosition2.z())
        {
            // Within limits
            return true;
        }
        else
        {
            // Not within limits
            return false;
        }
    }

//    void wgs84ToNed(const double& lat, const double& lon, const double& alt, double* north, double* east, double* down);


public slots:
    /** @brief Set the current home position, but do not change it on the UAVs */
    bool setHomePosition(double lat, double lon, double alt);

    /** @brief Set the current home position on all UAVs*/
    bool setHomePositionAndNotify(double lat, double lon, double alt);

    /** @brief Set the safety limits in local position frame */
    void setLocalNEDSafetyBorders(double x1, double y1, double z1, double x2, double y2, double z2);

    /** @brief Update home position based on the position from one of the UAVs */
    void uavChangedHomePosition(int uav, double lat, double lon, double alt);

    /** @brief Load settings */
    void loadSettings();
    /** @brief Store settings */
    void storeSettings();


protected:
    UASWaypointManager *offlineUASWaypointManager;
    double homeLat;
    double homeLon;
    double homeAlt;
    int homeFrame;
    Eigen::Quaterniond ecef_ref_orientation_;
    Eigen::Vector3d ecef_ref_point_;
    Eigen::Vector3d nedSafetyLimitPosition1;
    Eigen::Vector3d nedSafetyLimitPosition2;

    void initReference(const double & latitude, const double & longitude, const double & altitude);
    
private:
    /// @brief All access to UASManager singleton is through UASManager::instance
    UASManager(QObject* parent = NULL);
    ~UASManager();

public:
    /* Need to align struct pointer to prevent a memory assertion:
     * See http://eigen.tuxfamily.org/dox-devel/TopicUnalignedArrayAssert.html
     * for details
     */
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW
};

#endif // _UASMANAGER_H_
