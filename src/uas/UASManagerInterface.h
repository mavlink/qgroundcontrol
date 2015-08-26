/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009 - 2014 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
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

#ifndef _UASMANAGERINTERFACE_H_
#define _UASMANAGERINTERFACE_H_

#include <QList>
#include <QMutex>

#include <Eigen/Eigen>

#include "UASInterface.h"
#include "QGCGeo.h"
#include "QGCSingleton.h"

/**
 * @brief Central manager for all connected aerial vehicles
 *
 * This class keeps a list of all connected / configured UASs. It also stores which
 * UAS is currently select with respect to user input or manual controls.
 *
 * This is the abstract base class for UASManager. Although there is normally only
 * a single UASManger we still use a abstract base interface since this allows us
 * to create mock versions of the UASManager for testing purposes.
 *
 * See UASManager.h for method documentation
 **/
class UASManagerInterface : public QGCSingleton
{
    Q_OBJECT
    
public:
    virtual UASInterface* getActiveUAS() = 0;
    virtual UASWaypointManager *getActiveUASWaypointManager() = 0;
    virtual UASInterface* silentGetActiveUAS() = 0;
    virtual UASInterface* getUASForId(int id) = 0;
    virtual QList<UASInterface*> getUASList() = 0;
    virtual double getHomeLatitude() const  = 0;
    virtual double getHomeLongitude() const  = 0;
    virtual double getHomeAltitude() const = 0;
    virtual int getHomeFrame() const = 0;
    virtual Eigen::Vector3d wgs84ToEcef(const double & latitude, const double & longitude, const double & altitude) = 0;
    virtual Eigen::Vector3d ecefToEnu(const Eigen::Vector3d & ecef) = 0;
    virtual void wgs84ToEnu(const double& lat, const double& lon, const double& alt, double* east, double* north, double* up) = 0;
    virtual void enuToWgs84(const double& x, const double& y, const double& z, double* lat, double* lon, double* alt) = 0;
    virtual void nedToWgs84(const double& x, const double& y, const double& z, double* lat, double* lon, double* alt) = 0;
    virtual void getLocalNEDSafetyLimits(double* x1, double* y1, double* z1, double* x2, double* y2, double* z2) = 0;
    virtual bool isInLocalNEDSafetyLimits(double x, double y, double z) = 0;
    
public slots:
    virtual void addUAS(UASInterface* UAS) = 0;
    virtual void removeUAS(UASInterface* uas) = 0;
    virtual void setActiveUAS(UASInterface* UAS) = 0;
    virtual bool launchActiveUAS() = 0;
    virtual bool haltActiveUAS() = 0;
    virtual bool continueActiveUAS() = 0;
    virtual bool returnActiveUAS() = 0;
    virtual bool stopActiveUAS() = 0;
    virtual bool killActiveUAS() = 0;
    virtual bool shutdownActiveUAS() = 0;
    virtual bool setHomePosition(double lat, double lon, double alt) = 0;
    virtual bool setHomePositionAndNotify(double lat, double lon, double alt) = 0;
    virtual void setLocalNEDSafetyBorders(double x1, double y1, double z1, double x2, double y2, double z2) = 0;
    virtual void uavChangedHomePosition(int uav, double lat, double lon, double alt) = 0;
    virtual void loadSettings() = 0;
    virtual void storeSettings() = 0;
    virtual void _shutdown(void) = 0;
    
signals:
    /** A new system was created */
    void UASCreated(UASInterface* UAS);
    /** A system was deleted */
    void UASDeleted(UASInterface* UAS);
    /** A system was deleted */
    void UASDeleted(int systemId);
    /** @brief The UAS currently under main operator control changed */
    void activeUASSet(UASInterface* UAS);
    /** @brief The UAS currently under main operator control changed */
    void activeUASSetListIndex(int listIndex);
    /** @brief The UAS currently under main operator control changed */
    void activeUASStatusChanged(UASInterface* UAS, bool active);
    /** @brief The UAS currently under main operator control changed */
    void activeUASStatusChanged(int systemId, bool active);
    /** @brief Current home position changed */
    void homePositionChanged(double lat, double lon, double alt);
    
protected:
    UASManagerInterface(QObject* parent = NULL) :
        QGCSingleton(parent) { }
};

#endif // _UASMANAGERINTERFACE_H_
