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
    color:              qgcPal.window
    visible:            false

    signal showFlyView

    property var    missionController
    property var    currentMissionItem          ///< Mission item to display status for

    property var    missionItems:               missionController.visualItems
    property real   missionDistance:            missionController.missionDistance
    property real   missionTime:                missionController.missionTime
    property real   missionMaxTelemetry:        missionController.missionMaxTelemetry

    property var    _activeVehicle:             QGroundControl.multiVehicleManager.activeVehicle

    property bool   _statusValid:               currentMissionItem != undefined
    property bool   _missionValid:              missionItems != undefined

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

    QGCPalette { id: qgcPal }

    Row {
        id: mainRow
        spacing:        27.75 //-- Hard coded to fit the ST16 Screen
        anchors.top:    parent.top
        anchors.bottom: parent.bottom
        anchors.bottomMargin:       1
        anchors.horizontalCenter:   parent.horizontalCenter

        QGCToolBarButton {
            id:                 settingsButton
            anchors.top:        parent.top
            anchors.bottom:     parent.bottom
            source:             "/typhoonh/Home.svg"
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
                text:               qsTr("You have %1 changes to your mission. Are you sure you want to leave before you %2?").arg(_activeVehicle ? qsTr("unsent") : qsTr("unsaved")).arg(_activeVehicle ? qsTr("send the missoin to the vehicle") : qsTr("save the mission to a file"))
                standardButtons:    StandardButton.Yes | StandardButton.No
                onNo: visible = false
                onYes: {
                    visible = false
                    showFlyView()
                }
            }
        }

        Rectangle {
            height:             parent.height * 0.75
            width:              1
            color:              qgcPal.text
            opacity:            0.5
            anchors.verticalCenter: parent.verticalCenter
        }

        Row {
            spacing:        ScreenTools.defaultFontPixelWidth * 2
            anchors.verticalCenter: parent.verticalCenter
            Column {
                spacing:    ScreenTools.defaultFontPixelHeight * 0.25
                QGCLabel {
                    text: qsTr("Selected")
                    anchors.horizontalCenter: parent.horizontalCenter
                }
                QGCLabel {
                    text: qsTr("Waypoint")
                    anchors.horizontalCenter: parent.horizontalCenter
                }
            }

            GridLayout {
                columns:        4
                rowSpacing:     ScreenTools.defaultFontPixelHeight * 0.25
                columnSpacing:  _margins

                QGCLabel { text: qsTr("Distance:") }
                QGCLabel {
                    text: _distanceText
                    Layout.minimumWidth: ScreenTools.defaultFontPixelWidth * 8
                    horizontalAlignment: Text.AlignRight
                }

                QGCLabel { text: qsTr("Gradient:") }
                QGCLabel { text: _gradientText
                    Layout.minimumWidth: ScreenTools.defaultFontPixelWidth * 6
                    horizontalAlignment: Text.AlignRight
                }

                QGCLabel { text: qsTr("Alt Diff:") }
                QGCLabel { text: _altDifferenceText
                    Layout.minimumWidth: ScreenTools.defaultFontPixelWidth * 8
                    horizontalAlignment: Text.AlignRight
                }

                QGCLabel { text: qsTr("Azimuth:") }
                QGCLabel { text: _azimuthText
                    Layout.minimumWidth: ScreenTools.defaultFontPixelWidth * 6
                    horizontalAlignment: Text.AlignRight
                }
            }
        }

        Row {
            spacing:        ScreenTools.defaultFontPixelWidth * 2
            anchors.verticalCenter: parent.verticalCenter
            Column {
                spacing:    ScreenTools.defaultFontPixelHeight * 0.25
                QGCLabel {
                    text: qsTr("Total")
                    anchors.horizontalCenter: parent.horizontalCenter
                }
                QGCLabel {
                    text: qsTr("Mission")
                    anchors.horizontalCenter: parent.horizontalCenter
                }
            }

            GridLayout {
                columns:        4
                rowSpacing:     ScreenTools.defaultFontPixelHeight * 0.25
                columnSpacing:  _margins

                QGCLabel { text: qsTr("Distance:") }
                QGCLabel { text: _missionDistanceText
                    Layout.minimumWidth: ScreenTools.defaultFontPixelWidth * 10
                    horizontalAlignment: Text.AlignRight
                }

                QGCLabel { text: qsTr("Time:") }
                QGCLabel { text: _missionTimeText
                    Layout.minimumWidth: ScreenTools.defaultFontPixelWidth * 6
                    horizontalAlignment: Text.AlignRight
                }

                QGCLabel { text: qsTr("Max Telem Dist:") }
                QGCLabel { text: _missionMaxTelemetryText
                    Layout.minimumWidth: ScreenTools.defaultFontPixelWidth * 10
                    horizontalAlignment: Text.AlignRight
                }
            }
        }

        QGCButton {
            id:         saveButton
            text:       _activeVehicle ? qsTr("Upload") : qsTr("Save")
            visible:    missionController.dirty
            width:      ScreenTools.defaultFontPixelWidth * 10
            anchors.verticalCenter: parent.verticalCenter
            onClicked: {
                if (_activeVehicle) {
                    missionController.sendToVehicle()
                } else {
                    missionController.saveToSelectedFile()
                }
            }
        }

        Item {
            height:     1
            width:      saveButton.width
            visible:    !missionController.dirty
        }

        Rectangle {
            height:             parent.height * 0.75
            width:              1
            color:              qgcPal.text
            opacity:            0.5
            anchors.verticalCenter: parent.verticalCenter
        }

        Item {
            width:              logoImage.width
            anchors.top:        parent.top
            anchors.bottom:     parent.bottom
            anchors.margins:    ScreenTools.defaultFontPixelHeight * 0.66
            Image {
                id:             logoImage
                height:         parent.height * 0.45
                fillMode:       Image.PreserveAspectFit
                source:         _outdoorPalette ? _brandImageOutdoor : _brandImageIndoor
                anchors.verticalCenter: parent.verticalCenter
                property bool   _outdoorPalette:        qgcPal.globalTheme === QGCPalette.Light
                property bool   _corePluginBranding:    QGroundControl.corePlugin.brandImageIndoor.length !== 0
                property string _brandImageIndoor:      _corePluginBranding ? QGroundControl.corePlugin.brandImageIndoor  : (_activeVehicle ? _activeVehicle.brandImageIndoor : "")
                property string _brandImageOutdoor:     _corePluginBranding ? QGroundControl.corePlugin.brandImageOutdoor : (_activeVehicle ? _activeVehicle.brandImageOutdoor : "")
            }
        }

    }

}

