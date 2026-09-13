#pragma once

#include "FactGroup.h"

class GPSBaseStationFactGroup : public FactGroup
{
    Q_OBJECT
    Q_PROPERTY(Fact* currentDuration READ currentDuration CONSTANT)
    Q_PROPERTY(Fact* currentAccuracy READ currentAccuracy CONSTANT)
    Q_PROPERTY(Fact* currentLatitude READ currentLatitude CONSTANT)
    Q_PROPERTY(Fact* currentLongitude READ currentLongitude CONSTANT)
    Q_PROPERTY(Fact* currentAltitude READ currentAltitude CONSTANT)
    Q_PROPERTY(Fact* valid READ valid CONSTANT)
    Q_PROPERTY(Fact* active READ active CONSTANT)

public:
    explicit GPSBaseStationFactGroup(QObject* parent = nullptr);
    ~GPSBaseStationFactGroup() override;

    Fact* currentDuration() { return &_currentDurationFact; }

    Fact* currentAccuracy() { return &_currentAccuracyFact; }

    Fact* currentLatitude() { return &_currentLatitudeFact; }

    Fact* currentLongitude() { return &_currentLongitudeFact; }

    Fact* currentAltitude() { return &_currentAltitudeFact; }

    Fact* valid() { return &_validFact; }

    Fact* active() { return &_activeFact; }

private:
    Fact _currentDurationFact = Fact(0, QStringLiteral("currentDuration"), FactMetaData::valueTypeDouble);
    Fact _currentAccuracyFact = Fact(0, QStringLiteral("currentAccuracy"), FactMetaData::valueTypeDouble);
    Fact _currentLatitudeFact = Fact(0, QStringLiteral("currentLatitude"), FactMetaData::valueTypeDouble);
    Fact _currentLongitudeFact = Fact(0, QStringLiteral("currentLongitude"), FactMetaData::valueTypeDouble);
    Fact _currentAltitudeFact = Fact(0, QStringLiteral("currentAltitude"), FactMetaData::valueTypeFloat);
    Fact _validFact = Fact(0, QStringLiteral("valid"), FactMetaData::valueTypeBool);
    Fact _activeFact = Fact(0, QStringLiteral("active"), FactMetaData::valueTypeBool);
};
