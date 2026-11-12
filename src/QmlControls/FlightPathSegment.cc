#include "FlightPathSegment.h"

#include <cmath>

#include <QtCore/QtMath>

#include "QGCLoggingCategory.h"
#include "QGCMath.h"
#include "TerrainQuery.h"
#include "TerrainTileCopernicus.h"

QGC_LOGGING_CATEGORY(FlightPathSegmentLog, "Plan.FlightPathSegment")

FlightPathSegment::FlightPathSegment(SegmentType segmentType, const QGeoCoordinate& coord1, double amslCoord1Alt, const QGeoCoordinate& coord2, double amslCoord2Alt, bool queryTerrainData, QObject* parent)
    : QObject           (parent)
    , _coord1           (coord1)
    , _coord2           (coord2)
    , _coord1AMSLAlt     (amslCoord1Alt)
    , _coord2AMSLAlt     (amslCoord2Alt)
    , _queryTerrainData (queryTerrainData)
    , _segmentType      (segmentType)
{
    _delayedTerrainPathQueryTimer.setSingleShot(true);
    _delayedTerrainPathQueryTimer.setInterval(200);
    _delayedTerrainPathQueryTimer.callOnTimeout(this, &FlightPathSegment::_sendTerrainPathQuery);
    _updateTotalDistance();

    qCDebug(FlightPathSegmentLog) << this << "new" << coord1 << coord2 << amslCoord1Alt << amslCoord2Alt << _totalDistance;

    _sendTerrainPathQuery();
}

void FlightPathSegment::setCoordinate1(const QGeoCoordinate &coordinate)
{
    if (_coord1 != coordinate) {
        _coord1 = coordinate;
        emit coordinate1Changed(_coord1);
        _delayedTerrainPathQueryTimer.start();
        _updateTotalDistance();
    }
}

void FlightPathSegment::setCoordinate2(const QGeoCoordinate &coordinate)
{
    if (_coord2 != coordinate) {
        _coord2 = coordinate;
        emit coordinate2Changed(_coord2);
        _delayedTerrainPathQueryTimer.start();
        _updateTotalDistance();
    }
}

void FlightPathSegment::setCoord1AMSLAlt(double alt)
{
    if (!QGC::fuzzyCompare(alt, _coord1AMSLAlt)) {
        _coord1AMSLAlt = alt;
        emit coord1AMSLAltChanged();
        _updateTerrainCollision();
    }
}

void FlightPathSegment::setCoord2AMSLAlt(double alt)
{
    if (!QGC::fuzzyCompare(alt, _coord2AMSLAlt)) {
        _coord2AMSLAlt = alt;
        emit coord2AMSLAltChanged();
        _updateTerrainCollision();
    }
}

void FlightPathSegment::setSpecialVisual(bool specialVisual)
{
    if (_specialVisual != specialVisual) {
        _specialVisual = specialVisual;
        emit specialVisualChanged(specialVisual);
    }
}

void FlightPathSegment::setSplinePath(const QVariantList& splinePath)
{
    if (_splinePath == splinePath) {
        return;
    }
    _splinePath = splinePath;

    _splineSampleDistances.clear();
    double distance = 0;
    for (int i = 0; i < _splinePath.count(); i++) {
        if (i > 0) {
            distance += _splinePath[i - 1].value<QGeoCoordinate>().distanceTo(_splinePath[i].value<QGeoCoordinate>());
        }
        _splineSampleDistances.append(distance);
    }

    emit splinePathChanged();
    _updateTotalDistance();
    // The terrain under the leg follows the samples, so it must be sampled again
    _delayedTerrainPathQueryTimer.start();
}

int FlightPathSegment::_splineSpanAtDistance(double distance, double& fraction) const
{
    int span = 0;
    while (span < _splineSampleDistances.count() - 2 && _splineSampleDistances[span + 1] < distance) {
        span++;
    }
    const double spanLength = _splineSampleDistances[span + 1] - _splineSampleDistances[span];
    fraction = spanLength > 0 ? qBound(0.0, (distance - _splineSampleDistances[span]) / spanLength, 1.0) : 0.0;
    return span;
}

double FlightPathSegment::amslAltitudeAtDistance(double distance) const
{
    if (_splineSampleDistances.count() > 1) {
        double fraction = 0;
        const int span = _splineSpanAtDistance(distance, fraction);
        const double fromAlt = _splinePath[span].value<QGeoCoordinate>().altitude();
        const double toAlt = _splinePath[span + 1].value<QGeoCoordinate>().altitude();
        if (!qIsNaN(fromAlt) && !qIsNaN(toAlt)) {
            return fromAlt + ((toAlt - fromAlt) * fraction);
        }
    }

    if (_totalDistance <= 0) {
        return _coord1AMSLAlt;
    }
    return _coord1AMSLAlt + ((_coord2AMSLAlt - _coord1AMSLAlt) * qBound(0.0, distance / _totalDistance, 1.0));
}

