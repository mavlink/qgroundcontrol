#pragma once

#include <QtCore/QLoggingCategory>
#include <QtPositioning/QGeoCoordinate>

#include "FactGroup.h"

Q_DECLARE_LOGGING_CATEGORY(GPSRTKFactGroupLog)

class GPSRTKFactGroup : public FactGroup
{
    Q_OBJECT
    Q_PROPERTY(Fact *connected          READ connected          CONSTANT)
    Q_PROPERTY(Fact *currentDuration    READ currentDuration    CONSTANT)
    Q_PROPERTY(Fact *currentAccuracy    READ currentAccuracy    CONSTANT)
    Q_PROPERTY(Fact *currentLatitude    READ currentLatitude    CONSTANT)
    Q_PROPERTY(Fact *currentLongitude   READ currentLongitude   CONSTANT)
    Q_PROPERTY(Fact *currentAltitude    READ currentAltitude    CONSTANT)
    Q_PROPERTY(Fact *valid              READ valid              CONSTANT)
    Q_PROPERTY(Fact *active             READ active             CONSTANT)
    Q_PROPERTY(Fact *numSatellites      READ numSatellites      CONSTANT)
    Q_PROPERTY(Fact *baseLatitude       READ baseLatitude       CONSTANT)
    Q_PROPERTY(Fact *baseLongitude      READ baseLongitude      CONSTANT)
    Q_PROPERTY(Fact *baseAltitude       READ baseAltitude       CONSTANT)
    Q_PROPERTY(Fact *baseFixType        READ baseFixType        CONSTANT)
    Q_PROPERTY(Fact *heading            READ heading            CONSTANT)
    Q_PROPERTY(Fact *headingValid       READ headingValid       CONSTANT)
    Q_PROPERTY(Fact *baselineLength     READ baselineLength     CONSTANT)
    Q_PROPERTY(Fact *carrierFixed       READ carrierFixed       CONSTANT)

public:
    explicit GPSRTKFactGroup(QObject *parent = nullptr);
    ~GPSRTKFactGroup() override = default;

    Fact *connected() { return &_connectedFact; }
    Fact *currentDuration() { return &_currentDurationFact; }
    Fact *currentAccuracy() { return &_currentAccuracyFact; }
    Fact *currentLatitude() { return &_currentLatitudeFact; }
    Fact *currentLongitude() { return &_currentLongitudeFact; }
    Fact *currentAltitude() { return &_currentAltitudeFact; }
    Fact *valid() { return &_validFact; }
    Fact *active() { return &_activeFact; }
    Fact *numSatellites() { return &_numSatellitesFact; }
    Fact *baseLatitude() { return &_baseLatitudeFact; }
    Fact *baseLongitude() { return &_baseLongitudeFact; }
    Fact *baseAltitude() { return &_baseAltitudeFact; }
    Fact *baseFixType() { return &_baseFixTypeFact; }
    Fact *heading() { return &_headingFact; }
    Fact *headingValid() { return &_headingValidFact; }
    Fact *baselineLength() { return &_baselineLengthFact; }
    Fact *carrierFixed() { return &_carrierFixedFact; }

    /// Assembles lat/lon/alt Facts into a QGeoCoordinate for C++ callers.
    /// Returns an invalid coordinate if any component is non-finite.
    QGeoCoordinate currentPosition() const;
    QGeoCoordinate basePosition() const;

    void resetToDefaults();

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
    Fact _baseLatitudeFact = Fact(0, QStringLiteral("baseLatitude"), FactMetaData::valueTypeDouble);
    Fact _baseLongitudeFact = Fact(0, QStringLiteral("baseLongitude"), FactMetaData::valueTypeDouble);
    Fact _baseAltitudeFact = Fact(0, QStringLiteral("baseAltitude"), FactMetaData::valueTypeDouble);
    Fact _baseFixTypeFact = Fact(0, QStringLiteral("baseFixType"), FactMetaData::valueTypeUint8);
    Fact _headingFact = Fact(0, QStringLiteral("heading"), FactMetaData::valueTypeDouble);
    Fact _headingValidFact = Fact(0, QStringLiteral("headingValid"), FactMetaData::valueTypeBool);
    Fact _baselineLengthFact = Fact(0, QStringLiteral("baselineLength"), FactMetaData::valueTypeDouble);
    Fact _carrierFixedFact = Fact(0, QStringLiteral("carrierFixed"), FactMetaData::valueTypeBool);
};
