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
#include "FactSystem.h"

class APMGeoFenceManager : public GeoFenceManager
{
    Q_OBJECT
    
public:
    APMGeoFenceManager(Vehicle* vehicle);
    ~APMGeoFenceManager();

    // Overrides from GeoFenceManager
    bool            inProgress              (void) const final;
    void            loadFromVehicle         (void) final;
    void            sendToVehicle           (const QGeoCoordinate& breachReturn, const QList<QGeoCoordinate>& polygon) final;
    bool            fenceSupported          (void) const final { return _fenceSupported; }
    bool            circleSupported         (void) const final;
    bool            polygonSupported        (void) const final;
    bool            breachReturnSupported   (void) const final { return _breachReturnSupported; }
    float           circleRadius            (void) const final;
    QVariantList    params                  (void) const final { return _params; }
    QStringList     paramLabels             (void) const final { return _paramLabels; }
    QString         editorQml               (void) const final;

private slots:
    void _mavlinkMessageReceived(const mavlink_message_t& message);
    void _updateSupportedFlags(void);
    void _circleRadiusRawValueChanged(QVariant value);
    void _parametersReady(void);
    
private:
    void _requestFencePoint(uint8_t pointIndex);
    void _sendFencePoint(uint8_t pointIndex);
    bool _geoFenceSupported(void);

private:
    bool _fenceSupported;
    bool _breachReturnSupported;
    bool _circleSupported;
    bool _polygonSupported;
    bool _firstParamLoadComplete;

    QVariantList    _params;
    QStringList     _paramLabels;

    Fact* _circleRadiusFact;

    bool _readTransactionInProgress;
    bool _writeTransactionInProgress;

    uint8_t _cReadFencePoints;
    uint8_t _cWriteFencePoints;
    uint8_t _currentFencePoint;

    Fact* _fenceTypeFact;

    static const char* _fenceTotalParam;
    static const char* _fenceActionParam;
    static const char* _fenceEnableParam;
};

#endif
