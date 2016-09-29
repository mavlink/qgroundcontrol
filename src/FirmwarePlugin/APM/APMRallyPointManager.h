/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#ifndef APMRallyPointManager_H
#define APMRallyPointManager_H

#include "RallyPointManager.h"
#include "QGCMAVLink.h"

class APMRallyPointManager : public RallyPointManager
{
    Q_OBJECT
    
public:
    APMRallyPointManager(Vehicle* vehicle);
    ~APMRallyPointManager();

    // Overrides from RallyPointManager
    bool inProgress             (void) const final;
    void loadFromVehicle        (void) final;
    void sendToVehicle          (const QList<QGeoCoordinate>& rgPoints) final;
    bool rallyPointsSupported   (void) const final { return true; }

    QString editorQml(void) const final { return QStringLiteral("qrc:/FirmwarePlugin/APM/APMRallyPointEditor.qml"); }

private slots:
    void _mavlinkMessageReceived(const mavlink_message_t& message);
    
private:
    void _requestRallyPoint(uint8_t pointIndex);
    void _sendRallyPoint(uint8_t pointIndex);

private:
    bool _readTransactionInProgress;
    bool _writeTransactionInProgress;

    uint8_t _cReadRallyPoints;
    uint8_t _currentRallyPoint;

    static const char* _rallyTotalParam;
};

#endif
