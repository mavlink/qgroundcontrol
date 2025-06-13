/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "FlightPathSegment.h"
#include "QGC.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(FlightPathSegmentLog, "FlightPathSegmentLog")

FlightPathSegment::FlightPathSegment(SegmentType segmentType, const QGeoCoordinate& coord1, double amslCoord1Alt, const QGeoCoordinate& coord2, double amslCoord2Alt, bool queryTerrainData, QObject* parent)
    : QObject           (parent)
    , _realCoord1       (coord1)
    , _realCoord2       (coord2)
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
    if (_realCoord1 != coordinate) {
        _realCoord1 = coordinate;
        emit coordinate1Changed(_realCoord1);
        _delayedTerrainPathQueryTimer.start();
        _updateTotalDistance();
        adjustCoordinatesForLoiterRadius();
    }
}

void FlightPathSegment::setCoordinate2(const QGeoCoordinate &coordinate)
{
    if (_realCoord2 != coordinate) {
        _realCoord2 = coordinate;
        emit coordinate2Changed(_realCoord2);
        _delayedTerrainPathQueryTimer.start();
        _updateTotalDistance();
        adjustCoordinatesForLoiterRadius();
    }
}

void FlightPathSegment::setCoord1LoiterRadius(double loiterRadius) {
    if (qFuzzyCompare(_coord1LoiterRadius, loiterRadius)) 
        return; // No change
    _coord1LoiterRadius = loiterRadius;
    adjustCoordinatesForLoiterRadius();
}

void FlightPathSegment::setCoord2LoiterRadius(double loiterRadius) {
    if (qFuzzyCompare(_coord2LoiterRadius, loiterRadius)) 
        return; // No change
    _coord2LoiterRadius = loiterRadius;
    adjustCoordinatesForLoiterRadius();
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

void FlightPathSegment::_sendTerrainPathQuery(void)
{
    if (_queryTerrainData && _realCoord1.isValid() && _realCoord2.isValid()) {
        qCDebug(FlightPathSegmentLog) << this << "_sendTerrainPathQuery";
        // Clear any previous query
        if (_currentTerrainPathQuery) {
            // We are already waiting on another query. We don't care about those results any more.
            disconnect(_currentTerrainPathQuery, &TerrainPathQuery::terrainDataReceived, this, &FlightPathSegment::_terrainDataReceived);
            _currentTerrainPathQuery = nullptr;
        }

        // Clear old terrain data
        _amslTerrainHeights.clear();
        _distanceBetween = 0;
        _finalDistanceBetween = 0;
        emit distanceBetweenChanged(0);
        emit finalDistanceBetweenChanged(0);
        emit amslTerrainHeightsChanged();

        _currentTerrainPathQuery = new TerrainPathQuery(true /* autoDelete */);
        connect(_currentTerrainPathQuery, &TerrainPathQuery::terrainDataReceived, this, &FlightPathSegment::_terrainDataReceived);
        _currentTerrainPathQuery->requestData(_realCoord1, _realCoord2);
    }
}

void FlightPathSegment::_terrainDataReceived(bool success, const TerrainPathQuery::PathHeightInfo_t& pathHeightInfo)
{
    qCDebug(FlightPathSegmentLog) << this << "_terrainDataReceived" << success << pathHeightInfo.heights.count();
    if (success) {
        if (!QGC::fuzzyCompare(pathHeightInfo.distanceBetween, _distanceBetween)) {
            _distanceBetween = pathHeightInfo.distanceBetween;
            emit distanceBetweenChanged(_distanceBetween);
        }
        if (!QGC::fuzzyCompare(pathHeightInfo.finalDistanceBetween, _finalDistanceBetween)) {
            _finalDistanceBetween = pathHeightInfo.finalDistanceBetween;
            emit finalDistanceBetweenChanged(_finalDistanceBetween);
        }

        _amslTerrainHeights.clear();
        for (const double& amslTerrainHeight: pathHeightInfo.heights) {
            _amslTerrainHeights.append(amslTerrainHeight);
        }
        emit amslTerrainHeightsChanged();
    }

    _currentTerrainPathQuery->deleteLater();
    _currentTerrainPathQuery = nullptr;

    _updateTerrainCollision();
}

void FlightPathSegment::_updateTotalDistance(void)
{
    double newTotalDistance = 0;
    if (_realCoord1.isValid() && _realCoord2.isValid()) {
        newTotalDistance = _realCoord1.distanceTo(_realCoord2);
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
        double slope =      (_coord2AMSLAlt - _coord1AMSLAlt) / _totalDistance;
        double yIntercept = _coord1AMSLAlt;

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
                if (y > (slope * x) + yIntercept) {
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

void FlightPathSegment::adjustCoordinatesForLoiterRadius() {
    bool hasR1 = !qIsNaN(_coord1LoiterRadius);
    bool hasR2 = !qIsNaN(_coord2LoiterRadius);
    if (!hasR1 && !hasR2) {
        _coord1 = _realCoord1;
        _coord2 = _realCoord2;
        return;
    }
    QGeoCoordinate c1 = _realCoord1;
    QGeoCoordinate c2 = _realCoord2;
    double distance = c1.distanceTo(c2);
    double azimuth = c1.azimuthTo(c2);
    if (hasR1 && hasR2)
    {
        if (distance >= fabs(_coord1LoiterRadius - _coord2LoiterRadius)) {
            double alphaRad = qAcos((_coord1LoiterRadius - _coord2LoiterRadius) / distance);
            double alphaDeg = alphaRad * 180.0 / M_PI;
            double θ = azimuth - alphaDeg;  
            double angle1 = (_coord1LoiterRadius >= 0) ? θ : (θ + 180.0);
            double angle2 = (_coord2LoiterRadius >= 0) ? θ : (θ + 180.0);
            _coord1 = c1.atDistanceAndAzimuth(fabs(_coord1LoiterRadius), angle1);
            _coord2 = c2.atDistanceAndAzimuth(fabs(_coord2LoiterRadius), angle2);
        } else {
            _coord1 = QGeoCoordinate();
            _coord2 = QGeoCoordinate();
        }
    }
    else if (hasR2) {
        if (distance > _coord2LoiterRadius) {
            double azimuth2to1 = c2.azimuthTo(c1);
            double angleOffset = qAsin(_coord2LoiterRadius / distance) * 180.0 / M_PI;
            double angle = azimuth2to1 + 90.0 - angleOffset;

            QGeoCoordinate tangentPoint = c2.atDistanceAndAzimuth(_coord2LoiterRadius, angle);
            _coord1 = _realCoord1;
            _coord2 = tangentPoint;
        } else {
            _coord2 = QGeoCoordinate();
        }
    }
    else if (hasR1) {
        if (distance > _coord1LoiterRadius) {
            double angleOffset = qAsin(_coord1LoiterRadius / distance) * 180.0 / M_PI;
            double angleCCW = azimuth - 90.0 + angleOffset;
            QGeoCoordinate tangentPointCCW = c1.atDistanceAndAzimuth(_coord1LoiterRadius, angleCCW);
            _coord1 = tangentPointCCW;
            _coord2 = _realCoord2;
        }
    }
    emit coordinate1Changed(_coord1);
    emit coordinate2Changed(_coord2);
}
