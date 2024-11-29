/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "TerrainProtocolHandler.h"
#include "TerrainQuery.h"
#include "Vehicle.h"
#include "MAVLinkProtocol.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QTimer>

QGC_LOGGING_CATEGORY(TerrainProtocolHandlerLog, "qgc.vehicle.terrainprotocolhandler")

TerrainProtocolHandler::TerrainProtocolHandler(Vehicle *vehicle, TerrainFactGroup *terrainFactGroup, QObject *parent)
    : QObject(parent)
    , _vehicle(vehicle)
    , _terrainFactGroup(terrainFactGroup)
    , _terrainDataSendTimer(new QTimer(this))
{
    // qCDebug(TerrainProtocolHandlerLog) << Q_FUNC_INFO << this;

    _terrainDataSendTimer->setSingleShot(false);
    _terrainDataSendTimer->setInterval(1000.0/12.0);
    (void) connect(_terrainDataSendTimer, &QTimer::timeout, this, &TerrainProtocolHandler::_sendNextTerrainData);
}

TerrainProtocolHandler::~TerrainProtocolHandler()
{
    // qCDebug(TerrainProtocolHandlerLog) << Q_FUNC_INFO << this;
}

bool TerrainProtocolHandler::mavlinkMessageReceived(const mavlink_message_t &message)
{
    switch (message.msgid) {
    case MAVLINK_MSG_ID_TERRAIN_REQUEST:
        _handleTerrainRequest(message);
        return false;
    case MAVLINK_MSG_ID_TERRAIN_REPORT:
        _handleTerrainReport(message);
        return false;
    default:
        return true;
    }
}

void TerrainProtocolHandler::_handleTerrainRequest(const mavlink_message_t &message)
{
    _terrainRequestActive = true;
    mavlink_msg_terrain_request_decode(&message, &_currentTerrainRequest);
    _sendNextTerrainData();
}

void TerrainProtocolHandler::_handleTerrainReport(const mavlink_message_t &message)
{
    mavlink_terrain_report_t terrainReport;
    mavlink_msg_terrain_report_decode(&message, &terrainReport);

    _terrainFactGroup->blocksPending()->setRawValue(terrainReport.pending);
    _terrainFactGroup->blocksLoaded()->setRawValue(terrainReport.loaded);

    if (TerrainProtocolHandlerLog().isDebugEnabled()) {
        bool error;
        QList<double> altitudes;
        QList<QGeoCoordinate> coordinates;
        QGeoCoordinate coord(static_cast<double>(terrainReport.lat) / 1e7, static_cast<double>(terrainReport.lon) / 1e7);
        (void) coordinates.append(coord);
        const bool altAvailable = TerrainAtCoordinateQuery::getAltitudesForCoordinates(coordinates, altitudes, error);
        const QString vehicleAlt = terrainReport.spacing ? QStringLiteral("%1").arg(terrainReport.terrain_height) : QStringLiteral("n/a");
        QString qgcAlt;
        if (error) {
            qgcAlt = QStringLiteral("Error");
        } else if (altAvailable && !altitudes.isEmpty()) {
            qgcAlt = QStringLiteral("%1").arg(altitudes[0]);
        } else {
            qgcAlt = QStringLiteral("N/A");
        }
        qCDebug(TerrainProtocolHandlerLog) << "TERRAIN_REPORT" << coord << QStringLiteral("Vehicle(%1) QGC(%2)").arg(vehicleAlt).arg(qgcAlt);
    }
}

void TerrainProtocolHandler::_sendNextTerrainData()
{
    if (!_terrainRequestActive) {
        return;
    }

    QGeoCoordinate terrainRequestCoordSWCorner(static_cast<double>(_currentTerrainRequest.lat) / 1e7, static_cast<double>(_currentTerrainRequest.lon) / 1e7);
    const int spacingBetweenGrids = _currentTerrainRequest.grid_spacing * 4;

    // Each TERRAIN_DATA sent to vehicle contains a 4x4 grid of heights
    // TERRAIN_REQUEST.mask has a bit for each entry in an 8x7 grid
    // gridBit = 0 refers to the the sw corner of the 8x7 grid

    bool bitFound = false;
    for (int rowIndex=0; rowIndex<7; rowIndex++) {
        for (int colIndex=0; colIndex<8; colIndex++) {
            const uint8_t gridBit = (rowIndex * 8) + colIndex;
            const uint64_t checkBit = 1ull << gridBit;
            if (_currentTerrainRequest.mask & checkBit) {
                // Move east and then north to generate the coordinate for sw corner of the specific gridBit
                QGeoCoordinate swCorner = terrainRequestCoordSWCorner.atDistanceAndAzimuth(spacingBetweenGrids * colIndex, 90);
                swCorner = swCorner.atDistanceAndAzimuth(spacingBetweenGrids * rowIndex, 0);
                _sendTerrainData(swCorner, gridBit);
                bitFound = true;
                break;
            }
        }

        if (bitFound) {
            break;
        }
    }

    if (bitFound) {
        // Kick timer to send next possible TERRAIN_DATA to vehicle
        _terrainDataSendTimer->start();
    } else {
        _terrainRequestActive = false;
    }
}

void TerrainProtocolHandler::_sendTerrainData(const QGeoCoordinate &swCorner, uint8_t gridBit)
{
    QList<QGeoCoordinate> coordinates;
    for (int rowIndex=0; rowIndex<4; rowIndex++) {
        for (int colIndex=0; colIndex<4; colIndex++) {
            // Move east and then north to generate the coordinate for grid point
            QGeoCoordinate coord = swCorner.atDistanceAndAzimuth(_currentTerrainRequest.grid_spacing * colIndex, 90);
            coord = coord.atDistanceAndAzimuth(_currentTerrainRequest.grid_spacing * rowIndex, 0);
            coordinates.append(coord);
        }
    }

    // Query terrain system for altitudes. If it has them available it will return them. If not they will be queued for download.
    bool error = false;
    QList<double> altitudes;
    if (!TerrainAtCoordinateQuery::getAltitudesForCoordinates(coordinates, altitudes, error)) {
        return;
    }

    if (error) {
        qCWarning(TerrainProtocolHandlerLog) << Q_FUNC_INFO << "TerrainAtCoordinateQuery::getAltitudesForCoordinates failed";
        return;
    }

    // Only clear the bit if the query succeeds. Otherwise just let it try again on the next timer tick
    const uint64_t removeBit = ~(1ull << gridBit);
    _currentTerrainRequest.mask &= removeBit;
    int altIndex = 0;
    int16_t terrainData[16];
    for (const double& altitude : altitudes) {
        terrainData[altIndex++] = static_cast<int16_t>(altitude);
    }

    SharedLinkInterfacePtr sharedLink = _vehicle->vehicleLinkManager()->primaryLink().lock();
    if (sharedLink) {
        mavlink_message_t msg;
        (void) mavlink_msg_terrain_data_pack_chan(
            MAVLinkProtocol::instance()->getSystemId(),
            MAVLinkProtocol::getComponentId(),
            sharedLink->mavlinkChannel(),
            &msg,
            _currentTerrainRequest.lat,
            _currentTerrainRequest.lon,
            _currentTerrainRequest.grid_spacing,
            gridBit,
            terrainData
        );

        _vehicle->sendMessageOnLinkThreadSafe(sharedLink.get(), msg);
    }
}
