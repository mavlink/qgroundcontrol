#include "GPSPositionFactGroup.h"

#include <QtCore/QPointer>

#include <array>
#include <utility>

#include "QGCGeo.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GPSPositionFactGroupLog, "GPS.Models.GPSPositionFactGroup")

GPSPositionFactGroup::GPSPositionFactGroup(QObject* parent, GPSRuntimeScheduler* scheduler)
    : FactGroup(1000, QStringLiteral(":/json/GPS/Position/GPSFact.json"), parent)
{
    qCDebug(GPSPositionFactGroupLog) << this;
    _integrity = new GPSIntegrityFactGroup(this, scheduler);
    _addFactGroup(_integrity, QStringLiteral("integrity"));
    for (Fact* fact : {&_latFact, &_lonFact, &_mgrsFact, &_hdopFact, &_vdopFact, &_courseOverGroundFact, &_yawFact,
                       &_countFact, &_lockFact}) {
        _addFact(fact);
    }
    resetPosition();
    _countFact.setRawValue(0);
}

GPSPositionFactGroup::~GPSPositionFactGroup()
{
    qCDebug(GPSPositionFactGroupLog) << this;
}

void GPSPositionFactGroup::updatePosition(const GPSObservation& observation, std::optional<int> satelliteCount,
                                          std::optional<int> lockCode)
{
    const QPointer<GPSPositionFactGroup> guard(this);
    const quint64 revision = ++_positionRevision;
    const auto coordinate = observation.position.coordinate();
    const bool hasPosition = observation.position.isValid();
    // Dead reckoning has no equivalent in the vehicle GPS-lock enumeration.
    const int fixType = lockCode.value_or(observation.fixQuality == GPSObservation::FixQuality::Extrapolated
                                              ? 0
                                              : static_cast<int>(observation.fixQuality));
    const std::array<std::pair<Fact*, QVariant>, 8> values = {{
        {lat(), hasPosition ? coordinate.latitude() : qQNaN()},
        {lon(), hasPosition ? coordinate.longitude() : qQNaN()},
        {mgrs(), hasPosition ? QGCGeo::convertGeoToMGRS(coordinate) : QString()},
        {hdop(), observation.horizontalDop.value_or(qQNaN())},
        {vdop(), observation.verticalDop.value_or(qQNaN())},
        {courseOverGround(), observation.position.attribute(QGeoPositionInfo::Direction)},
        {yaw(), observation.trueHeadingDegrees.value_or(qQNaN())},
        {lock(), fixType},
    }};
    for (const auto& [fact, value] : values) {
        if (!guard || _positionRevision != revision) {
            return;
        }
        fact->setRawValue(value);
    }
    if (guard && _positionRevision == revision && satelliteCount) {
        count()->setRawValue(*satelliteCount);
    }
    if (guard && _positionRevision == revision) {
        _setTelemetryAvailable(true);
    }
}

void GPSPositionFactGroup::resetPosition()
{
    const QPointer<GPSPositionFactGroup> guard(this);
    const quint64 revision = ++_positionRevision;
    const std::array<std::pair<Fact*, QVariant>, 8> values = {{
        {lat(), qQNaN()},
        {lon(), qQNaN()},
        {mgrs(), QString()},
        {hdop(), qQNaN()},
        {vdop(), qQNaN()},
        {courseOverGround(), qQNaN()},
        {yaw(), qQNaN()},
        {lock(), 0},
    }};
    for (const auto& [fact, value] : values) {
        if (!guard || _positionRevision != revision) {
            return;
        }
        fact->setRawValue(value);
    }
    if (guard && _positionRevision == revision) {
        _setTelemetryAvailable(false);
    }
}
