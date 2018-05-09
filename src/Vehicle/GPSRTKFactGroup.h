/****************************************************************************
 *
 *   (c) 2017 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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
    GPSRTKFactGroup(QObject* parent = NULL);

    Q_PROPERTY(Fact* connected            READ connected            CONSTANT)
    Q_PROPERTY(Fact* currentDuration      READ currentDuration      CONSTANT)
    Q_PROPERTY(Fact* currentAccuracy      READ currentAccuracy      CONSTANT)
    Q_PROPERTY(Fact* valid                READ valid                CONSTANT)
    Q_PROPERTY(Fact* active               READ active               CONSTANT)
    Q_PROPERTY(Fact* numSatellites        READ numSatellites        CONSTANT)

    Fact* connected                    (void) { return &_connected; }
    Fact* currentDuration              (void) { return &_currentDuration; }
    Fact* currentAccuracy              (void) { return &_currentAccuracy; }
    Fact* valid                        (void) { return &_valid; }
    Fact* active                       (void) { return &_active; }
    Fact* numSatellites                (void) { return &_numSatellites; }

    static const char* _connectedFactName;
    static const char* _currentDurationFactName;
    static const char* _currentAccuracyFactName;
    static const char* _validFactName;
    static const char* _activeFactName;
    static const char* _numSatellitesFactName;

private:
    Fact        _connected; ///< is an RTK gps connected?
    Fact        _currentDuration; ///< survey-in status in [s]
    Fact        _currentAccuracy; ///< survey-in accuracy in [mm]
    Fact        _valid; ///< survey-in valid?
    Fact        _active; ///< survey-in active?
    Fact        _numSatellites; ///< number of satellites
};
