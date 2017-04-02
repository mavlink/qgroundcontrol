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
    property bool   _manualUpload:              QGroundControl.settingsManager.appSettings.automaticMissionUpload.rawValue == 0

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

    readonly property real _margins:            ScreenTools.defaultFontPixelWidth
    readonly property real _fontSize:           ScreenTools.smallFontPointSize

    QGCPalette { id: qgcPal }

    //-- Eat mouse events, preventing them from reaching toolbar, which is underneath us.
    MouseArea {
        anchors.fill:   parent
        onWheel:        { wheel.accepted = true; }
        onPressed:      { mouse.accepted = true; }
        onReleased:     { mouse.accepted = true; }
    }

    QGCToolBarButton {
        id:                 settingsButton
        anchors.top:        parent.top
        anchors.bottom:     parent.bottom
        anchors.left:       parent.left
        anchors.leftMargin: 10
        source:             "/typhoonh/Home.svg"
        checked:            false

        onClicked: {
            checked = false
            if (missionController.uploadOnSwitch()) {
                showFlyView()
            }
        }
    }

    Row {
        id: mainRow
        spacing:            30
        anchors.top:    parent.top
        anchors.bottom: parent.bottom
        anchors.bottomMargin:       1
        anchors.horizontalCenter:   parent.horizontalCenter

        GridLayout {
            columns:        4
            rowSpacing:     ScreenTools.defaultFontPixelHeight * 0.15
            columnSpacing:  _margins
            anchors.verticalCenter: parent.verticalCenter

            QGCLabel {
                text: qsTr("Selected Waypoint")
                Layout.columnSpan: 4
                font.pointSize: _fontSize
            }

            QGCLabel { text: qsTr("Distance:"); font.pointSize: _fontSize }
            QGCLabel {
                text: _distanceText
                font.pointSize:      _fontSize
                Layout.minimumWidth: ScreenTools.defaultFontPixelWidth * 8
                horizontalAlignment: Text.AlignRight
            }

            QGCLabel { text: qsTr("Gradient:"); font.pointSize: _fontSize }
            QGCLabel { text: _gradientText
                font.pointSize: _fontSize
                Layout.minimumWidth: ScreenTools.defaultFontPixelWidth * 6
                horizontalAlignment: Text.AlignRight
            }

            QGCLabel { text: qsTr("Alt Diff:"); font.pointSize: _fontSize }
            QGCLabel { text: _altDifferenceText
                font.pointSize: _fontSize
                Layout.minimumWidth: ScreenTools.defaultFontPixelWidth * 8
                horizontalAlignment: Text.AlignRight
            }

            QGCLabel { text: qsTr("Azimuth:"); font.pointSize: _fontSize }
            QGCLabel { text: _azimuthText
                font.pointSize: _fontSize
                Layout.minimumWidth: ScreenTools.defaultFontPixelWidth * 6
                horizontalAlignment: Text.AlignRight
            }
        }

        Item {
            width:  10
            height: 1
        }

        GridLayout {
            columns:        4
            rowSpacing:     ScreenTools.defaultFontPixelHeight * 0.15
            columnSpacing:  _margins
            anchors.verticalCenter: parent.verticalCenter

            QGCLabel {
                text: qsTr("Total Mission")
                Layout.columnSpan: 4
                font.pointSize: _fontSize
            }

            QGCLabel { text: qsTr("Distance:"); font.pointSize: _fontSize }
            QGCLabel { text: _missionDistanceText
                font.pointSize: _fontSize
                Layout.minimumWidth: ScreenTools.defaultFontPixelWidth * 10
                horizontalAlignment: Text.AlignRight
            }

            QGCLabel { text: qsTr("Time:"); font.pointSize: _fontSize }
            QGCLabel { text: _missionTimeText
                font.pointSize: _fontSize
                Layout.minimumWidth: ScreenTools.defaultFontPixelWidth * 6
                horizontalAlignment: Text.AlignRight
            }

            QGCLabel { text: qsTr("Max Telem Dist:"); font.pointSize: _fontSize }
            QGCLabel { text: _missionMaxTelemetryText
                font.pointSize: _fontSize
                Layout.minimumWidth: ScreenTools.defaultFontPixelWidth * 10
                horizontalAlignment: Text.AlignRight
            }
        }

        Item {
            width:  10
            height: 1
        }

        GridLayout {
            columns:        2
            rowSpacing:     ScreenTools.defaultFontPixelHeight * 0.15
            columnSpacing:  _margins
            anchors.verticalCenter: parent.verticalCenter

            QGCLabel {
                text:               qsTr("Battery")
                Layout.columnSpan:  2
                font.pointSize:     _fontSize
            }

            QGCLabel { text: qsTr("Batteries required:"); font.pointSize: _fontSize }
            QGCLabel { text: "--.--"
                font.pointSize: _fontSize
                Layout.minimumWidth: ScreenTools.defaultFontPixelWidth * 6
                horizontalAlignment: Text.AlignRight
            }

            QGCLabel { text: qsTr("Swap waypoint:"); font.pointSize: _fontSize }
            QGCLabel { text: "--"
                font.pointSize: _fontSize
                Layout.minimumWidth: ScreenTools.defaultFontPixelWidth * 6
                horizontalAlignment: Text.AlignRight
            }
        }

        QGCButton {
            id:                     uploadButton
            anchors.verticalCenter: parent.verticalCenter
            text:                   missionController ? (missionController.dirty ? qsTr("Upload Required") : qsTr("Upload")) : ""
            enabled:                _activeVehicle
            visible:                _manualUpload
            onClicked:              missionController.upload()
        }
    }
}

