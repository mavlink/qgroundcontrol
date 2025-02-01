/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.MultiVehicleManager
import QGroundControl.ScreenTools
import QGroundControl.Palette

//-------------------------------------------------------------------------
//-- RC RSSI Indicator
Item {
    id:             control
    width:          rssiRow.width * 1.1
    anchors.top:    parent.top
    anchors.bottom: parent.bottom

    property bool showIndicator: _activeVehicle.supportsRadio && _rcRSSIAvailable

    property var    _activeVehicle:     QGroundControl.multiVehicleManager.activeVehicle
    property bool   _rcRSSIAvailable:   _activeVehicle.rcRSSI > 0 && _activeVehicle.rcRSSI <= 100

    Component {
        id: rcRSSIInfoPage

        ToolIndicatorPage {
            showExpand: false

            contentComponent: SettingsGroupLayout {
                heading: qsTr("RC RSSI Status")

                LabelledLabel {
                    label:      qsTr("RSSI")
                    labelText:  _activeVehicle.rcRSSI + "%"
                }
            }
        }
    }

    Row {
        id:             rssiRow
        anchors.top:    parent.top
        anchors.bottom: parent.bottom
        spacing:        ScreenTools.defaultFontPixelWidth

        QGCColoredImage {
            width:              height
            anchors.top:        parent.top
            anchors.bottom:     parent.bottom
            sourceSize.height:  height
            source:             "/qmlimages/RC.svg"
            fillMode:           Image.PreserveAspectFit
            opacity:            _rcRSSIAvailable ? 1 : 0.5
            color:              qgcPal.buttonText
        }

        SignalStrength {
            anchors.verticalCenter: parent.verticalCenter
            size:                   parent.height * 0.5
            percent:                _rcRSSIAvailable ? _activeVehicle.rcRSSI : 0
        }
    }

    MouseArea {
        anchors.fill:   parent
        onClicked:      mainWindow.showIndicatorDrawer(rcRSSIInfoPage, control)
    }
}
