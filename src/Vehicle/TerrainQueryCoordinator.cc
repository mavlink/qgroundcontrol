#include "TerrainQueryCoordinator.h"

#include <cmath>

#include "AppMessages.h"
#include "QGCLoggingCategory.h"
#include "TerrainQuery.h"
#include "Vehicle.h"

QGC_LOGGING_CATEGORY(TerrainQueryCoordinatorLog, "Vehicle.TerrainQueryCoordinator")

namespace {
// DO_SET_HOME sanity bounds for resolved terrain altitude (AMSL, meters).
constexpr double kSetHomeTerrainAltMax = 10000.0;
constexpr double kSetHomeTerrainAltMin = -500.0;

// Altitude-above-terrain polling throttle.
constexpr int    kAltQueryMinIntervalMs  = 500;
constexpr qreal  kAltMinDistanceTraveled = 2.0;   // meters
constexpr float  kAltMinAltitudeChanged  = 0.5f;  // meters
} // namespace

TerrainQueryCoordinator::TerrainQueryCoordinator(Vehicle* vehicle)
    : QObject(vehicle)
    , _vehicle(vehicle)
{
    _altQueryTimer.restart();
}

void TerrainQueryCoordinator::doSetHomeWithTerrain(const QGeoCoordinate& coord)
{
    if (!coord.isValid()) {
        return;
    }

    // If a prior query is still in-flight, disconnect it and start a new one. TerrainQuery
    // auto-deletes the old query; we just let it go. This matters when two doSetHome
    // commands arrive back-to-back: only the most recent should drive the outbound command.
    if (_doSetHomeQuery) {
        disconnect(_doSetHomeQuery, &TerrainAtCoordinateQuery::terrainDataReceived,
                   this, &TerrainQueryCoordinator::_doSetHomeTerrainReceived);
        _doSetHomeQuery = nullptr;
    }

    _doSetHomeCoordinate = coord;

    _doSetHomeQuery = new TerrainAtCoordinateQuery(true /* autoDelete */);
    connect(_doSetHomeQuery, &TerrainAtCoordinateQuery::terrainDataReceived,
            this, &TerrainQueryCoordinator::_doSetHomeTerrainReceived);

    QList<QGeoCoordinate> rgCoord;
    rgCoord.append(coord);
    _doSetHomeQuery->requestData(rgCoord);
}

void TerrainQueryCoordinator::_doSetHomeTerrainReceived(bool success, QList<double> heights)
{
    if (success) {
        const double terrainAltitude = heights[0];
        if (_doSetHomeCoordinate.isValid()
            && terrainAltitude <= kSetHomeTerrainAltMax
            && terrainAltitude >= kSetHomeTerrainAltMin) {
            _vehicle->sendMavCommand(
                _vehicle->defaultComponentId(),
                MAV_CMD_DO_SET_HOME,
                true, // show error if fails
                0,
                0,
                0,
                static_cast<float>(qQNaN()),
                _doSetHomeCoordinate.latitude(),
                _doSetHomeCoordinate.longitude(),
                terrainAltitude);
        } else if (_doSetHomeCoordinate.isValid()) {
            qCDebug(TerrainQueryCoordinatorLog) << "_doSetHomeTerrainReceived: elevation data out of limits";
        } else {
            qCDebug(TerrainQueryCoordinatorLog) << "_doSetHomeTerrainReceived: cached home coordinate not valid";
        }
    } else {
        QGC::showAppMessage(tr("Set Home failed, terrain data not available for selected coordinate"));
    }

    _doSetHomeQuery = nullptr;
    _doSetHomeCoordinate = QGeoCoordinate(); // Invalidate for extra safety
}

void TerrainQueryCoordinator::roiWithTerrain(const QGeoCoordinate& coord)
{
    if (!coord.isValid()) {
        return;
    }

    if (_roiQuery) {
        disconnect(_roiQuery, &TerrainAtCoordinateQuery::terrainDataReceived,
                   this, &TerrainQueryCoordinator::_roiTerrainReceived);
        _roiQuery = nullptr;
    }

    _roiCoordinate = coord;

    _roiQuery = new TerrainAtCoordinateQuery(true /* autoDelete */);
    connect(_roiQuery, &TerrainAtCoordinateQuery::terrainDataReceived,
            this, &TerrainQueryCoordinator::_roiTerrainReceived);

    QList<QGeoCoordinate> rgCoord;
    rgCoord.append(coord);
    _roiQuery->requestData(rgCoord);
}

