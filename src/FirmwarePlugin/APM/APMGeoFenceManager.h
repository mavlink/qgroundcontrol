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
    bool            inProgress          (void) const final;
    void            loadFromVehicle     (void) final;
    void            sendToVehicle       (const QGeoCoordinate& breachReturn, const QList<QGeoCoordinate>& polygon) final;
    bool            circleEnabled       (void) const final { return _circleEnabled; }
    bool            polygonEnabled      (void) const final { return _polygonEnabled; }
    bool            breachReturnEnabled (void) const final { return _breachReturnEnabled; }
    float           circleRadius        (void) const final;
    QVariantList    params              (void) const final { return _params; }
    QStringList     paramLabels         (void) const final { return _paramLabels; }
    QString         editorQml           (void) const final;

private slots:
    void _mavlinkMessageReceived(const mavlink_message_t& message);
    void _updateEnabledFlags(void);
    void _circleRadiusRawValueChanged(QVariant value);
    void _parametersReady(void);
    
private:
    void _requestFencePoint(uint8_t pointIndex);
    void _sendFencePoint(uint8_t pointIndex);

private:
    bool _fenceSupported;
    bool _breachReturnEnabled;
    bool _circleEnabled;
    bool _polygonEnabled;
    bool _firstParamLoadComplete;

    QVariantList    _params;
    QStringList     _paramLabels;

    bool _readTransactionInProgress;
    bool _writeTransactionInProgress;

    uint8_t _cReadFencePoints;
    uint8_t _cWriteFencePoints;
    uint8_t _currentFencePoint;

    Fact* _fenceTypeFact;
    Fact* _fenceEnableFact;
    Fact* _circleRadiusFact;

    static const char* _fenceTotalParam;
    static const char* _fenceActionParam;
    static const char* _fenceEnableParam;
};

#endif
