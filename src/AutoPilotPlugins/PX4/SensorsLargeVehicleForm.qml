import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

import "PX4SensorsRows.js" as SensorsRows

/// The compass calibration for a vehicle too big to rotate, which sets only the offsets from a known heading and the
/// vehicle's GPS position. It sits under the six side calibration, collapsed until it is asked for.
ColumnLayout {
    id: control

    /// SensorsRows.gpsReadiness()
    required property var gps
    property bool   open:       false
    /// The firmware's answer to the last calibration, empty before one
    property string message:    ""
    property bool   success:    false
    /// A calibration was requested and the vehicle has not answered yet
    property bool   pending:    false
    signal calibrateRequested(real heading)
    signal rebootRequested

    readonly property real _heading:        SensorsRows.parseHeading(headingField.text)
    readonly property bool _headingValid:   !isNaN(_heading) && _heading >= 0 && _heading < 360

    spacing: ScreenTools.defaultFontPixelHeight * 0.5

    QGCPalette { id: qgcPal }

    RowLayout {
        spacing:    ScreenTools.defaultFontPixelWidth
        visible:    !control.open

        QGCLabel {
            text:   qsTr("Too big to rotate?")
            color:  qgcPal.textSecondary
        }

        QGCLabel {
            objectName: "sensorsCal_largeVehicleOpen"
            text:       qsTr("Large vehicle calibration") + " ▸"
            color:      qgcPal.accent

            TapHandler {
                onTapped: control.open = true
            }
        }
    }

    Rectangle {
        Layout.fillWidth:   true
        implicitHeight:     formLayout.implicitHeight + ScreenTools.defaultFontPixelHeight
        radius:             ScreenTools.defaultBorderRadius * 2
        color:              qgcPal.windowCard
        border.color:       qgcPal.borderSubtle
        visible:            control.open

        ColumnLayout {
            id:                 formLayout
            anchors.fill:       parent
            anchors.margins:    ScreenTools.defaultFontPixelHeight * 0.5
            spacing:            ScreenTools.defaultFontPixelHeight * 0.5

            RowLayout {
                Layout.fillWidth:   true
                spacing:            ScreenTools.defaultFontPixelWidth

                QGCLabel {
                    text:       qsTr("Large vehicle calibration")
                    font.bold:  true
                }

                QGCChip {
                    text: qsTr("Offsets only")
                }

                Item { Layout.fillWidth: true }

                QGCLabel {
                    text:   qsTr("Hide") + " ▾"
                    color:  qgcPal.accent

                    TapHandler {
                        onTapped: control.open = false
                    }
                }
            }

            QGCLabel {
                Layout.fillWidth:   true
                wrapMode:           Text.WordWrap
                color:              qgcPal.textSecondary
                text:               qsTr("Point the vehicle at a known heading and hold it still. This uses the heading and the GPS position, and is less accurate than calibrating all six sides.")
            }

            RowLayout {
                Layout.fillWidth:   true
                spacing:            ScreenTools.defaultFontPixelWidth * 2

                ColumnLayout {
                    spacing: ScreenTools.defaultFontPixelHeight * 0.15

                    QGCLabel {
                        text:           qsTr("Heading (0 is true north)")
                        color:          qgcPal.textSecondary
                        font.pointSize: ScreenTools.smallFontPointSize
                    }

                    QGCTextField {
                        id:                 headingField
                        objectName:         "sensorsCal_headingField"
                        text:               "0"
                        numericValuesOnly:  true
                        showUnits:          true
                        unitsLabel:         qsTr("deg")
                        validationError:    !control._headingValid
                    }
                }

                SensorsGpsStatus {
                    gps: control.gps
                }

                Item { Layout.fillWidth: true }

                QGCButton {
                    objectName: "sensorsCal_largeVehicleCalibrate"
                    text:       qsTr("Calibrate")
                    primary:    true
                    enabled:    control.gps.ready && control._headingValid && !control.pending
                    onClicked:  control.calibrateRequested(control._heading)
                }
            }

            QGCLabel {
                objectName:         "sensorsCal_largeVehicleHint"
                Layout.fillWidth:   true
                wrapMode:           Text.WordWrap
                text:               control.message !== "" ? control.message :
                                        control.pending ? qsTr("Calibrating…") : (control.gps.ready ? "" : control.gps.message)
                color:              control.message !== "" ? (control.success ? qgcPal.successText : qgcPal.errorText) :
                                        control.pending ? qgcPal.textSecondary : qgcPal.errorText
                visible:            text !== ""
            }

            QGCButton {
                objectName: "sensorsCal_largeVehicleReboot"
                text:       qsTr("Reboot vehicle")
                primary:    true
                visible:    control.success && control.message !== ""
                onClicked:  control.rebootRequested()
            }
        }
    }
}