void TerrainQueryCoordinator::_roiTerrainReceived(bool success, QList<double> heights)
{
    _roiQuery = nullptr;

    if (!_roiCoordinate.isValid()) {
        return;
    }

    float roiAltitude;
    if (success) {
        roiAltitude = static_cast<float>(heights[0]);
    } else {
        qCDebug(TerrainQueryCoordinatorLog) << "_roiTerrainReceived: terrain query failed, falling back to home altitude";
        roiAltitude = static_cast<float>(_vehicle->homePosition().altitude());
    }

    qCDebug(TerrainQueryCoordinatorLog) << "roiWithTerrain: lat" << _roiCoordinate.latitude()
                                        << "lon" << _roiCoordinate.longitude()
                                        << "terrainAltAMSL" << roiAltitude << "success" << success;

    sendROICommand(_roiCoordinate, MAV_FRAME_GLOBAL, roiAltitude);

    _roiCoordinate = QGeoCoordinate();
}

void TerrainQueryCoordinator::sendROICommand(const QGeoCoordinate& coord, MAV_FRAME frame, float altitude)
{
    if (_vehicle->capabilityBits() & MAV_PROTOCOL_CAPABILITY_COMMAND_INT) {
        _vehicle->sendMavCommandInt(
            _vehicle->defaultComponentId(),
            MAV_CMD_DO_SET_ROI_LOCATION,
            frame,
            true,                           // show error if fails
            static_cast<float>(qQNaN()),
            static_cast<float>(qQNaN()),
            static_cast<float>(qQNaN()),
            static_cast<float>(qQNaN()),
            coord.latitude(),
            coord.longitude(),
            altitude);
    } else {
        _vehicle->sendMavCommand(
            _vehicle->defaultComponentId(),
            MAV_CMD_DO_SET_ROI_LOCATION,
            true,                           // show error if fails
            static_cast<float>(qQNaN()),
            static_cast<float>(qQNaN()),
            static_cast<float>(qQNaN()),
            static_cast<float>(qQNaN()),
            static_cast<float>(coord.latitude()),
            static_cast<float>(coord.longitude()),
            altitude);
    }
}

void TerrainQueryCoordinator::updateAltAboveTerrain()
{
    // Throttle: no query sooner than 500 ms after the last one. Any requests that miss
    // this window are fine — the next coordinateChanged will re-trigger us.
    if (_altQueryTimer.elapsed() < kAltQueryMinIntervalMs) {
        return;
    }

    const QGeoCoordinate currentCoord = _vehicle->coordinate();
    if (!currentCoord.isValid()) {
        return;
    }

    // Throttle: require minimum distance/altitude deltas to avoid pounding the
    // terrain service with near-identical queries.
    const float currentRelAlt = _vehicle->altitudeRelative()->rawValue().toFloat();
    if (_altLastCoord.isValid() && !qIsNaN(_altLastRelAlt)) {
        if (_altLastCoord.distanceTo(currentCoord) < kAltMinDistanceTraveled
            && std::fabs(currentRelAlt - _altLastRelAlt) < kAltMinAltitudeChanged) {
            return;
        }
    }

    _altLastCoord  = currentCoord;
    _altLastRelAlt = currentRelAlt;

    if (_altQuery) {
        disconnect(_altQuery, &TerrainAtCoordinateQuery::terrainDataReceived,
                   this, &TerrainQueryCoordinator::_altitudeAboveTerrainReceived);
        _altQuery = nullptr;
    }

    _altQuery = new TerrainAtCoordinateQuery(true /* autoDelete */);
    connect(_altQuery, &TerrainAtCoordinateQuery::terrainDataReceived,
            this, &TerrainQueryCoordinator::_altitudeAboveTerrainReceived);

    QList<QGeoCoordinate> rgCoord;
    rgCoord.append(currentCoord);
    _altQuery->requestData(rgCoord);
    _altQueryTimer.restart();
}

void TerrainQueryCoordinator::_altitudeAboveTerrainReceived(bool success, QList<double> heights)
{
    if (!success) {
        qCDebug(TerrainQueryCoordinatorLog) << "_altitudeAboveTerrainReceived: terrain data not available";
    } else {
        const double terrainAltitude     = heights[0];
        const double altitudeAboveTerrain = _vehicle->altitudeAMSL()->rawValue().toDouble() - terrainAltitude;
        _vehicle->altitudeAboveTerr()->setRawValue(altitudeAboveTerrain);
    }

    _altQuery = nullptr;
}
