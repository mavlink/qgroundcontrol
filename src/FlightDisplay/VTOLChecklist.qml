/****************************************************************************
 *
 * (c) 2009-2026 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQml.Models

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlightDisplay
import QGroundControl.Vehicle

Item {
    id: root

    property var model: listModel
    property real _commandRoll:  0
    property real _commandPitch: 0
    property real _commandYaw:   0

    function _sendCurrentControl() {
        if (globals.activeVehicle) {
            globals.activeVehicle.sendFlightCheckControl(_commandRoll, _commandPitch, _commandYaw, 0)
        }
    }

    function _startControlPulse(roll, pitch, yaw) {
        _commandRoll = roll
        _commandPitch = pitch
        _commandYaw = yaw
        controlPulseTimer.remainingSends = 8
        _sendCurrentControl()
        controlPulseTimer.restart()
    }

    function _stopControlPulse() {
        controlPulseTimer.stop()
        if (globals.activeVehicle) {
            globals.activeVehicle.sendFlightCheckControl(0, 0, 0, 0)
        }
    }

    Component.onDestruction: _stopControlPulse()

    Timer {
        id:             controlPulseTimer
        interval:       100
        repeat:         true
        property int remainingSends: 0

        onTriggered: {
            if (remainingSends > 0) {
                root._sendCurrentControl()
                remainingSends--
            } else {
                root._stopControlPulse()
            }
        }
    }

    PreFlightCheckModel {
        id: listModel

        PreFlightCheckGroup {
            name: qsTr("Control Surface Logic - Manual Flight Mode")

            PreFlightManualCheckButton {
                name:       qsTr("1. Move aileron stick left")
                manualText: qsTr("Left aileron up; right aileron down.")
            }
            PreFlightManualCheckButton {
                name:       qsTr("2. Move aileron stick right")
                manualText: qsTr("Left aileron down; right aileron up.")
            }
            PreFlightManualCheckButton {
                name:       qsTr("3. Move elevator stick up")
                manualText: qsTr("Both V-tail surfaces move inward.")
            }
            PreFlightManualCheckButton {
                name:       qsTr("4. Move elevator stick down")
                manualText: qsTr("Both V-tail surfaces move outward.")
            }
            PreFlightManualCheckButton {
                name:       qsTr("5. Move rudder stick left")
                manualText: qsTr("Left tail surface moves upper-left; right tail surface moves lower-left.")
            }
            PreFlightManualCheckButton {
                name:       qsTr("6. Move rudder stick right")
                manualText: qsTr("Left tail surface moves lower-right; right tail surface moves upper-right.")
            }
        }

        PreFlightCheckGroup {
            name: qsTr("Control Surface Logic - Assisted Flight Mode A")

            PreFlightManualCheckButton {
                name:       qsTr("7. Tilt aircraft left")
                manualText: qsTr("Left aileron down; right aileron up.")
            }
            PreFlightManualCheckButton {
                name:       qsTr("8. Tilt aircraft right")
                manualText: qsTr("Left aileron up; right aileron down.")
            }
            PreFlightManualCheckButton {
                name:       qsTr("9. Raise aircraft nose")
                manualText: qsTr("Both V-tail surfaces move inward.")
            }
            PreFlightManualCheckButton {
                name:       qsTr("10. Lower aircraft nose")
                manualText: qsTr("Both V-tail surfaces move outward.")
            }
        }

        PreFlightCheckGroup {
            name: qsTr("Airspeed Check")

            PreFlightManualCheckButton {
                name:       qsTr("11. Do not blow into the airspeed tube")
                manualText: qsTr("Airspeed is 0-2 m/s; it may occasionally jump to 3 m/s or 4 m/s.")
            }
            PreFlightManualCheckButton {
                name:       qsTr("12. Blow directly into the airspeed tube")
                manualText: qsTr("Airspeed increases clearly above 10 m/s.")
            }
        }

        PreFlightCheckGroup {
            name: qsTr("Satellite Count Check")

            PreFlightManualCheckButton {
                name:       qsTr("13. Observe satellite count")
                manualText: qsTr("Satellite count is at least 28.")
            }
        }

        PreFlightCheckGroup {
            name: qsTr("Flight Control Surface Check")

            PreFlightCommandCheckButton {
                name:       qsTr("14. Send climb command")
                manualText: qsTr("Both V-tail surfaces move outward.")
                onCommandRequested: root._startControlPulse(0, -0.5, 0)
            }
            PreFlightCommandCheckButton {
                name:       qsTr("15. Send descend command")
                manualText: qsTr("Both V-tail surfaces move inward.")
                onCommandRequested: root._startControlPulse(0, 0.5, 0)
            }
            PreFlightCommandCheckButton {
                name:       qsTr("16. Send roll-left command")
                manualText: qsTr("Left aileron up; right aileron down.")
                onCommandRequested: root._startControlPulse(-0.5, 0, 0)
            }
            PreFlightCommandCheckButton {
                name:       qsTr("17. Send roll-right command")
                manualText: qsTr("Left aileron down; right aileron up.")
                onCommandRequested: root._startControlPulse(0.5, 0, 0)
            }
            PreFlightCommandCheckButton {
                name:       qsTr("18. Send yaw-left command")
                manualText: qsTr("Left tail surface moves upper-left; right tail surface moves lower-left.")
                onCommandRequested: root._startControlPulse(0, 0, -0.5)
            }
            PreFlightCommandCheckButton {
                name:       qsTr("19. Send yaw-right command")
                manualText: qsTr("Left tail surface moves lower-right; right tail surface moves upper-right.")
                onCommandRequested: root._startControlPulse(0, 0, 0.5)
            }
        }

        PreFlightCheckGroup {
            name: qsTr("Fixed-Wing Throttle Check")

            PreFlightCommandCheckButton {
                name:               qsTr("20. Test fixed-wing throttle")
                manualText:         qsTr("The fixed-wing motor should rotate counter-clockwise when viewed from tail to nose.")
                commandAvailable:   false
            }
        }

        PreFlightCheckGroup {
            name: qsTr("Lift Motor Check")

            PreFlightCommandCheckButton {
                name:       qsTr("21. Test motor A")
                manualText: qsTr("Front-right motor should rotate counter-clockwise.")
                onCommandRequested: if (globals.activeVehicle) globals.activeVehicle.motorTest(1, 10, 2, true)
            }
            PreFlightCommandCheckButton {
                name:       qsTr("22. Test motor B")
                manualText: qsTr("Rear-right motor should rotate clockwise.")
                onCommandRequested: if (globals.activeVehicle) globals.activeVehicle.motorTest(2, 10, 2, true)
            }
            PreFlightCommandCheckButton {
                name:       qsTr("23. Test motor C")
                manualText: qsTr("Rear-left motor should rotate counter-clockwise.")
                onCommandRequested: if (globals.activeVehicle) globals.activeVehicle.motorTest(3, 10, 2, true)
            }
            PreFlightCommandCheckButton {
                name:       qsTr("24. Test motor D")
                manualText: qsTr("Front-left motor should rotate clockwise.")
                onCommandRequested: if (globals.activeVehicle) globals.activeVehicle.motorTest(4, 10, 2, true)
            }
        }

        PreFlightCheckGroup {
            name: qsTr("Flight Check Complete")

            PreFlightManualCheckButton {
                name:       qsTr("25. Aircraft flight check complete")
                manualText: qsTr("Confirm that all flight-check items have been completed.")
            }
        }
    }
}
