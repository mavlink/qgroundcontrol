#include "GPSRelativePositionModel.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GPSRelativePositionModelLog, "GPS.Models.GPSRelativePositionModel")

GPSRelativePositionModel::GPSRelativePositionModel(GPSRelativePositionStore& store, QObject* parent) : QObject(parent)
{
    qCDebug(GPSRelativePositionModelLog) << this;
    if (store.thread() != thread()) {
        qCWarning(GPSRelativePositionModelLog) << "Store must share the projection thread";
    } else {
        _store = &store;
        connect(_store, &GPSRelativePositionStore::observationChanged, this, &GPSRelativePositionModel::_project);
        connect(_store, &QObject::destroyed, this, &GPSRelativePositionModel::_project);
    }
    _project();
}

GPSRelativePositionModel::~GPSRelativePositionModel()
{
    qCDebug(GPSRelativePositionModelLog) << this;
    if (_store) {
        _store->disconnect(this);
    }
}

double GPSRelativePositionModel::_positionValue(double value, bool accuracy) const
{
    return _fresh && _observation.fixValid && _observation.positionValid && std::isfinite(value) &&
                   (!accuracy || value >= 0)
               ? value
               : std::numeric_limits<double>::quiet_NaN();
}

double GPSRelativePositionModel::north() const
{
    return _positionValue(_observation.positionNedMeters[0]);
}

double GPSRelativePositionModel::east() const
{
    return _positionValue(_observation.positionNedMeters[1]);
}

double GPSRelativePositionModel::down() const
{
    return _positionValue(_observation.positionNedMeters[2]);
}

double GPSRelativePositionModel::northAccuracy() const
{
    return _positionValue(_observation.accuracyNedMeters[0], true);
}

double GPSRelativePositionModel::eastAccuracy() const
{
    return _positionValue(_observation.accuracyNedMeters[1], true);
}

double GPSRelativePositionModel::downAccuracy() const
{
    return _positionValue(_observation.accuracyNedMeters[2], true);
}

double GPSRelativePositionModel::length() const
{
    return _positionValue(_observation.lengthMeters, true);
}

double GPSRelativePositionModel::lengthAccuracy() const
{
    return _positionValue(_observation.lengthAccuracyMeters, true);
}

double GPSRelativePositionModel::heading() const
{
    const auto value = _observation.headingDegrees;
    return _fresh && _observation.fixValid && value && std::isfinite(*value) && *value >= 0 && *value <= 360
               ? (*value == 360 ? 0 : *value)
               : std::numeric_limits<double>::quiet_NaN();
}

double GPSRelativePositionModel::headingAccuracy() const
{
    const auto value = _observation.headingAccuracyDegrees;
    return std::isfinite(heading()) && value && std::isfinite(*value) && *value >= 0
               ? *value
               : std::numeric_limits<double>::quiet_NaN();
}

std::array<QVariant, 23> GPSRelativePositionModel::_values() const
{
    return {sourceId(),
            QVariant::fromValue(sessionId()),
            fresh(),
            referenceStationId(),
            north(),
            east(),
            down(),
            northAccuracy(),
            eastAccuracy(),
            downAccuracy(),
            length(),
            lengthAccuracy(),
            heading(),
            headingAccuracy(),
            fixValid(),
            differential(),
            positionValid(),
            carrierFloat(),
            carrierFixed(),
            movingBase(),
            referencePositionMissing(),
            referenceObservationsMissing(),
            normalized()};
}

void GPSRelativePositionModel::_project()
{
    const auto previous = _values();
    _sourceId = _store ? _store->sourceId() : QString();
    _sessionId = _store ? _store->sessionId() : 0;
    _observation = _store ? _store->observation() : GPSRelativeObservation();
    _fresh = _store && _store->fresh();
    const auto next = _values();
    for (size_t index = 0; index < next.size(); ++index) {
        const bool bothNaN = next[index].metaType() == QMetaType::fromType<double>() &&
                             previous[index].metaType() == QMetaType::fromType<double>() &&
                             std::isnan(next[index].toDouble()) && std::isnan(previous[index].toDouble());
        if (!bothNaN && next[index] != previous[index]) {
            emit stateChanged();
            return;
        }
    }
}
