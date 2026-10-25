import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlightMap

/// Live attitude of the vehicle and the sensors it has, to check the autopilot orientation and the compass against.
/// devices is PX4SensorsRows.hardwareDevices, deviceName(param) returns the description of a device ID parameter.
ConfigRowsPanel {
    id: control

    property var vehicle:       null
    /// "multiRotor", "fixedWing", "vtol", "rover" or anything else for a flat board
    property string vehicleKind:    ""
    property var devices:       []
    property var deviceName:    function (param) { return "" }

    QGCPalette { id: qgcPal }

    // Formats after rounding, so a small negative value shows 0 rather than -0. A wrapped heading shows 360 as 0.
    function _degrees(value, decimals, wrap) {
        if (!isFinite(value)) {
            return "--"
        }
        let rounded = Number(value.toFixed(decimals))
        if (wrap && rounded >= 360) {
            rounded -= 360
        }
        if (rounded === 0) {
            rounded = 0
        }
        return rounded.toFixed(decimals) + "°"
    }

    QGCLabel {
        text:       qsTr("Live orientation")
        font.bold:  true
    }

    // Qt Quick 3D needs a GPU renderer. Without one the attitude indicator is shown instead.
    Loader {
        objectName:             "sensors_attitude"
        Layout.fillWidth:       true
        Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 12
        sourceComponent: {
            switch (GraphicsInfo.api) {
            case GraphicsInfo.Unknown:
                return null
            case GraphicsInfo.Software:
                return attitudeIndicator
            }
            return vehicleView
        }
    }

    Component {
        id: vehicleView

        SensorsVehicleView {
            live:           true
            vehicleKind:    control.vehicleKind
            roll:           control.vehicle ? control.vehicle.roll.rawValue : 0
            pitch:          control.vehicle ? control.vehicle.pitch.rawValue : 0
            heading:        control.vehicle ? control.vehicle.heading.rawValue : 0
        }
    }

    Component {
        id: attitudeIndicator

        Item {
            QGCAttitudeWidget {
                anchors.centerIn:   parent
                size:               parent.height
                vehicle:            control.vehicle
            }
        }
    }

    RowLayout {
        Layout.fillWidth:   true
        spacing:            ScreenTools.defaultFontPixelWidth

        Repeater {
            model: [
                { caption: qsTr("Roll"),    value: control._degrees(control.vehicle ? control.vehicle.roll.rawValue : NaN, 1) },
                { caption: qsTr("Pitch"),   value: control._degrees(control.vehicle ? control.vehicle.pitch.rawValue : NaN, 1) },
                { caption: qsTr("Heading"), value: control._degrees(control.vehicle ? control.vehicle.heading.rawValue : NaN, 0, true) }
            ]

            // Equal fixed widths and right aligned values, so changing values do not move anything
            QGCCaptionCell {
                Layout.fillWidth:       true
                Layout.preferredWidth:  1
                caption:                modelData.caption

                required property var modelData

                QGCLabel {
                    Layout.fillWidth:       true
                    horizontalAlignment:    Text.AlignRight
                    text:                   modelData.value
                    font.family:            ScreenTools.fixedFontFamily
                }
            }
        }
    }

    Rectangle {
        Layout.fillWidth:   true
        implicitHeight:     1
        color:              qgcPal.borderSubtle
        visible:            control.devices.length > 0
    }

    ColumnLayout {
        Layout.fillWidth:   true
        spacing:            ScreenTools.defaultFontPixelHeight * 0.25
        visible:            control.devices.length > 0

        Repeater {
            model: control.devices

            RowLayout {
                Layout.fillWidth:   true

                required property var modelData

                QGCLabel {
                    Layout.fillWidth:   true
                    text:               modelData.label
                    color:              qgcPal.textSecondary
                }

                QGCLabel {
                    text:           control.deviceName(modelData.param)
                    font.family:    ScreenTools.fixedFontFamily
                }
            }
        }
    }

    ConfigNote {
        text: qsTr("Tilt and turn the vehicle: the model should follow. If it moves the wrong way, change the autopilot rotation.")
    }
}
