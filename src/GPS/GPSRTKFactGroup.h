/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QLoggingCategory>

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

public:
    explicit GPSRTKFactGroup(QObject *parent = nullptr);
    ~GPSRTKFactGroup();

    Fact *connected() { return &_connectedFact; }
    Fact *currentDuration() { return &_currentDurationFact; }
    Fact *currentAccuracy() { return &_currentAccuracyFact; }
    Fact *currentLatitude() { return &_currentLatitudeFact; }
    Fact *currentLongitude() { return &_currentLongitudeFact; }
    Fact *currentAltitude()  { return &_currentAltitudeFact; }
    Fact *valid() { return &_validFact; }
    Fact *active() { return &_activeFact; }
    Fact *numSatellites() { return &_numSatellitesFact; }

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
};
