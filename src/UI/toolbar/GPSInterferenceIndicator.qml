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

            let maxState = Math.max(_activeVehicle.gps.spoofingState.value, _activeVehicle.gps.jammingState.value);

            switch (maxState) {
            case 3:
                return qgcPal.colorRed;
            case 2:
                return qgcPal.colorOrange;
            case 1:
                return qgcPal.colorWhite;
            default:
                return qgcPal.colorGrey;
            }
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
                    labelText: _activeVehicle ? (_activeVehicle.gps.jammingState.valueString || qsTr("n/a")) : qsTr("n/a")
                }

                LabelledLabel {
                    label: qsTr("GPS Spoofing")
                    labelText: _activeVehicle ? (_activeVehicle.gps.spoofingState.valueString || qsTr("n/a")) : qsTr("n/a")
                }
            }
        }
    }
}
