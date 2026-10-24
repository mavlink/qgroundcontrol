.pragma library

// Row definitions for PX4ReturnLandComponent.qml, in the PX4ConfigRows.js schema plus:
// Cell: overriddenBy?  parameter which, while at or above zero, makes the firmware set this one itself

/// True while the firmware sets the cell's parameter itself. rawValueOf(name) returns undefined for a missing parameter.
function cellOverridden(cell, rawValueOf) {
    return cell.overriddenBy !== undefined && rawValueOf(cell.overriddenBy) >= 0
}

function returnLandRows() {
    return [
        {
            id: "route", group: qsTr("Return"), title: qsTr("Route"),
            description: qsTr("Where the vehicle goes and how high it flies. Used by every Return action, including the failsafes."),
            cells: [
                { param: "RTL_TYPE", label: qsTr("Destination") },
                { param: "RTL_RETURN_ALT", label: qsTr("Climb to altitude") },
                { param: "RTL_MIN_DIST", label: qsTr("Cone: special rules within") },
                { param: "RTL_CONE_ANG", label: qsTr("Altitude cone half-angle") },
                { param: "RTL_APPR_FORCE", label: qsTr("Require approach") }
            ]
        },
        {
            id: "arrival", group: qsTr("Return"), title: qsTr("Arrival"),
            description: qsTr("What the vehicle does once it reaches the destination."),
            cells: [
                { param: "RTL_DESCEND_ALT", label: qsTr("Loiter altitude") },
                { param: "RTL_LAND_DELAY", label: qsTr("Loiter time before landing"),
                  note: qsTr("-1 loiters and does not land.") },
                { param: "RTL_LOITER_RAD", label: qsTr("Loiter radius"), vehicles: ["fixedWing", "vtol"] }
            ]
        },

        {
            id: "descent", group: qsTr("Landing"), title: qsTr("Descent"),
            description: qsTr("How fast the vehicle descends as it gets close to the ground."),
            vehicles: ["multiRotor", "vtol"],
            cells: [
                { param: "MPC_LAND_ALT1", label: qsTr("Slow descent below"),
                  note: qsTr("Must be higher than the landing speed altitude.") },
                { param: "MPC_LAND_ALT2", label: qsTr("Landing speed below"),
                  note: qsTr("Must be lower than the slow descent altitude.") },
                { param: "MPC_LAND_SPEED", label: qsTr("Landing descent rate"), overriddenBy: "MPC_Z_VEL_ALL",
                  note: qsTr("Not editable while the overall vertical velocity limit on the Flight Behavior page is on.") }
            ]
        },
        {
            id: "touchdown", group: qsTr("Landing"), title: qsTr("Touchdown"),
            description: qsTr("What happens when the vehicle lands."),
            cells: [
                { param: "RTL_PLD_MD", label: qsTr("Precision landing") },
                { param: "COM_DISARM_LAND", label: qsTr("Disarm after landing"), note: qsTr("0 or negative disables.") }
            ]
        }
    ]
}
