#include "GPSReceiverFactGroup.h"

#include <cmath>
#include <limits>
#include <utility>

#include "GPSReceiverConfig.h"
#include "GPSReceiverReports.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GPSReceiverFactGroupLog, "GPS.Receiver.GPSReceiverFactGroup")

GPSReceiverFactGroup::GPSReceiverFactGroup(const GPSReceiver* receiver, QObject* parent)
    : FactGroup(1000, QStringLiteral(":/json/Vehicle/GPSReceiverFact.json"), parent)
    , _receiver(receiver)
{
    // qCDebug(GPSReceiverFactGroupLog) << Q_FUNC_INFO << this;

    _addFact(&_connectedFact);
    _addFact(&_currentDurationFact);
    _addFact(&_currentAccuracyFact);
    _addFact(&_currentLatitudeFact);
    _addFact(&_currentLongitudeFact);
    _addFact(&_currentAltitudeFact);
    _addFact(&_validFact);
    _addFact(&_activeFact);
    _addFact(&_numSatellitesFact);
    _addFact(&_numSatellitesUsedFact);
    _addFact(&_fixTypeFact);
    _addFact(&_jammingStateFact);
    _addFact(&_spoofingStateFact);

    for (Fact* fact :
         {&_validFact, &_currentLatitudeFact, &_currentLongitudeFact, &_currentAltitudeFact, &_currentAccuracyFact}) {
        connect(fact, &Fact::rawValueChanged, this, &GPSReceiverFactGroup::currentBasePositionChanged);
    }
    for (Fact* fact : {&_jammingStateFact, &_spoofingStateFact}) {
        connect(fact, &Fact::rawValueChanged, this, &GPSReceiverFactGroup::interferenceWarningChanged);
    }
    for (Fact* fact : {&_fixTypeFact, &_activeFact}) {
        connect(fact, &Fact::rawValueChanged, this, &GPSReceiverFactGroup::_updateSummaryLabel);
    }
    if (receiver) {
        _mirror(receiver->status());
        connect(receiver, &GPSReceiver::statusChanged, this, [this, receiver]() { _mirror(receiver->status()); });
        connect(receiver, &GPSReceiver::receiverChanged, this, &GPSReceiverFactGroup::_updateSummaryLabel);
    }
    _updateSummaryLabel();
}

void GPSReceiverFactGroup::_updateSummaryLabel()
{
    QString label;
    if (_receiver && _receiver->hasReceiver() && _receiver->activeRole() == GPSReceiver::ConfiguredBase) {
        label = _activeFact.rawValue().toBool() ? tr("Survey", "Base survey-in in progress") : tr("Base");
    } else {
        switch (gpsFixQualityFromValue(_fixTypeFact.rawValue().toInt())) {
            case GPSFixQuality::NoFix:
                label = tr("No fix");
                break;
            case GPSFixQuality::Fix2D:
                label = tr("2D");
                break;
            case GPSFixQuality::Fix3D:
                label = tr("3D");
                break;
            case GPSFixQuality::Differential:
                label = tr("DGPS");
                break;
            case GPSFixQuality::RTKFloat:
                label = tr("Float", "RTK float fix");
                break;
            case GPSFixQuality::RTKFixed:
                label = tr("Fixed", "RTK fixed fix");
                break;
            case GPSFixQuality::Extrapolated:
                label = tr("DR", "Dead reckoning (extrapolated) fix");
                break;
            case GPSFixQuality::Unknown:
                break;
        }
    }
    if (std::exchange(_summaryLabel, label) != label) {
        emit summaryLabelChanged();
    }
}

GPSReceiverFactGroup::~GPSReceiverFactGroup()
{
    // qCDebug(GPSReceiverFactGroupLog) << Q_FUNC_INFO << this;
}

void GPSReceiverFactGroup::_mirror(GPSReceiver::Status status)
{
    const auto update = [](Fact* fact, const QVariant& value) {
        // Unavailable values stay NaN; rewriting NaN would report a change, as NaN never equals itself.
        if (!std::isnan(value.toDouble()) || !std::isnan(fact->rawValue().toDouble())) {
            fact->setRawValue(value);
        }
    };
    update(&_connectedFact, status.connected);
    update(&_currentDurationFact, static_cast<qint64>(status.currentDuration.count()));
    update(&_currentAccuracyFact, status.currentAccuracy);
    update(&_currentLatitudeFact, status.currentLatitude);
    update(&_currentLongitudeFact, status.currentLongitude);
    update(&_currentAltitudeFact, status.currentAltitude);
    update(&_validFact, status.valid);
    update(&_activeFact, status.active);
    update(&_numSatellitesFact, status.numSatellites);
    update(&_numSatellitesUsedFact, status.numSatellitesUsed);
    update(&_fixTypeFact, static_cast<int>(status.fixType));
    update(&_jammingStateFact, static_cast<int>(status.jammingState));
    update(&_spoofingStateFact, static_cast<int>(status.spoofingState));
}

bool GPSReceiverFactGroup::interferenceWarning() const
{
    using JammingState = GPSIntegrityReport::JammingState;
    using SpoofingState = GPSIntegrityReport::SpoofingState;
    const int jamming = _jammingStateFact.rawValue().toInt();
    const int spoofing = _spoofingStateFact.rawValue().toInt();
    return jamming == static_cast<int>(JammingState::Warning) || jamming == static_cast<int>(JammingState::Critical) ||
           spoofing == static_cast<int>(SpoofingState::Indicated) ||
           spoofing == static_cast<int>(SpoofingState::Multiple);
}

bool GPSReceiverFactGroup::canSaveCurrentBasePosition() const
{
    const double accuracy = _currentAccuracyFact.rawValue().toDouble();
    if (!_validFact.rawValue().toBool() || !std::isfinite(accuracy) || accuracy < 0 ||
        accuracy > (std::numeric_limits<float>::max)()) {
        return false;
    }
    const GPSBaseStationConfig config{
        .mode =
            GPSBaseStationConfig::Fixed{.position = {.latitudeDegrees = _currentLatitudeFact.rawValue().toDouble(),
                                                     .longitudeDegrees = _currentLongitudeFact.rawValue().toDouble(),
                                                     .altitudeMeters = _currentAltitudeFact.rawValue().toFloat()},
                                        .accuracyMeters = static_cast<float>(accuracy)},
    };
    return gpsValidateBaseStationConfig(config) == GPSReceiverConfigError::None;
}