QList<QGeoCoordinate> FlightPathSegment::_splineTerrainQueryCoords(double& distanceBetween,
                                                                   double& finalDistanceBetween) const
{
    QList<QGeoCoordinate> coordinates;

    const double totalDistance = _splineSampleDistances.last();
    const int numPoints = qMax(2, qCeil(totalDistance / TerrainTileCopernicus::kTileValueSpacingMeters) + 1);
    distanceBetween = finalDistanceBetween = totalDistance / (numPoints - 1);

    for (int i = 0; i < numPoints; i++) {
        double fraction = 0;
        const int span = _splineSpanAtDistance(i * distanceBetween, fraction);
        const QGeoCoordinate from = _splinePath[span].value<QGeoCoordinate>();
        const QGeoCoordinate to = _splinePath[span + 1].value<QGeoCoordinate>();
        coordinates.append(from.atDistanceAndAzimuth(from.distanceTo(to) * fraction, from.azimuthTo(to)));
    }

    return coordinates;
}

QVector3D FlightPathSegment::_localOffset(const QGeoCoordinate& from, const QGeoCoordinate& to)
{
    const double distance = from.distanceTo(to);
    const double azimuth = qDegreesToRadians(from.azimuthTo(to));
    const bool altitudesKnown = !qIsNaN(from.altitude()) && !qIsNaN(to.altitude());
    const double up = altitudesKnown ? to.altitude() - from.altitude() : 0.0;
    return QVector3D(distance * std::cos(azimuth), distance * std::sin(azimuth), up);
}

// ArduPilot flies a leg between two terrain-frame points in height above terrain rather than AMSL
QVector3D FlightPathSegment::_localOffset(const SplinePoint& from, const SplinePoint& to)
{
    QVector3D offset = _localOffset(from.coord, to.coord);
    if (from.terrainFrame && to.terrainFrame && !qIsNaN(from.aglAlt) && !qIsNaN(to.aglAlt)) {
        offset.setZ(to.aglAlt - from.aglAlt);
    }
    return offset;
}

// ArduPilot caps the tangents at four times the leg length so a curve cannot overshoot wildly
void FlightPathSegment::_capSplineTangents(const QVector3D& legOffset, QVector3D& originTangent,
                                           QVector3D& destinationTangent)
{
    const double tangentLength = originTangent.length() + destinationTangent.length();
    const double maxTangentLength = legOffset.length() * 4.0;
    if (tangentLength > maxTangentLength) {
        const double scale = maxTangentLength / tangentLength;
        originTangent *= scale;
        destinationTangent *= scale;
    }
}

// Cubic Hermite curve in a local north/east/up frame, matching ArduPilot's SplineCurve::set_origin_and_destination.
// The tangents must already be capped.
QVariantList FlightPathSegment::_hermiteSplinePath(const QGeoCoordinate& origin, const QGeoCoordinate& destination,
                                                   const QVector3D& legOffset, const QVector3D& originTangent,
                                                   const QVector3D& destinationTangent)
{
    const QVector3D& destinationOffset = legOffset;

    const QVector3D h2 = (originTangent * -2.0) + (destinationOffset * 3.0) - destinationTangent;
    const QVector3D h3 = originTangent - (destinationOffset * 2.0) + destinationTangent;

    // Sample densely enough that the chords between samples stay close to the curve on long legs
    const double lengthEstimate =
        destinationOffset.length() + ((originTangent.length() + destinationTangent.length()) / 2.0);
    const int sampleCount =
        qBound(_minSplineSampleCount, qCeil(lengthEstimate / _splineSampleSpacingMeters), _maxSplineSampleCount);

    QVariantList path;
    path.append(QVariant::fromValue(origin));
    for (int i = 1; i < sampleCount; i++) {
        const double t = static_cast<double>(i) / sampleCount;
        const QVector3D offset = (originTangent * t) + (h2 * t * t) + (h3 * t * t * t);
        const double distance = std::hypot(offset.x(), offset.y());
        // A NaN origin altitude stays NaN, flagging the sample altitude as unknown. A sample with no horizontal
        // offset reuses the origin position exactly rather than taking a round trip through the geodesic math.
        if (distance > 0) {
            const double azimuth = qRadiansToDegrees(std::atan2(offset.y(), offset.x()));
            path.append(QVariant::fromValue(origin.atDistanceAndAzimuth(distance, azimuth, offset.z())));
        } else {
            path.append(QVariant::fromValue(
                QGeoCoordinate(origin.latitude(), origin.longitude(), origin.altitude() + offset.z())));
        }
    }
    path.append(QVariant::fromValue(destination));

    return path;
}

