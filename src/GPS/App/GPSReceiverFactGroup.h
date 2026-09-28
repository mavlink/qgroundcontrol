#pragma once

#include <QtQmlIntegration/QtQmlIntegration>

#include "FactGroup.h"
#include "GPSReceiver.h"

/// Facts of the local receiver's status for QML, mirrored from GPSReceiver::statusChanged.
class GPSReceiverFactGroup : public FactGroup
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Provided by GPSManager")
    Q_PROPERTY(Fact* connected READ connected CONSTANT FINAL)
    Q_PROPERTY(Fact* currentDuration READ currentDuration CONSTANT FINAL)
    Q_PROPERTY(Fact* currentAccuracy READ currentAccuracy CONSTANT FINAL)
    Q_PROPERTY(Fact* currentLatitude READ currentLatitude CONSTANT FINAL)
    Q_PROPERTY(Fact* currentLongitude READ currentLongitude CONSTANT FINAL)
    Q_PROPERTY(Fact* currentAltitude READ currentAltitude CONSTANT FINAL)
    Q_PROPERTY(Fact* valid READ valid CONSTANT FINAL)
    Q_PROPERTY(Fact* active READ active CONSTANT FINAL)
    Q_PROPERTY(Fact* numSatellites READ numSatellites CONSTANT FINAL)
    Q_PROPERTY(Fact* numSatellitesUsed READ numSatellitesUsed CONSTANT FINAL)
    Q_PROPERTY(Fact* fixType READ fixType CONSTANT FINAL)
    Q_PROPERTY(Fact* jammingState READ jammingState CONSTANT FINAL)
    Q_PROPERTY(Fact* spoofingState READ spoofingState CONSTANT FINAL)
    Q_PROPERTY(bool canSaveCurrentBasePosition READ canSaveCurrentBasePosition NOTIFY currentBasePositionChanged FINAL)
    Q_PROPERTY(bool interferenceWarning READ interferenceWarning NOTIFY interferenceWarningChanged FINAL)
    /// Short state for compact views: a configured base's survey state, otherwise the fix type.
    Q_PROPERTY(QString summaryLabel READ summaryLabel NOTIFY summaryLabelChanged FINAL)

public:
    /// Follows @a receiver's status when one is given.
    explicit GPSReceiverFactGroup(const GPSReceiver* receiver = nullptr, QObject* parent = nullptr);
    ~GPSReceiverFactGroup();

    Fact* connected() { return &_connectedFact; }

    Fact* currentDuration() { return &_currentDurationFact; }

    Fact* currentAccuracy() { return &_currentAccuracyFact; }

    Fact* currentLatitude() { return &_currentLatitudeFact; }

    Fact* currentLongitude() { return &_currentLongitudeFact; }

    Fact* currentAltitude() { return &_currentAltitudeFact; }

    Fact* valid() { return &_validFact; }

    Fact* active() { return &_activeFact; }

    Fact* numSatellites() { return &_numSatellitesFact; }

    Fact* numSatellitesUsed() { return &_numSatellitesUsedFact; }

    Fact* fixType() { return &_fixTypeFact; }

    Fact* jammingState() { return &_jammingStateFact; }

    Fact* spoofingState() { return &_spoofingStateFact; }

    /// A valid receiver status alone does not establish usable coordinates or accuracy.
    bool canSaveCurrentBasePosition() const;

    /// Jamming at Warning or Critical, or any spoofing indication.
    bool interferenceWarning() const;

    QString summaryLabel() const { return _summaryLabel; }

signals:
    void currentBasePositionChanged();
    void interferenceWarningChanged();
    void summaryLabelChanged();

private:
    // By value: a Fact observer may reconfigure the receiver while the Facts update; each pass mirrors one status.
    void _mirror(GPSReceiver::Status status);
    void _updateSummaryLabel();

    const GPSReceiver* const _receiver;
    QString _summaryLabel;

    Fact _connectedFact = Fact(0, QStringLiteral("connected"), FactMetaData::valueTypeBool);
    Fact _currentDurationFact = Fact(0, QStringLiteral("currentDuration"), FactMetaData::valueTypeDouble);
    Fact _currentAccuracyFact = Fact(0, QStringLiteral("currentAccuracy"), FactMetaData::valueTypeDouble);
    Fact _currentLatitudeFact = Fact(0, QStringLiteral("currentLatitude"), FactMetaData::valueTypeDouble);
    Fact _currentLongitudeFact = Fact(0, QStringLiteral("currentLongitude"), FactMetaData::valueTypeDouble);
    Fact _currentAltitudeFact = Fact(0, QStringLiteral("currentAltitude"), FactMetaData::valueTypeFloat);
    Fact _validFact = Fact(0, QStringLiteral("valid"), FactMetaData::valueTypeBool);
    Fact _activeFact = Fact(0, QStringLiteral("active"), FactMetaData::valueTypeBool);
    Fact _numSatellitesFact = Fact(0, QStringLiteral("numSatellites"), FactMetaData::valueTypeInt32);
    Fact _numSatellitesUsedFact = Fact(0, QStringLiteral("numSatellitesUsed"), FactMetaData::valueTypeInt32);
    Fact _fixTypeFact = Fact(0, QStringLiteral("fixType"), FactMetaData::valueTypeUint32);
    Fact _jammingStateFact = Fact(0, QStringLiteral("jammingState"), FactMetaData::valueTypeUint8);
    Fact _spoofingStateFact = Fact(0, QStringLiteral("spoofingState"), FactMetaData::valueTypeUint8);
};
