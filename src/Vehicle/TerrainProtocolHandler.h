/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtPositioning/QGeoCoordinate>

#include "MAVLinkLib.h"

class QTimer;
class TerrainFactGroup;
class Vehicle;

Q_DECLARE_LOGGING_CATEGORY(TerrainProtocolHandlerLog)

class TerrainProtocolHandler : public QObject
{
    Q_OBJECT

public:
    explicit TerrainProtocolHandler(Vehicle *vehicle, TerrainFactGroup *terrainFactGroup, QObject *parent = nullptr);
    ~TerrainProtocolHandler();

    /// @return true: Allow vehicle to continue processing, false: Vehicle should not process message
    bool mavlinkMessageReceived(const mavlink_message_t &message);

private slots:
    void _sendNextTerrainData();

private:
    void _handleTerrainRequest(const mavlink_message_t &message);
    void _handleTerrainReport(const mavlink_message_t &message);
    void _sendTerrainData(const QGeoCoordinate &swCorner, uint8_t gridBit);

    Vehicle *_vehicle = nullptr;
    TerrainFactGroup *_terrainFactGroup = nullptr;
    QTimer *_terrainDataSendTimer = nullptr;
    bool _terrainRequestActive = false;
    mavlink_terrain_request_t _currentTerrainRequest;
};
