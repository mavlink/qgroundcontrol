.pragma library

// Row definitions for PX4FailsafesComponent.qml, in the PX4ConfigRows.js schema plus:
// Row:  warnOnly?   the failsafe only warns
// Cell: severity?   marks an action. A string for a fixed action, or a map of the parameter's raw values to a
//                   severity: none, warning, fallback, hold, return, land or terminate

// Shared by the parameters that use the same None/Warning/Hold/Return/Terminate/Land choices
var _commonActions = { "0": "none", "1": "warning", "2": "hold", "3": "return", "4": "terminate", "5": "land" }

function failsafeRows() {
    return [
        {
            id: "battery", group: qsTr("Power"), title: qsTr("Battery"),
            description: qsTr("Three levels. The action applies at critical and emergency."),
            cells: [
                { thresholdBar: ["BAT_EMERGEN_THR", "BAT_CRIT_THR", "BAT_LOW_THR"], label: qsTr("Thresholds") },
                { param: "COM_LOW_BAT_ACT", label: qsTr("Action"),
                  severity: { "0": "warning", "1": "return", "2": "land", "3": "return" } },
                { param: "BAT_LOW_THR", label: qsTr("Low level") },
                { param: "BAT_CRIT_THR", label: qsTr("Critical level") },
                { param: "BAT_EMERGEN_THR", label: qsTr("Emergency level") },
                { param: "COM_ARM_BAT_MIN", label: qsTr("Min. battery to arm"),
                  note: qsTr("Negative uses the critical level.") },
                { fixed: qsTr("Warning"), label: qsTr("Low level action"), severity: "warning",
                  note: qsTr("Always a warning.") }
            ]
        },
        {
            id: "remainingFlightTime", group: qsTr("Power"), title: qsTr("Remaining flight time"),
            description: qsTr("Remaining battery time is less than the time needed to return."),
            activeWhen: [{ param: "COM_FLTT_LOW_ACT", op: "gt", value: 0 }],
            cells: [
                { param: "COM_FLTT_LOW_ACT", label: qsTr("Action"),
                  severity: { "0": "none", "1": "warning", "3": "return" } },
                { param: "RTL_TIME_FACTOR", label: qsTr("Estimate safety factor") },
                { param: "RTL_TIME_MARGIN", label: qsTr("Estimate margin") }
            ]
        },
        {
            id: "maxFlightTime", group: qsTr("Power"), title: qsTr("Maximum flight time"),
            description: qsTr("Time since takeoff exceeds the limit. A warning is sent from 90% of the limit."),
            activeWhen: [{ param: "COM_FLT_TIME_MAX", op: "gt", value: 0 }],
            cells: [
                { param: "COM_FLT_TIME_MAX", label: qsTr("Limit"), note: qsTr("0 or negative disables.") },
                { fixed: qsTr("Return"), label: qsTr("Action"), severity: "return" }
            ]
        },
        {
            id: "batteryUnhealthy", group: qsTr("Power"), title: qsTr("Battery unhealthy"),
            description: qsTr("Battery reports a fault. Disarms if it happens during spool-up after arming."),
            warnOnly: true,
            cells: [
                { param: "COM_SPOOLUP_TIME", label: qsTr("Spool-up time") },
                { fixed: qsTr("Warning"), label: qsTr("Action"), severity: "warning" },
                { fixed: qsTr("Disarm"), label: qsTr("During spool-up"), severity: "terminate" }
            ]
        },

        {
            id: "manualControl", group: qsTr("Link Loss"), title: qsTr("Manual control loss"),
            description: qsTr("RC receiver or MAVLink joystick stops sending sticks."),
            activeWhen: [{ param: "COM_RC_IN_MODE", op: "ne", value: 4 }],
            cells: [
                { param: "COM_RC_LOSS_T", label: qsTr("Timeout") },
                { param: "NAV_RCL_ACT", label: qsTr("Action"),
                  severity: { "1": "hold", "2": "return", "3": "land", "5": "terminate", "6": "terminate" } },
                { param: "COM_RCL_EXCEPT", label: qsTr("Ignore loss in") },
                { param: "COM_RC_IN_MODE", label: qsTr("Manual control source") }
            ]
        },
        {
            id: "dataLink", group: qsTr("Link Loss"), title: qsTr("Data link (GCS) loss"),
            description: qsTr("No MAVLink heartbeat from a ground station."),
            activeWhen: [{ param: "NAV_DLL_ACT", op: "gt", value: 0 }],
            cells: [
                { param: "COM_DL_LOSS_T", label: qsTr("Timeout") },
                { param: "NAV_DLL_ACT", label: qsTr("Action"),
                  severity: { "0": "none", "1": "hold", "2": "return", "3": "land", "5": "terminate", "6": "terminate" } },
                { param: "COM_DLL_EXCEPT", label: qsTr("Ignore loss in") },
                { param: "COM_HLDL_LOSS_T", label: qsTr("High-latency link timeout") },
                { param: "COM_HLDL_REG_T", label: qsTr("High-latency link regain time") }
            ]
        },
        {
            id: "offboard", group: qsTr("Link Loss"), title: qsTr("Offboard control loss"),
            description: qsTr("Companion computer stops sending offboard setpoints."),
            cells: [
                { param: "COM_OF_LOSS_T", label: qsTr("Timeout") },
                { param: "COM_OBL_RC_ACT", label: qsTr("Action"),
                  severity: { "0": "fallback", "1": "fallback", "2": "fallback", "3": "return", "4": "land", "5": "hold",
                              "6": "terminate", "7": "terminate" } }
            ]
        },
        {
            id: "onboardComputer", group: qsTr("Link Loss"), title: qsTr("Onboard computer loss"),
            description: qsTr("Companion computer connection is lost."), warnOnly: true,
            cells: [
                { param: "COM_OBC_LOSS_T", label: qsTr("Warn after") },
                { fixed: qsTr("Warning"), label: qsTr("Action"), severity: "warning" }
            ]
        },

        {
            id: "geofence", group: qsTr("Navigation & Environment"), title: qsTr("Geofence"),
            description: qsTr("Breach of the max distance limits or of the fences defined in Plan view."),
            activeWhen: [{ param: "GF_ACTION", op: "gt", value: 0 }],
            cells: [
                { param: "GF_ACTION", label: qsTr("Action"), severity: _commonActions },
                { param: "GF_MAX_HOR_DIST", label: qsTr("Max horizontal distance"), note: qsTr("0 disables.") },
                { param: "GF_MAX_VER_DIST", label: qsTr("Max vertical distance"), note: qsTr("0 disables.") },
                { param: "GF_SOURCE", label: qsTr("Position source") },
                { param: "GF_PREDICT", label: qsTr("Pre-emptive trigger"), note: qsTr("Experimental.") }
            ]
        },
        {
            id: "gnssLoss", group: qsTr("Navigation & Environment"), title: qsTr("GNSS loss"),
            description: qsTr("Active GNSS receivers drop below the required count."),
            activeWhen: [{ param: "SYS_HAS_NUM_GNSS", op: "gt", value: 0 }],
            cells: [
                { param: "COM_GNSSLOSS_ACT", label: qsTr("Action"),
                  severity: { "0": "warning", "1": "return", "2": "land", "3": "terminate" } },
                { param: "SYS_HAS_NUM_GNSS", label: qsTr("Required receivers"), note: qsTr("0 disables.") }
            ]
        },
        {
            id: "positionEstimate", group: qsTr("Navigation & Environment"), title: qsTr("Position estimate loss"),
            description: qsTr("Position or velocity error too large. Falls back to Altitude, then Stabilized mode."),
            cells: [
                { param: "COM_POS_FS_EPH", label: qsTr("Horizontal position error"), note: qsTr("Negative disables."),
                  vehicles: ["multiRotor", "vtol"] },
                { param: "COM_VEL_FS_EVH", label: qsTr("Horizontal velocity error") },
                { fixed: qsTr("Altitude, then Stabilized fallback"), label: qsTr("Action"), severity: "fallback" }
            ]
        },
        {
            id: "lowPositionAccuracy", group: qsTr("Navigation & Environment"), title: qsTr("Low position accuracy"),
            description: qsTr("Estimated accuracy worse than the threshold. Only acts in Mission and Hold, a warning otherwise."),
            activeWhen: [{ param: "COM_POS_LOW_ACT", op: "gt", value: 0 }, { param: "COM_POS_LOW_EPH", op: "gt", value: 0 }],
            cells: [
                { param: "COM_POS_LOW_EPH", label: qsTr("Threshold"), note: qsTr("0 or negative disables.") },
                { param: "COM_POS_LOW_ACT", label: qsTr("Action"), severity: _commonActions }
            ]
        },
        {
            id: "highWind", group: qsTr("Navigation & Environment"), title: qsTr("High wind"),
            description: qsTr("Estimated wind speed. Above the max, mission resume and non-Return auto modes are blocked."),
            activeWhenAny: [[{ param: "COM_WIND_WARN", op: "gt", value: 0 }],
                            [{ param: "COM_WIND_MAX_ACT", op: "gt", value: 0 }, { param: "COM_WIND_MAX", op: "gt", value: 0 }]],
            cells: [
                { param: "COM_WIND_WARN", label: qsTr("Warn above"), note: qsTr("0 or negative disables.") },
                { param: "COM_WIND_MAX", label: qsTr("Failsafe above"), note: qsTr("0 or negative disables.") },
                { param: "COM_WIND_MAX_ACT", label: qsTr("Action"), severity: _commonActions }
            ]
        },
        {
            id: "fixedWingGps", group: qsTr("Navigation & Environment"), title: qsTr("GPS failure loiter"),
            description: qsTr("Open-loop loiter while waiting for GPS to recover, then descend. VTOLs only with NAV_FORCE_VT set to 0."),
            vehicles: ["fixedWing", "vtol"],
            activeWhen: [{ param: "FW_GPSF_LT", op: "gt", value: 0 }],
            cells: [
                { param: "FW_GPSF_LT", label: qsTr("Loiter time"), note: qsTr("0 disables.") },
                { param: "FW_GPSF_R", label: qsTr("Roll angle") }
            ]
        },
        {
            id: "traffic", group: qsTr("Navigation & Environment"), title: qsTr("Traffic avoidance (ADS-B)"),
            description: qsTr("Transponder traffic predicted to come too close."),
            activeWhen: [{ param: "NAV_TRAFF_AVOID", op: "gt", value: 0 }],
            cells: [
                { param: "NAV_TRAFF_AVOID", label: qsTr("Action"),
                  severity: { "0": "none", "1": "warning", "2": "return", "3": "land", "4": "hold" } },
                { param: "NAV_TRAFF_A_HOR", label: qsTr("Horizontal distance") },
                { param: "NAV_TRAFF_A_VER", label: qsTr("Vertical distance") },
                { param: "NAV_TRAFF_COLL_T", label: qsTr("Time to collision") }
            ]
        },

        {
            id: "behavior", group: qsTr("Failsafe Behavior"), title: qsTr("Reaction"),
            description: qsTr("How long the vehicle holds before acting, and how the pilot can take back control."),
            cells: [
                { param: "COM_FAIL_ACT_T", label: qsTr("Hold before action"),
                  note: qsTr("0 disables. The pilot can switch modes but not take over with the sticks during this time.") },
                { param: "COM_RC_OVERRIDE", label: qsTr("Stick override"), vehicles: ["multiRotor", "vtol"] },
                { param: "COM_RC_STICK_OV", label: qsTr("Override threshold"), vehicles: ["multiRotor", "vtol"] }
            ]
        },

        {
            id: "quadchute", group: qsTr("VTOL"), title: qsTr("Quadchute"),
            description: qsTr("VTOL falls back to multicopter flight when fixed-wing flight is unsafe."),
            vehicles: ["vtol"],
            cells: [
                { param: "COM_QC_ACT", label: qsTr("Action"),
                  severity: { "-1": "warning", "0": "return", "1": "land", "2": "hold" } },
                { param: "VT_FW_MIN_ALT", label: qsTr("Min. altitude"), note: qsTr("0 disables.") },
                { param: "VT_FW_QC_HMAX", label: qsTr("Max height"), note: qsTr("0 disables.") },
                { param: "VT_FW_QC_P", label: qsTr("Max pitch"), note: qsTr("0 disables.") },
                { param: "VT_FW_QC_R", label: qsTr("Max roll"), note: qsTr("0 disables.") },
                { param: "VT_QC_ALT_LOSS", label: qsTr("Uncommanded descent"), note: qsTr("0 disables.") },
                { param: "VT_QC_T_ALT_LOSS", label: qsTr("Transition altitude loss"), note: qsTr("0 disables.") }
            ]
        },

        {
            id: "attitude", group: qsTr("Vehicle Health"), title: qsTr("Attitude failure"),
            description: qsTr("Roll or pitch beyond the limits. Disarms shortly after takeoff, otherwise warns unless flight termination is enabled."),
            activeWhenAny: [[{ param: "FD_FAIL_R", op: "gt", value: 0 }], [{ param: "FD_FAIL_P", op: "gt", value: 0 }]],
            cells: [
                { param: "FD_FAIL_R", label: qsTr("Max roll"), note: qsTr("0 disables.") },
                { param: "FD_FAIL_R_TTRI", label: qsTr("Roll trigger time") },
                { param: "FD_FAIL_P", label: qsTr("Max pitch"), note: qsTr("0 disables.") },
                { param: "FD_FAIL_P_TTRI", label: qsTr("Pitch trigger time") },
                { param: "COM_LKDOWN_TKO", label: qsTr("Lockdown window after takeoff"),
                  note: qsTr("0 or negative disables.") }
            ]
        },
        {
            id: "motorFailure", group: qsTr("Vehicle Health"), title: qsTr("Motor / actuator failure"),
            description: qsTr("Motor current does not match its throttle (stuck or lost rotor)."),
            vehicles: ["multiRotor", "vtol"],
            activeWhen: [{ param: "FD_ACT_EN", op: "gt", value: 0 }],
            cells: [
                { param: "FD_ACT_EN", label: qsTr("Detection") },
                { param: "COM_ACT_FAIL_ACT", label: qsTr("Action"),
                  severity: { "0": "warning", "1": "hold", "2": "land", "3": "return", "4": "terminate" } },
                { param: "FD_ACT_MOT_THR", label: qsTr("Throttle above"), note: qsTr("100% disables.") },
                { param: "FD_ACT_MOT_C2T", label: qsTr("Current / throttle below") },
                { param: "FD_ACT_MOT_TOUT", label: qsTr("For longer than") },
                { param: "MOTFAIL_C2T", label: qsTr("Expected current per throttle") },
                { param: "MOTFAIL_OFF", label: qsTr("Allowed current deviation") },
                { param: "MOTFAIL_TIME", label: qsTr("For longer than") }
            ]
        },
        {
            id: "imbalancedPropeller", group: qsTr("Vehicle Health"), title: qsTr("Imbalanced propeller"),
            description: qsTr("Vibration pattern consistent with a damaged propeller."),
            vehicles: ["multiRotor", "vtol"],
            activeWhen: [{ param: "FD_IMB_PROP_THR", op: "gt", value: 0 }, { param: "COM_IMB_PROP_ACT", op: "ne", value: -1 }],
            cells: [
                { param: "FD_IMB_PROP_THR", label: qsTr("Threshold"), note: qsTr("0 disables.") },
                { param: "COM_IMB_PROP_ACT", label: qsTr("Action"),
                  severity: { "-1": "none", "0": "warning", "1": "return", "2": "land" } },
                { fixed: qsTr("Warning"), label: qsTr("Action"), severity: "warning", onlyIfMissing: "COM_IMB_PROP_ACT" }
            ]
        },
        {
            id: "escArming", group: qsTr("Vehicle Health"), title: qsTr("ESC arming failure"),
            description: qsTr("An ESC does not report armed after arming. Disarms."),
            activeWhen: [{ param: "FD_ESCS_EN", op: "gt", value: 0 },
                         { param: "FD_ACT_EN", op: "gt", value: 0, onlyIfMissing: "FD_ESCS_EN" }],
            cells: [
                { param: "FD_ESCS_EN", label: qsTr("Detection") },
                { param: "FD_ACT_EN", label: qsTr("Detection"), onlyIfMissing: "FD_ESCS_EN" },
                { fixed: qsTr("Disarm"), label: qsTr("Action"), severity: "terminate" }
            ]
        },
        {
            id: "parachute", group: qsTr("Vehicle Health"), title: qsTr("Parachute"),
            description: qsTr("Deployed on flight termination."),
            activeWhen: [{ param: "COM_PARACHUTE", op: "gt", value: 0 }],
            cells: [
                { param: "COM_PARACHUTE", label: qsTr("Require healthy MAVLink parachute") }
            ]
        },
        {
            id: "altitudeLoss", group: qsTr("Vehicle Health"), title: qsTr("Altitude loss"),
            description: qsTr("Altitude falls below the setpoint by more than the limit. Terminates and deploys the parachute."),
            vehicles: ["multiRotor", "vtol"],
            activeWhen: [{ param: "FD_ALT_LOSS", op: "gt", value: 0 }],
            cells: [
                { param: "FD_ALT_LOSS", label: qsTr("Max loss"), note: qsTr("0 disables.") },
                { param: "FD_ALT_LOSS_T", label: qsTr("Trigger time") },
                { fixed: qsTr("Terminate"), label: qsTr("Action"), severity: "terminate" }
            ]
        },
        {
            id: "externalTrigger", group: qsTr("Vehicle Health"), title: qsTr("External trigger (ATS)"),
            description: qsTr("PWM input from an external automatic trigger system (ASTM F3322-18). Terminates."),
            activeWhen: [{ param: "FD_EXT_ATS_EN", op: "gt", value: 0 }],
            cells: [
                { param: "FD_EXT_ATS_EN", label: qsTr("Input") },
                { param: "FD_EXT_ATS_TRIG", label: qsTr("Trigger above") }
            ]
        }
    ]
}

/// Severity key for an action cell: none, warning, fallback, hold, return, land or terminate.
/// rawValue is the parameter's raw value, ignored for a fixed action.
function actionSeverity(cell, rawValue) {
    if (typeof cell.severity === "string") {
        return cell.severity
    }
    var severity = cell.severity ? cell.severity[rawValue] : undefined
    return severity === undefined ? "none" : severity
}
