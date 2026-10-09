#pragma once

#include <initializer_list>
#include <optional>

#include <QtQmlIntegration/QtQmlIntegration>

#include "FactGroup.h"
#include "GPSReceiverConfig.h"
#include "GPSReceiverReports.h"

/// The local receiver's status as Facts, which its GPSReceiver writes as the receiver reports. Raw values notify when
/// written; the values QML shows notify at most once a second. telemetryAvailable follows whether the receiver finished
/// configuration.
class GPSReceiverFactGroup : public FactGroup
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Provided by GPSReceiver")
    Q_PROPERTY(Fact* currentDuration READ currentDuration CONSTANT FINAL)
    Q_PROPERTY(Fact* currentAccuracy READ currentAccuracy CONSTANT FINAL)
    Q_PROPERTY(Fact* valid READ valid CONSTANT FINAL)
    Q_PROPERTY(Fact* active READ active CONSTANT FINAL)
    Q_PROPERTY(Fact* numSatellites READ numSatellites CONSTANT FINAL)
    Q_PROPERTY(Fact* numSatellitesUsed READ numSatellitesUsed CONSTANT FINAL)
    Q_PROPERTY(Fact* fixType READ fixType CONSTANT FINAL)
    Q_PROPERTY(Fact* jammingState READ jammingState CONSTANT FINAL)
    Q_PROPERTY(Fact* spoofingState READ spoofingState CONSTANT FINAL)
    Q_PROPERTY(Fact* antennaState READ antennaState CONSTANT FINAL)
    Q_PROPERTY(bool canSaveCurrentBasePosition READ canSaveCurrentBasePosition NOTIFY currentBasePositionChanged FINAL)
    /// The current base position was saved to the settings since the survey that produced it started.
    Q_PROPERTY(bool currentBasePositionSaved READ currentBasePositionSaved NOTIFY currentBasePositionChanged FINAL)
    Q_PROPERTY(bool receiverWarning READ receiverWarning NOTIFY receiverWarningChanged FINAL)

    friend class GPSReceiver;
    friend class GPSManager;

public:
    Fact* currentDuration() { return &_currentDurationFact; }

    Fact* currentAccuracy() { return &_currentAccuracyFact; }

    Fact* valid() { return &_validFact; }

    Fact* active() { return &_activeFact; }

    Fact* numSatellites() { return &_numSatellitesFact; }

    Fact* numSatellitesUsed() { return &_numSatellitesUsedFact; }

    Fact* fixType() { return &_fixTypeFact; }

    Fact* jammingState() { return &_jammingStateFact; }

    Fact* spoofingState() { return &_spoofingStateFact; }

    Fact* antennaState() { return &_antennaStateFact; }

    /// The receiver's valid base position with a known accuracy, which a fixed-position base can start from; empty
    /// otherwise. A valid receiver status alone does not establish usable coordinates or accuracy.
    std::optional<GPSBaseStationConfig::Fixed> currentBasePosition() const { return _currentBasePosition; }

    bool canSaveCurrentBasePosition() const { return _currentBasePosition.has_value(); }

    bool currentBasePositionSaved() const { return _currentBasePositionSaved; }

    /// Jamming at Warning or Critical, any spoofing indication, or an open or shorted antenna.
    bool receiverWarning() const { return _receiverWarning; }

signals:
    void currentBasePositionChanged();
    void receiverWarningChanged();

private:
    /// Starts with no receiver: every Fact at its metadata default.
    explicit GPSReceiverFactGroup(QObject* parent);

    // Written by GPSReceiver.
    void _setSurvey(const GPSSurveyReport& report);
    void _setSatellites(int inView, int used);
    void _setSolution(GPSFixQuality fixType, const GPSIntegrityReport& integrity);
    /// Clears the fix, satellites, and integrity of a receiver that stopped reporting position.
    void _clearSolution();
    /// Restores the status of no receiver.
    void _reset();

    void _setConfigured(bool configured) { _setTelemetryAvailable(configured); }

    static void _update(Fact& fact, const QVariant& value);
    static void _restoreDefaults(std::initializer_list<Fact*> facts);
    void _setCurrentBasePosition(const std::optional<GPSBaseStationConfig::Fixed>& base);
    void _setCurrentBasePositionSaved(bool saved);
    void _updateReceiverWarning();

    std::optional<GPSBaseStationConfig::Fixed> _currentBasePosition;
    bool _currentBasePositionSaved = false;
    bool _receiverWarning = false;

    Fact _currentDurationFact = Fact(0, QStringLiteral("currentDuration"), FactMetaData::valueTypeDouble);
    Fact _currentAccuracyFact = Fact(0, QStringLiteral("currentAccuracy"), FactMetaData::valueTypeDouble);
    Fact _validFact = Fact(0, QStringLiteral("valid"), FactMetaData::valueTypeBool);
    Fact _activeFact = Fact(0, QStringLiteral("active"), FactMetaData::valueTypeBool);
    Fact _numSatellitesFact = Fact(0, QStringLiteral("numSatellites"), FactMetaData::valueTypeInt32);
    Fact _numSatellitesUsedFact = Fact(0, QStringLiteral("numSatellitesUsed"), FactMetaData::valueTypeInt32);
    Fact _fixTypeFact = Fact(0, QStringLiteral("fixType"), FactMetaData::valueTypeUint32);
    Fact _jammingStateFact = Fact(0, QStringLiteral("jammingState"), FactMetaData::valueTypeUint8);
    Fact _spoofingStateFact = Fact(0, QStringLiteral("spoofingState"), FactMetaData::valueTypeUint8);
    Fact _antennaStateFact = Fact(0, QStringLiteral("antennaState"), FactMetaData::valueTypeUint8);
};