QVariantList FlightPathSegment::splineSegmentPath(const QList<SplinePoint>& points, int index)
{
    if (index < 1 || index >= points.count()) {
        return QVariantList();
    }

    const SplinePoint& current = points[index];
    const SplinePoint& previous = points[index - 1];

    if (!current.spline || !current.coord.isValid() || !previous.coord.isValid()) {
        return QVariantList({QVariant::fromValue(previous.coord), QVariant::fromValue(current.coord)});
    }

    QVector3D originTangent;
    QVector3D destinationTangent;
    _splineTangents(points, index, originTangent, destinationTangent);

    return _hermiteSplinePath(previous.coord, current.coord, _localOffset(previous, current), originTangent,
                              destinationTangent);
}

// Tangents follow AC_WPNav::set_spline_destination: the vehicle only carries velocity through a fly-through
// waypoint and only between legs flown in the same altitude frame, a previous spline leg hands over its own
// exit velocity as computed before capping (SplineCurve keeps the unscaled vector), and the exit velocity aims
// at the next target (through the current point when the next leg is also a spline). A run of consecutive
// spline legs is therefore computed in order from its first leg.
void FlightPathSegment::_splineTangents(const QList<SplinePoint>& points, int index, QVector3D& originTangent,
                                        QVector3D& destinationTangent)
{
    int first = index;
    while (first > 1 && points[first - 1].spline && points[first - 1].flyThrough) {
        first--;
    }

    QVector3D carriedTangent;
    for (int i = first; i <= index; i++) {
        const SplinePoint& current = points[i];
        const SplinePoint& previous = points[i - 1];
        originTangent = QVector3D();
        destinationTangent = QVector3D();

        if (i > 1 && previous.flyThrough && !previous.runStart && points[i - 2].coord.isValid() &&
            previous.terrainFrame == current.terrainFrame) {
            originTangent = previous.spline ? carriedTangent : _localOffset(points[i - 2], previous);
        }

        if (i < points.count() - 1 && current.flyThrough) {
            const SplinePoint& next = points[i + 1];
            if (next.navTarget && next.coord.isValid() && next.terrainFrame == current.terrainFrame) {
                destinationTangent = _localOffset(next.spline ? previous : current, next);
            }
        }

        // A zero-length leg leaves with no velocity at all (SplineCurve zeroes both ends of such a track)
        const QVector3D legOffset = _localOffset(previous, current);
        carriedTangent = legOffset.isNull() ? QVector3D() : destinationTangent;
        _capSplineTangents(legOffset, originTangent, destinationTangent);
    }
}

void FlightPathSegment::_cancelTerrainQuery(void)
{
    // We don't care about the results of a query still in flight any more
    if (_currentTerrainPathQuery) {
        disconnect(_currentTerrainPathQuery, &TerrainPathQuery::terrainDataReceived, this,
                   &FlightPathSegment::_terrainDataReceived);
        _currentTerrainPathQuery = nullptr;
    }
    if (_currentTerrainCoordinateQuery) {
        disconnect(_currentTerrainCoordinateQuery, nullptr, this, nullptr);
        _currentTerrainCoordinateQuery = nullptr;
    }
}

