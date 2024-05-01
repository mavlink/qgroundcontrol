/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#pragma once

#include "FactGroup.h"

class GPSRTKFactGroup : public FactGroup
{
    Q_OBJECT

public:
    GPSRTKFactGroup(QObject* parent = nullptr);

    Q_PROPERTY(Fact* connected            READ connected            CONSTANT)
    Q_PROPERTY(Fact* currentDuration      READ currentDuration      CONSTANT)
    Q_PROPERTY(Fact* currentAccuracy      READ currentAccuracy      CONSTANT)
    Q_PROPERTY(Fact* currentLatitude      READ currentLatitude      CONSTANT)
    Q_PROPERTY(Fact* currentLongitude     READ currentLongitude      CONSTANT)
    Q_PROPERTY(Fact* currentAltitude      READ currentAltitude      CONSTANT)
    Q_PROPERTY(Fact* valid                READ valid                CONSTANT)
    Q_PROPERTY(Fact* active               READ active               CONSTANT)
    Q_PROPERTY(Fact* numSatellites        READ numSatellites        CONSTANT)

    Fact* connected         (void) { return &_connected; }
    Fact* currentDuration   (void) { return &_currentDuration; }
    Fact* currentAccuracy   (void) { return &_currentAccuracy; }
    Fact* currentLatitude   (void) { return &_currentLatitude; }
    Fact* currentLongitude  (void) { return &_currentLongitude; }
    Fact* currentAltitude   (void) { return &_currentAltitude; }
    Fact* valid             (void) { return &_valid; }
    Fact* active            (void) { return &_active; }
    Fact* numSatellites     (void) { return &_numSatellites; }

private:
    const QString _connectedFactName =                QStringLiteral("connected");
    const QString _currentAccuracyFactName =          QStringLiteral("currentAccuracy");
    const QString _currentDurationFactName =          QStringLiteral("currentDuration");
    const QString _currentLatitudeFactName =          QStringLiteral("currentLatitude");
    const QString _currentLongitudeFactName =         QStringLiteral("currentLongitude");
    const QString _currentAltitudeFactName =          QStringLiteral("currentAltitude");
    const QString _validFactName =                    QStringLiteral("valid");
    const QString _activeFactName =                   QStringLiteral("active");
    const QString _numSatellitesFactName =            QStringLiteral("numSatellites");

    Fact _connected;        ///< is an RTK gps connected?
    Fact _currentDuration;  ///< survey-in status in [s]
    Fact _currentAccuracy;  ///< survey-in accuracy in [mm]
    Fact _currentLatitude;  ///< survey-in latitude
    Fact _currentLongitude; ///< survey-in latitude
    Fact _currentAltitude;  ///< survey-in latitude
    Fact _valid;            ///< survey-in complete?
    Fact _active;           ///< survey-in active?
    Fact _numSatellites;    ///< number of satellites
};
