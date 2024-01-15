/****************************************************************************
 *
 * (c) 2022 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "HealthAndArmingCheckReport.h"
#include "QGCMAVLink.h"

#include <libevents/libs/cpp/generated/events_generated.h>

HealthAndArmingCheckReport::HealthAndArmingCheckReport()
{
#if 0 // to test the UI
    _problemsForCurrentMode->append(new HealthAndArmingCheckProblem("No global position", "", "error"));
    _problemsForCurrentMode->append(new HealthAndArmingCheckProblem("No RC", "Details", "warning"));
    _problemsForCurrentMode->append(new HealthAndArmingCheckProblem("Accel uncalibrated", "Details <a href='www.test.com'>test</a>", "error"));
    _problemsForCurrentMode->append(new HealthAndArmingCheckProblem("Gyro uncalibrated", "Details <a href='param://SDLOG_PROFILE'>SDLOG_PROFILE</a>", "error"));
    _problemsForCurrentMode->append(new HealthAndArmingCheckProblem(
            "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat.",
            "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur.<br>Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum", ""));
    _supported = true;
    emit updated();
#endif
}

HealthAndArmingCheckReport::~HealthAndArmingCheckReport()
{
    _problemsForCurrentMode->clearAndDeleteContents();
}

void HealthAndArmingCheckReport::update(uint8_t compid, const events::HealthAndArmingChecks::Results& results,
        int flightModeGroup)
{
    if (compid != MAV_COMP_ID_AUTOPILOT1) {
        // only autopilot supported atm
        return;
    }
    if (flightModeGroup == -1) {
        qWarning() << "Flight mode group not set";
        return;
    }
    _supported = true;

    _problemsForCurrentMode->clearAndDeleteContents();
    _hasWarningsOrErrors = false;
    for (const auto& check : results.checks(flightModeGroup)) {
        QString severity = "";
        if (events::externalLogLevel(check.log_levels) <= events::Log::Error) {
            severity = "error";
            _hasWarningsOrErrors = true;
        } else if (events::externalLogLevel(check.log_levels) <= events::Log::Warning) {
            severity = "warning";
            _hasWarningsOrErrors = true;
        }
        QString description = QString::fromStdString(check.description);
        _problemsForCurrentMode->append(new HealthAndArmingCheckProblem(QString::fromStdString(check.message),
                description.replace("\n", "<br/>"), severity));
    }

    _canArm = results.canArm(flightModeGroup);
    if (_missionModeGroup != -1) {
        // TODO: use results.canRun(_missionModeGroup) while armed
        _canStartMission = results.canArm(_missionModeGroup);
    }
    if (_takeoffModeGroup != -1) {
        _canTakeoff = results.canArm(_takeoffModeGroup);
    }

    const auto& healthComponents = results.healthComponents().health_components;

    // GPS state
    const auto gpsStateIter = healthComponents.find("gps");
    if (gpsStateIter != healthComponents.end()) {
        const events::HealthAndArmingChecks::HealthComponent& gpsState = gpsStateIter->second;
        if (gpsState.health.error || gpsState.arming_check.error) {
            _gpsState = "red";
        } else if (gpsState.health.warning || gpsState.arming_check.warning) {
            _gpsState = "yellow";
        } else {
            _gpsState = "green";
        }
    }

    emit updated();
}

void HealthAndArmingCheckReport::setModeGroups(int takeoffModeGroup, int missionModeGroup)
{
    _takeoffModeGroup = takeoffModeGroup;
    _missionModeGroup = missionModeGroup;
}
