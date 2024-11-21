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
    explicit GPSRTKFactGroup(QObject* parent = nullptr);
    ~GPSRTKFactGroup();

    Fact *connected() const { return _connected; }
    Fact *currentDuration() const { return _currentDuration; }
    Fact *currentAccuracy() const { return _currentAccuracy; }
    Fact *currentLatitude() const { return _currentLatitude; }
    Fact *currentLongitude() const { return _currentLongitude; }
    Fact *currentAltitude() const { return _currentAltitude; }
    Fact *valid() const { return _valid; }
    Fact *active() const { return _active; }
    Fact *numSatellites() const { return _numSatellites; }

private:
    const QString _connectedFactName = QStringLiteral("connected");
    const QString _currentAccuracyFactName = QStringLiteral("currentAccuracy");
    const QString _currentDurationFactName = QStringLiteral("currentDuration");
    const QString _currentLatitudeFactName = QStringLiteral("currentLatitude");
    const QString _currentLongitudeFactName = QStringLiteral("currentLongitude");
    const QString _currentAltitudeFactName = QStringLiteral("currentAltitude");
    const QString _validFactName = QStringLiteral("valid");
    const QString _activeFactName = QStringLiteral("active");
    const QString _numSatellitesFactName = QStringLiteral("numSatellites");

    Fact *_connected = nullptr;        ///< is an RTK gps connected?
    Fact *_currentDuration = nullptr;  ///< survey-in status in [s]
    Fact *_currentAccuracy = nullptr;  ///< survey-in accuracy in [mm]
    Fact *_currentLatitude = nullptr;  ///< survey-in latitude
    Fact *_currentLongitude = nullptr; ///< survey-in latitude
    Fact *_currentAltitude = nullptr;  ///< survey-in latitude
    Fact *_valid = nullptr;            ///< survey-in complete?
    Fact *_active = nullptr;           ///< survey-in active?
    Fact *_numSatellites = nullptr;    ///< number of satellites
};
