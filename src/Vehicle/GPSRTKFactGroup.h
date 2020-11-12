/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#pragma once

#include "Vehicle.h"

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

    static const char* _connectedFactName;
    static const char* _currentDurationFactName;
    static const char* _currentAccuracyFactName;
    static const char* _currentLatitudeFactName;
    static const char* _currentLongitudeFactName;
    static const char* _currentAltitudeFactName;
    static const char* _validFactName;
    static const char* _activeFactName;
    static const char* _numSatellitesFactName;

private:
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