void FlightPathSegment::_sendTerrainPathQuery(void)
{
    if (_queryTerrainData && _coord1.isValid() && _coord2.isValid()) {
        qCDebug(FlightPathSegmentLog) << this << "_sendTerrainPathQuery";
        _cancelTerrainQuery();

        // Clear old terrain data
        _amslTerrainHeights.clear();
        _distanceBetween = 0;
        _finalDistanceBetween = 0;
        emit distanceBetweenChanged(0);
        emit finalDistanceBetweenChanged(0);
        emit amslTerrainHeightsChanged();

        if (_splineSampleDistances.count() > 1) {
            // A spline leg is sampled along its curve rather than along the chord between its endpoints
            double distanceBetween = 0;
            double finalDistanceBetween = 0;
            const QList<QGeoCoordinate> coordinates = _splineTerrainQueryCoords(distanceBetween, finalDistanceBetween);
            _currentTerrainCoordinateQuery = new TerrainAtCoordinateQuery(true /* autoDelete */);
            connect(_currentTerrainCoordinateQuery, &TerrainAtCoordinateQuery::terrainDataReceived, this,
                    [this, distanceBetween, finalDistanceBetween](bool success, const QList<double>& heights) {
                        _currentTerrainCoordinateQuery = nullptr;
                        _terrainHeightsReceived(success, distanceBetween, finalDistanceBetween, heights);
                    });
            _currentTerrainCoordinateQuery->requestData(coordinates);
        } else {
            _currentTerrainPathQuery = new TerrainPathQuery(true /* autoDelete */);
            connect(_currentTerrainPathQuery, &TerrainPathQuery::terrainDataReceived, this,
                    &FlightPathSegment::_terrainDataReceived);
            _currentTerrainPathQuery->requestData(_coord1, _coord2);
        }
    }
}

void FlightPathSegment::_terrainDataReceived(bool success, const TerrainPathHeightInfo& pathHeightInfo)
{
    _currentTerrainPathQuery = nullptr;
    _terrainHeightsReceived(success, pathHeightInfo.distanceBetween, pathHeightInfo.finalDistanceBetween,
                            pathHeightInfo.heights);
}

void FlightPathSegment::_terrainHeightsReceived(bool success, double distanceBetween, double finalDistanceBetween,
                                                const QList<double>& heights)
{
    qCDebug(FlightPathSegmentLog) << this << "_terrainHeightsReceived" << success << heights.count();
    if (success) {
        if (!QGC::fuzzyCompare(distanceBetween, _distanceBetween)) {
            _distanceBetween = distanceBetween;
            emit distanceBetweenChanged(_distanceBetween);
        }
        if (!QGC::fuzzyCompare(finalDistanceBetween, _finalDistanceBetween)) {
            _finalDistanceBetween = finalDistanceBetween;
            emit finalDistanceBetweenChanged(_finalDistanceBetween);
        }

        _amslTerrainHeights.clear();
        for (const double& amslTerrainHeight : heights) {
            _amslTerrainHeights.append(amslTerrainHeight);
        }
        emit amslTerrainHeightsChanged();
    }

    _updateTerrainCollision();
}

void FlightPathSegment::_updateTotalDistance(void)
{
    double newTotalDistance = 0;
    if (_splineSampleDistances.count() > 1) {
        newTotalDistance = _splineSampleDistances.last();
    } else if (_coord1.isValid() && _coord2.isValid()) {
        newTotalDistance = _coord1.distanceTo(_coord2);
    }

    if (!QGC::fuzzyCompare(newTotalDistance, _totalDistance)) {
        _totalDistance = newTotalDistance;
        emit totalDistanceChanged(_totalDistance);
    }
}

void FlightPathSegment::_updateTerrainCollision(void)
{
    bool newTerrainCollision = false;

    if (_segmentType != SegmentTypeTerrainFrame) {
        // A spline with no horizontal extent (a vertical climb or descent) is checked at its lowest altitude
        double lowestVerticalAlt = qQNaN();
        if (_totalDistance <= 0) {
            for (const QVariant& sample : _splinePath) {
                lowestVerticalAlt = std::fmin(lowestVerticalAlt, sample.value<QGeoCoordinate>().altitude());
            }
        }

        double x = 0;
        for (int i=0; i<_amslTerrainHeights.count(); i++) {
            bool ignoreCollision = false;
            if (_segmentType == SegmentTypeTakeoff && x < _collisionIgnoreMeters) {
                ignoreCollision = true;
            } else if (_segmentType == SegmentTypeLand && x > _totalDistance - _collisionIgnoreMeters) {
                ignoreCollision = true;
            }

            if (!ignoreCollision) {
                double y = _amslTerrainHeights[i].value<double>();
                const double flightAlt = qIsNaN(lowestVerticalAlt) ? amslAltitudeAtDistance(x) : lowestVerticalAlt;
                if (y > flightAlt) {
                    newTerrainCollision = true;
                    break;
                }
            }

            if (i == _amslTerrainHeights.count() - 2) {
                x += _finalDistanceBetween;
            } else {
                x += _distanceBetween;
            }
        }
    }

    qCDebug(FlightPathSegmentLog) << this << "_updateTerrainCollision new:old" << newTerrainCollision << _terrainCollision;

    if (newTerrainCollision != _terrainCollision) {
        _terrainCollision = newTerrainCollision;
        emit terrainCollisionChanged(_terrainCollision);
    }
}
