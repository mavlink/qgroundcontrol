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

#ifndef MOCKUASMANAGER_H
#define MOCKUASMANAGER_H

#include "UASManagerInterface.h"
#include "MockUAS.h"

/// @file
///     @brief This is a mock implementation of a UASManager used for writing Unit Tests. In order
///         to use it you must call UASManager::setMockUASManager which will then cause all further
///         calls to UASManager::instance to return the mock UASManager instead of the normal one.
///
///     @author Don Gagne <don@thegagnes.com>

class MockUASManager : public UASManagerInterface
{
    Q_OBJECT
    
signals:
    // The following signals from UASManager interface are supported:
    //      void activeUASSet(UASInterface* UAS);
    //      void activeUASSet(int systemId);
    //      void activeUASStatusChanged(UASInterface* UAS, bool active);
    //      void activeUASStatusChanged(int systemId, bool active);
    
public:
    // Implemented UASManagerInterface overrides
    virtual UASInterface* getActiveUAS(void);
    
public:
    // MockUASManager methods
    MockUASManager(void);
    
    // Does not support singleton deletion
    virtual void deleteInstance(void) { Q_ASSERT(false); }
    
    /// Sets the currently active mock UAS
    /// @param mockUAS new mock uas, NULL for no active UAS
    void setMockActiveUAS(MockUAS* mockUAS);

public:
    // Unimplemented UASManagerInterface overrides
    virtual UASWaypointManager *getActiveUASWaypointManager() { Q_ASSERT(false); return NULL; }
    virtual UASInterface* silentGetActiveUAS() { Q_ASSERT(false); return NULL; }
    virtual UASInterface* getUASForId(int id) { Q_ASSERT(false); Q_UNUSED(id); return NULL; }
    virtual QList<UASInterface*> getUASList() { Q_ASSERT(false); return _bogusQListUASInterface; }
    virtual double getHomeLatitude() const  { Q_ASSERT(false); return std::numeric_limits<double>::quiet_NaN(); }
    virtual double getHomeLongitude() const  { Q_ASSERT(false); return std::numeric_limits<double>::quiet_NaN(); }
    virtual double getHomeAltitude() const { Q_ASSERT(false); return std::numeric_limits<double>::quiet_NaN(); }
    virtual int getHomeFrame() const { Q_ASSERT(false); return 0; }
    virtual Eigen::Vector3d wgs84ToEcef(const double & latitude, const double & longitude, const double & altitude)
        { Q_ASSERT(false); Q_UNUSED(latitude); Q_UNUSED(longitude); Q_UNUSED(altitude); return _bogusEigenVector3d; }
    virtual Eigen::Vector3d ecefToEnu(const Eigen::Vector3d & ecef)
        { Q_ASSERT(false); Q_UNUSED(ecef); return _bogusEigenVector3d; }
    virtual void wgs84ToEnu(const double& lat, const double& lon, const double& alt, double* east, double* north, double* up)
        { Q_ASSERT(false); Q_UNUSED(lat); Q_UNUSED(lon); Q_UNUSED(alt); Q_UNUSED(east); Q_UNUSED(north); Q_UNUSED(up); }
    virtual void enuToWgs84(const double& x, const double& y, const double& z, double* lat, double* lon, double* alt)
        { Q_ASSERT(false); Q_UNUSED(x); Q_UNUSED(y); Q_UNUSED(z); Q_UNUSED(lat); Q_UNUSED(lon); Q_UNUSED(alt); }
    virtual void nedToWgs84(const double& x, const double& y, const double& z, double* lat, double* lon, double* alt)
        { Q_ASSERT(false); Q_UNUSED(x); Q_UNUSED(y); Q_UNUSED(z); Q_UNUSED(lat); Q_UNUSED(lon); Q_UNUSED(alt); }
    virtual void getLocalNEDSafetyLimits(double* x1, double* y1, double* z1, double* x2, double* y2, double* z2)
        { Q_ASSERT(false); Q_UNUSED(x1); Q_UNUSED(y1); Q_UNUSED(z1); Q_UNUSED(x2); Q_UNUSED(y2); Q_UNUSED(z2); }
    virtual bool isInLocalNEDSafetyLimits(double x, double y, double z)
        { Q_ASSERT(false); Q_UNUSED(x); Q_UNUSED(y); Q_UNUSED(z); return false; }
    
public slots:
    // Unimplemented UASManagerInterface overrides
    virtual void setActiveUAS(UASInterface* UAS) { Q_ASSERT(false); Q_UNUSED(UAS); }
    virtual void addUAS(UASInterface* UAS) { Q_ASSERT(false); Q_UNUSED(UAS); }
    virtual void removeUAS(UASInterface* uas) { Q_ASSERT(false); Q_UNUSED(uas);}
    virtual bool launchActiveUAS() { Q_ASSERT(false); return false; }
    virtual bool haltActiveUAS() { Q_ASSERT(false); return false; }
    virtual bool continueActiveUAS() { Q_ASSERT(false); return false; }
    virtual bool returnActiveUAS() { Q_ASSERT(false); return false; }
    virtual bool stopActiveUAS() { Q_ASSERT(false); return false; }
    virtual bool killActiveUAS() { Q_ASSERT(false); return false; }
    virtual bool shutdownActiveUAS() { Q_ASSERT(false); return false; }
    virtual bool setHomePosition(double lat, double lon, double alt)
        { Q_ASSERT(false); Q_UNUSED(lat); Q_UNUSED(lon); Q_UNUSED(alt); return false; }
    virtual bool setHomePositionAndNotify(double lat, double lon, double alt)
        { Q_ASSERT(false); Q_UNUSED(lat); Q_UNUSED(lon); Q_UNUSED(alt); return false; }
    virtual void setLocalNEDSafetyBorders(double x1, double y1, double z1, double x2, double y2, double z2)
        { Q_ASSERT(false); Q_UNUSED(x1); Q_UNUSED(y1); Q_UNUSED(z1); Q_UNUSED(x2); Q_UNUSED(y2); Q_UNUSED(z2); }
    virtual void uavChangedHomePosition(int uav, double lat, double lon, double alt)
        { Q_ASSERT(false); Q_UNUSED(uav); Q_UNUSED(lat); Q_UNUSED(lon); Q_UNUSED(alt); }
    virtual void loadSettings() { Q_ASSERT(false); }
    virtual void storeSettings() { Q_ASSERT(false); }
    
private:
    MockUAS*                _mockUAS;
    
    // Bogus variables used for return types of NYI methods
    QList<UASInterface*>    _bogusQListUASInterface;
    Eigen::Vector3d         _bogusEigenVector3d;
    
public:
    /* Need to align struct pointer to prevent a memory assertion:
     * See http://eigen.tuxfamily.org/dox-devel/TopicUnalignedArrayAssert.html
     * for details
     */
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW
};

#endif