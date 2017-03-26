import QtQuick          2.3
import QtQuick.Controls 1.2
import QtQuick.Layouts  1.2
import QtQuick.Dialogs  1.2

import QGroundControl                   1.0
import QGroundControl.ScreenTools       1.0
import QGroundControl.Controls          1.0
import QGroundControl.FactControls      1.0
import QGroundControl.Palette           1.0

// Toolbar for Plan View
Rectangle {
    id:                 _root
    height:             ScreenTools.toolbarHeight
    anchors.left:       parent.left
    anchors.right:      parent.right
    anchors.top:        parent.top
    z:                  toolBar.z + 1
    color:              qgcPal.globalTheme === QGCPalette.Light ? Qt.rgba(1,1,1,0.8) : Qt.rgba(0,0,0,0.75)
    visible:            false

    signal showFlyView

    property var    missionController
    property var    currentMissionItem          ///< Mission item to display status for

    property var    missionItems:               _controllerValid ? missionController.visualItems : undefined
    property real   missionDistance:            _controllerValid ? missionController.missionDistance : NaN
    property real   missionTime:                _controllerValid ? missionController.missionTime : NaN
    property real   missionMaxTelemetry:        _controllerValid ? missionController.missionMaxTelemetry : NaN
    property bool   missionDirty:               _controllerValid ? missionController.dirty : false

    property var    _activeVehicle:             QGroundControl.multiVehicleManager.activeVehicle

    property bool   _statusValid:               currentMissionItem != undefined
    property bool   _missionValid:              missionItems != undefined
    property bool   _controllerValid:           missionController != undefined

    property real   _distance:                  _statusValid ? currentMissionItem.distance : NaN
    property real   _altDifference:             _statusValid ? currentMissionItem.altDifference : NaN
    property real   _gradient:                  _statusValid && currentMissionItem.distance > 0 ? Math.atan(currentMissionItem.altDifference / currentMissionItem.distance) : NaN
    property real   _gradientPercent:           isNaN(_gradient) ? NaN : _gradient * 100
    property real   _azimuth:                   _statusValid ? currentMissionItem.azimuth : NaN
    property real   _missionDistance:           _missionValid ? missionDistance : NaN
    property real   _missionMaxTelemetry:       _missionValid ? missionMaxTelemetry : NaN
    property real   _missionTime:               _missionValid ? missionTime : NaN

    property string _distanceText:              isNaN(_distance) ? "-.-" : QGroundControl.metersToAppSettingsDistanceUnits(_distance).toFixed(1) + " " + QGroundControl.appSettingsDistanceUnitsString
    property string _altDifferenceText:         isNaN(_altDifference) ? "-.-" : QGroundControl.metersToAppSettingsDistanceUnits(_altDifference).toFixed(1) + " " + QGroundControl.appSettingsDistanceUnitsString
    property string _gradientText:              isNaN(_gradient) ? "-.-" : _gradientPercent.toFixed(0) + "%"
    property string _azimuthText:               isNaN(_azimuth) ? "-.-" : Math.round(_azimuth)
    property string _missionDistanceText:       isNaN(_missionDistance) ? "-.-" : QGroundControl.metersToAppSettingsDistanceUnits(_missionDistance).toFixed(1) + " " + QGroundControl.appSettingsDistanceUnitsString
    property string _missionTimeText:           isNaN(_missionTime) ? "-.-" : Number(_missionTime / 60).toFixed(1) + " min"
    property string _missionMaxTelemetryText:   isNaN(_missionMaxTelemetry) ? "-.-" : QGroundControl.metersToAppSettingsDistanceUnits(_missionMaxTelemetry).toFixed(1) + " " + QGroundControl.appSettingsDistanceUnitsString

    readonly property real _margins: ScreenTools.defaultFontPixelWidth

    QGCPalette { id: qgcPal }

    //-- Eat mouse events, preventing them from reaching toolbar, which is underneath us.
    MouseArea {
        anchors.fill:   parent
        onWheel:        { wheel.accepted = true; }
        onPressed:      { mouse.accepted = true; }
        onReleased:     { mouse.accepted = true; }
    }

    Row {
        anchors.top:    parent.top
        anchors.bottom: parent.bottom
        spacing:        ScreenTools.defaultFontPixelWidth * 2

        QGCToolBarButton {
            id:                 settingsButton
            anchors.top:        parent.top
            anchors.bottom:     parent.bottom
            source:             "/qmlimages/PaperPlane.svg"
            logo:               true
            checked:            false

            onClicked: {
                checked = false
                if (missionController.dirty) {
                    uploadPrompt.visible = true
                } else {
                    showFlyView()
                }
            }

            MessageDialog {
                id:                 uploadPrompt
                title:              _activeVehicle ? qsTr("Unsent changes") : qsTr("Unsaved changes")
                text:               qsTr("You have %1 changes to your mission. Are you sure you want to leave before you %2?").arg(_activeVehicle ? qsTr("unsent") : qsTr("unsaved")).arg(_activeVehicle ? qsTr("send the mission to the vehicle") : qsTr("save the mission to a file"))
                standardButtons:    StandardButton.Yes | StandardButton.No

                onNo: visible = false

                onYes: {
                    visible = false
                    showFlyView()
                }
            }
        }

        GridLayout {
            anchors.top:    parent.top
            anchors.bottom: parent.bottom
            columns:        5
            rowSpacing:     0
            columnSpacing:  _margins / 4

            QGCLabel {
                text:               qsTr("Selected waypoint")
                Layout.columnSpan:  5
                font.pointSize:     ScreenTools.smallFontPointSize
            }

            QGCLabel { text: qsTr("Distance:") }
            QGCLabel { text: _distanceText }

            Item { width: 1; height: 1 }

            QGCLabel { text: qsTr("Gradient:") }
            QGCLabel { text: _gradientText }

            QGCLabel { text: qsTr("Alt diff:") }
            QGCLabel { text: _altDifferenceText }

            Item { width: 1; height: 1 }

            QGCLabel { text: qsTr("Azimuth:") }
            QGCLabel { text: _azimuthText }
        }

        GridLayout {
            anchors.top:    parent.top
            anchors.bottom: parent.bottom
            columns:        5
            rowSpacing:     0
            columnSpacing:  _margins / 4

            QGCLabel {
                text:               qsTr("Total mission")
                Layout.columnSpan:  5
                font.pointSize:     ScreenTools.smallFontPointSize
            }

            QGCLabel { text: qsTr("Distance:") }
            QGCLabel { text: _missionDistanceText }

            Item { width: 1; height: 1 }

            QGCLabel { text: qsTr("Max telem dist:") }
            QGCLabel { text: _missionMaxTelemetryText }

            QGCLabel { text: qsTr("Time:") }
            QGCLabel { text: _missionTimeText }
        }

        GridLayout {
            anchors.top:    parent.top
            anchors.bottom: parent.bottom
            columns:        3
            rowSpacing:     0
            columnSpacing:  _margins / 4

            QGCLabel {
                text:               qsTr("Battery")
                Layout.columnSpan:  3
                font.pointSize:     ScreenTools.smallFontPointSize
            }

            QGCLabel { text: qsTr("Batteries required:") }
            QGCLabel { text: "--.--" }

            Item { width: 1; height: 1 }

            QGCLabel { text: qsTr("Swap waypoint:") }
            QGCLabel { text: "--" }
        }
    }

    QGCButton {
        anchors.rightMargin:    _margins
        anchors.right:          parent.right
        anchors.verticalCenter: parent.verticalCenter
        text:                   _activeVehicle ? qsTr("Upload") : qsTr("Save")
        visible:                missionDirty
        primary:                true

        onClicked: {
            if (_activeVehicle) {
                missionController.sendToVehicle()
            } else {
                missionController.saveToSelectedFile()
            }
        }

        NumberAnimation on opacity {
            id:         opacityAnimation
            running:    missionDirty
            from:       0.5
            to:         1.0
            loops:      Animation.Infinite
            duration:   2000
        }
    }
}

