#pragma once

#include "FactGroup.h"

class GPSRTKFactGroup : public FactGroup
{
    Q_OBJECT
    Q_PROPERTY(Fact* connected READ connected CONSTANT)
    Q_PROPERTY(Fact* currentDuration READ currentDuration CONSTANT)
    Q_PROPERTY(Fact* currentAccuracy READ currentAccuracy CONSTANT)
    Q_PROPERTY(Fact* currentLatitude READ currentLatitude CONSTANT)
    Q_PROPERTY(Fact* currentLongitude READ currentLongitude CONSTANT)
    Q_PROPERTY(Fact* currentAltitude READ currentAltitude CONSTANT)
    Q_PROPERTY(Fact* valid READ valid CONSTANT)
    Q_PROPERTY(Fact* active READ active CONSTANT)
    Q_PROPERTY(Fact* numSatellites READ numSatellites CONSTANT)
    Q_PROPERTY(Fact* numSatellitesUsed READ numSatellitesUsed CONSTANT)
    Q_PROPERTY(Fact* fixType READ fixType CONSTANT)
    Q_PROPERTY(Fact* jammingState READ jammingState CONSTANT)
    Q_PROPERTY(Fact* spoofingState READ spoofingState CONSTANT)
    Q_PROPERTY(bool canSaveCurrentBasePosition READ canSaveCurrentBasePosition NOTIFY currentBasePositionChanged)
    Q_PROPERTY(bool interferenceWarning READ interferenceWarning NOTIFY interferenceWarningChanged)

public:
    explicit GPSRTKFactGroup(QObject* parent = nullptr);
    ~GPSRTKFactGroup();

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

signals:
    void currentBasePositionChanged();
    void interferenceWarningChanged();

private:
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
