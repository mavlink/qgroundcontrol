/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.ScreenTools
import QGroundControl.Palette

//-------------------------------------------------------------------------
//-- GPS Interference Indicator
Item {
    id:             control
    width:          height
    anchors.top:    parent.top
    anchors.bottom: parent.bottom

    property var    _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle

    property bool   showIndicator: _activeVehicle && (_activeVehicle.gps.spoofingState.value > 0 || _activeVehicle.gps.jammingState.value > 0)
    

    function spoofingText() {
        if(!_activeVehicle){
            return qsTr("Disconnected")
        } else if (_activeVehicle.gps.spoofingState.value === 1) {
            return qsTr("OK")
        } else if (_activeVehicle.gps.spoofingState.value === 2) {
            return qsTr("Mitigated")
        } else if (_activeVehicle.gps.spoofingState.value === 3) {
            return qsTr("Ongoing")
        }
        return qsTr("n/a")
    }

    function jammingText() {
        if(!_activeVehicle){
            return qsTr("Disconnected")
        } else if (_activeVehicle.gps.jammingState.value === 1) {
            return qsTr("OK")
        } else if (_activeVehicle.gps.jammingState.value === 2) {
            return qsTr("Mitigated")
        } else if (_activeVehicle.gps.jammingState.value === 3) {
            return qsTr("Ongoing")
        }
        return qsTr("n/a")
    }

    QGCColoredImage {
        id:                 gpsSpoofingIcon
        width:              height
        anchors.top:        parent.top
        anchors.bottom:     parent.bottom
        source:             "/qmlimages/GpsInterference.svg"
        fillMode:           Image.PreserveAspectFit
        sourceSize.height:  height
        opacity:            1
        color: {
            if(!_activeVehicle){
                return qgcPal.colorGrey
            }

            let spoofing = _activeVehicle.gps.spoofingState.value
            let jamming = _activeVehicle.gps.jammingState.value

            if (spoofing === 3 || jamming === 3) {
                return qgcPal.colorRed
            } else if (spoofing === 2 || jamming === 2) {
                return qgcPal.colorOrange
            } else if (spoofing === 1 || jamming === 1) {
                return qgcPal.colorWhite
            }
            return qgcPal.colorGrey
        }
    }

    MouseArea {
        anchors.fill:   parent
        onClicked:      mainWindow.showIndicatorDrawer(gpsSpoofingPopup, control)
    }

    Component {
        id: gpsSpoofingPopup

        ToolIndicatorPage {
            showExpand: expandedComponent ? true : false
            contentComponent: spoofingContentComponent
        }
    }

    Component{
        id: spoofingContentComponent

        ColumnLayout{
            spacing: ScreenTools.defaultFontPixelHeight / 2

            SettingsGroupLayout {
                heading: qsTr("GPS Interference Status")
                showDividers: true

                LabelledLabel {
                    label: qsTr("GPS Jamming")
                    labelText: jammingText()
                }

                LabelledLabel {
                    label: qsTr("GPS Spoofing")
                    labelText: spoofingText()
                }
            }
        }
    }
}
