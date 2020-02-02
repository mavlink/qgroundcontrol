/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "VisualMissionItemTest.h"
#include "SimpleMissionItem.h"
#include "QGCApplication.h"

VisualMissionItemTest::VisualMissionItemTest(void)
    : _offlineVehicle(nullptr)
{
    
}

void VisualMissionItemTest::init(void)
{
    UnitTest::init();
    _offlineVehicle = new Vehicle(MAV_AUTOPILOT_PX4,
                                  MAV_TYPE_QUADROTOR,
                                  qgcApp()->toolbox()->firmwarePluginManager(),
                                  this);

    rgVisualItemSignals[altDifferenceChangedIndex] =                        SIGNAL(altDifferenceChanged(double));
    rgVisualItemSignals[altPercentChangedIndex] =                           SIGNAL(altPercentChanged(double));
    rgVisualItemSignals[azimuthChangedIndex] =                              SIGNAL(azimuthChanged(double));
    rgVisualItemSignals[commandDescriptionChangedIndex] =                   SIGNAL(commandDescriptionChanged());
    rgVisualItemSignals[commandNameChangedIndex] =                          SIGNAL(commandNameChanged());
    rgVisualItemSignals[abbreviationChangedIndex] =                         SIGNAL(abbreviationChanged());
    rgVisualItemSignals[coordinateChangedIndex] =                           SIGNAL(coordinateChanged(const QGeoCoordinate&));
    rgVisualItemSignals[exitCoordinateChangedIndex] =                       SIGNAL(exitCoordinateChanged(const QGeoCoordinate&));
    rgVisualItemSignals[dirtyChangedIndex] =                                SIGNAL(dirtyChanged(bool));
    rgVisualItemSignals[distanceChangedIndex] =                             SIGNAL(distanceChanged(double));
    rgVisualItemSignals[isCurrentItemChangedIndex] =                        SIGNAL(isCurrentItemChanged(bool));
    rgVisualItemSignals[sequenceNumberChangedIndex] =                       SIGNAL(sequenceNumberChanged(int));
    rgVisualItemSignals[isSimpleItemChangedIndex] =                         SIGNAL(isSimpleItemChanged(bool));
    rgVisualItemSignals[specifiesCoordinateChangedIndex] =                  SIGNAL(specifiesCoordinateChanged());
    rgVisualItemSignals[isStandaloneCoordinateChangedIndex] =               SIGNAL(isStandaloneCoordinateChanged());
    rgVisualItemSignals[specifiesAltitudeOnlyChangedIndex] =                SIGNAL(specifiesAltitudeOnlyChanged());
    rgVisualItemSignals[specifiedFlightSpeedChangedIndex] =                 SIGNAL(specifiedFlightSpeedChanged());
    rgVisualItemSignals[specifiedGimbalYawChangedIndex] =                   SIGNAL(specifiedGimbalYawChanged());
    rgVisualItemSignals[specifiedGimbalPitchChangedIndex] =                 SIGNAL(specifiedGimbalPitchChanged());
    rgVisualItemSignals[lastSequenceNumberChangedIndex] =                   SIGNAL(lastSequenceNumberChanged(int));
    rgVisualItemSignals[missionGimbalYawChangedIndex] =                     SIGNAL(missionGimbalYawChanged(double));
    rgVisualItemSignals[missionVehicleYawChangedIndex] =                    SIGNAL(missionVehicleYawChanged(double));
    rgVisualItemSignals[coordinateHasRelativeAltitudeChangedIndex] =        SIGNAL(coordinateHasRelativeAltitudeChanged(bool));
    rgVisualItemSignals[exitCoordinateHasRelativeAltitudeChangedIndex] =    SIGNAL(exitCoordinateHasRelativeAltitudeChanged(bool));
    rgVisualItemSignals[exitCoordinateSameAsEntryChangedIndex] =            SIGNAL(exitCoordinateSameAsEntryChanged(bool));
}

void VisualMissionItemTest::cleanup(void)
{
    _offlineVehicle->deleteLater();
    UnitTest::cleanup();
}

void VisualMissionItemTest::_createSpy(SimpleMissionItem* simpleItem, MultiSignalSpy** visualSpy)
{
    *visualSpy = nullptr;
    MultiSignalSpy* spy = new MultiSignalSpy();
    QCOMPARE(spy->init(simpleItem, rgVisualItemSignals, cVisualItemSignals), true);
    *visualSpy = spy;
}
