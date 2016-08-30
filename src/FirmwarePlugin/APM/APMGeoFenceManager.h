/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#ifndef APMGeoFenceManager_H
#define APMGeoFenceManager_H

#include "GeoFenceManager.h"
#include "QGCMAVLink.h"

class APMGeoFenceManager : public GeoFenceManager
{
    Q_OBJECT
    
public:
    APMGeoFenceManager(Vehicle* vehicle);
    ~APMGeoFenceManager();

    // Overrides from GeoFenceManager
    bool inProgress     (void) const final;
    void requestGeoFence(void) final;
    void setGeoFence    (const GeoFence_t& geoFence) final;
    
private slots:
    void _mavlinkMessageReceived(const mavlink_message_t& message);
    
private:
    void _requestFencePoint(uint8_t pointIndex);
    void _sendFencePoint(uint8_t pointIndex);
    bool _geoFenceSupported(void);

private:
    bool        _readTransactionInProgress;
    bool        _writeTransactionInProgress;

    uint8_t     _cReadFencePoints;
    uint8_t     _cWriteFencePoints;
    uint8_t     _currentFencePoint;

    static const char* _fenceTotalParam;
    static const char* _fenceActionParam;
    static const char* _fenceEnableParam;
};

#endif
