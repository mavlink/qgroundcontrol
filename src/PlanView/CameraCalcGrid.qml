import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.ScreenTools
import QGroundControl.Controls
import QGroundControl.FactControls
import QGroundControl.Palette

// Camera calculator "Grid" section for mission item editors
Column {
    spacing: _margin

    property var    cameraCalc
    property bool   vehicleFlightIsFrontal:         true
    property string distanceToSurfaceLabel
    property string frontalDistanceLabel
    property string sideDistanceLabel

    property real   _margin:            ScreenTools.defaultFontPixelWidth / 2
    property real   _fieldWidth:        ScreenTools.defaultFontPixelWidth * 10.5
    property var    _vehicle:           QGroundControl.multiVehicleManager.activeVehicle ? QGroundControl.multiVehicleManager.activeVehicle : QGroundControl.multiVehicleManager.offlineEditingVehicle
    property bool   _cameraComboFilled: false

    readonly property int _gridTypeManual:          0
    readonly property int _gridTypeCustomCamera:    1
    readonly property int _gridTypeCamera:          2

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    Column {
        anchors.left:   parent.left
        anchors.right:  parent.right
        spacing:        _margin
        visible:        !cameraCalc.isManualCamera

        RowLayout {
            anchors.left:   parent.left
            anchors.right:  parent.right
            spacing:        _margin
            Item { Layout.fillWidth: true }
            QGCLabel {
                Layout.preferredWidth:  _root._fieldWidth
                text:                   qsTr("Front Lap")
            }
            QGCLabel {
                Layout.preferredWidth:  _root._fieldWidth
                text:                   qsTr("Side Lap")
            }
        }

        RowLayout {
            anchors.left:   parent.left
            anchors.right:  parent.right
            spacing:        _margin
            QGCLabel { text: qsTr("Overlap"); Layout.fillWidth: true }
            FactTextField {
                Layout.preferredWidth:  _root._fieldWidth
                fact:                   cameraCalc.frontalOverlap
            }
            FactTextField {
                Layout.preferredWidth:  _root._fieldWidth
                fact:                   cameraCalc.sideOverlap
            }
        }

        QGCLabel {
            wrapMode:               Text.WordWrap
            text:                   qsTr("Select one:")
            Layout.preferredWidth:  parent.width
            Layout.columnSpan:      2
        }

        GridLayout {
            anchors.left:   parent.left
            anchors.right:  parent.right
            columnSpacing:  _margin
            rowSpacing:     _margin
            columns:        2

            QGCRadioButton {
                id:                     fixedDistanceRadio
                leftPadding:            0
                text:                   distanceToSurfaceLabel
                checked:                !!cameraCalc.valueSetIsDistance.value
                onClicked:              cameraCalc.valueSetIsDistance.value = 1
            }

            AltitudeFactTextField {
                fact:                       cameraCalc.distanceToSurface
                altitudeMode:               cameraCalc.distanceMode
                enabled:                    fixedDistanceRadio.checked
                Layout.fillWidth:           true
            }

            QGCRadioButton {
                id:                     fixedImageDensityRadio
                leftPadding:            0
                text:                   qsTr("Grnd Res")
                checked:                !cameraCalc.valueSetIsDistance.value
                onClicked:              cameraCalc.valueSetIsDistance.value = 0
            }

            FactTextField {
                fact:                   cameraCalc.imageDensity
                enabled:                fixedImageDensityRadio.checked
                Layout.fillWidth:       true
            }
        }
    } // Column - Camera spec based ui

    // No camera spec ui
    GridLayout {
        anchors.left:   parent.left
        anchors.right:  parent.right
        columnSpacing:  _margin
        rowSpacing:     _margin
        columns:        2
        visible:        cameraCalc.isManualCamera

        QGCLabel { text: distanceToSurfaceLabel }
        AltitudeFactTextField {
            fact:                       cameraCalc.distanceToSurface
            altitudeMode:               cameraCalc.distanceMode
            Layout.fillWidth:           true
        }

        QGCLabel { text: frontalDistanceLabel }
        FactTextField {
            Layout.fillWidth:   true
            fact:               cameraCalc.adjustedFootprintFrontal
        }

        QGCLabel { text: sideDistanceLabel }
        FactTextField {
            Layout.fillWidth:   true
            fact:               cameraCalc.adjustedFootprintSide
        }
    } // GridLayout
} // Column
