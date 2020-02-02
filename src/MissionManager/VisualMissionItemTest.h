/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "UnitTest.h"
#include "TCPLink.h"
#include "MultiSignalSpy.h"
#include "SimpleMissionItem.h"

#include <QGeoCoordinate>

/// Unit test for SimpleMissionItem
class VisualMissionItemTest : public UnitTest
{
    Q_OBJECT

public:
    VisualMissionItemTest(void);

    void init(void) override;
    void cleanup(void) override;

protected:
    void _createSpy(SimpleMissionItem* simpleItem, MultiSignalSpy** visualSpy);

    enum {
        altDifferenceChangedIndex = 0,
        altPercentChangedIndex,
        azimuthChangedIndex,
        commandDescriptionChangedIndex,
        commandNameChangedIndex,
        abbreviationChangedIndex,
        coordinateChangedIndex,
        exitCoordinateChangedIndex,
        dirtyChangedIndex,
        distanceChangedIndex,
        isCurrentItemChangedIndex,
        sequenceNumberChangedIndex,
        isSimpleItemChangedIndex,
        specifiesCoordinateChangedIndex,
        isStandaloneCoordinateChangedIndex,
        specifiesAltitudeOnlyChangedIndex,
        specifiedFlightSpeedChangedIndex,
        specifiedGimbalYawChangedIndex,
        specifiedGimbalPitchChangedIndex,
        lastSequenceNumberChangedIndex,
        missionGimbalYawChangedIndex,
        missionVehicleYawChangedIndex,
        coordinateHasRelativeAltitudeChangedIndex,
        exitCoordinateHasRelativeAltitudeChangedIndex,
        exitCoordinateSameAsEntryChangedIndex,
        maxSignalIndex,
    };

    enum {
        altDifferenceChangedMask =                      1 << altDifferenceChangedIndex,
        altPercentChangedMask =                         1 << altPercentChangedIndex,
        azimuthChangedMask =                            1 << azimuthChangedIndex,
        commandDescriptionChangedMask =                 1 << commandDescriptionChangedIndex,
        commandNameChangedMask =                        1 << commandNameChangedIndex,
        abbreviationChangedMask =                       1 << abbreviationChangedIndex,
        coordinateChangedMask =                         1 << coordinateChangedIndex,
        exitCoordinateChangedMask =                     1 << exitCoordinateChangedIndex,
        dirtyChangedMask =                              1 << dirtyChangedIndex,
        distanceChangedMask =                           1 << distanceChangedIndex,
        isCurrentItemChangedMask =                      1 << isCurrentItemChangedIndex,
        sequenceNumberChangedMask =                     1 << sequenceNumberChangedIndex,
        isSimpleItemChangedMask =                       1 << isSimpleItemChangedIndex,
        specifiesCoordinateChangedMask =                1 << specifiesCoordinateChangedIndex,
        isStandaloneCoordinateChangedMask =             1 << isStandaloneCoordinateChangedIndex,
        specifiesAltitudeOnlyChangedMask =              1 << specifiesAltitudeOnlyChangedIndex,
        specifiedFlightSpeedChangedMask =               1 << specifiedFlightSpeedChangedIndex,
        specifiedGimbalYawChangedMask =                 1 << specifiedGimbalYawChangedIndex,
        specifiedGimbalPitchChangedMask =               1 << specifiedGimbalPitchChangedIndex,
        lastSequenceNumberChangedMask =                 1 << lastSequenceNumberChangedIndex,
        missionGimbalYawChangedMask =                   1 << missionGimbalYawChangedIndex,
        missionVehicleYawChangedMask =                  1 << missionVehicleYawChangedIndex,
        coordinateHasRelativeAltitudeChangedMask =      1 << coordinateHasRelativeAltitudeChangedIndex,
        exitCoordinateHasRelativeAltitudeChangedMask =  1 << exitCoordinateHasRelativeAltitudeChangedIndex,
        exitCoordinateSameAsEntryChangedMask =          1 << exitCoordinateSameAsEntryChangedIndex,
    };

    static const size_t cVisualItemSignals = maxSignalIndex;
    const char*         rgVisualItemSignals[cVisualItemSignals];

    Vehicle*        _offlineVehicle;
};
