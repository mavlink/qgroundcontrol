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

import QGroundControl.ScreenTools

//-------------------------------------------------------------------------
//-- RC RSSI Indicator
Item {
    id:             control
    anchors.top:    parent.top
    height: rssiRow.height
    anchors.verticalCenter: parent.verticalCenter


    property real indicatorHeight: ScreenTools.defaultFontPixelHeight * 3.5//my add
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
        height: indicatorHeight
        anchors.verticalCenter: parent.verticalCenter
        //spacing:        ScreenTools.defaultFontPixelWidth
        spacing: indicatorHeight * 0.2

        QGCColoredImage {
            width:  indicatorHeight * 0.6
            height: indicatorHeight * 0.6
            anchors.verticalCenter: parent.verticalCenter
            sourceSize.width:  indicatorHeight * 0.8
            sourceSize.height: indicatorHeight * 0.8
            source: "/qmlimages/RC.svg"
            fillMode: Image.PreserveAspectFit
            opacity: _rcRSSIAvailable ? 1 : 0.5
            color: qgcPal.buttonText
        }

        SignalStrength {
            anchors.verticalCenter: parent.verticalCenter
            size:                   indicatorHeight//parent.height * 0.5
            percent:                _rcRSSIAvailable ? _activeVehicle.rcRSSI : 0
        }

    }

    MouseArea {
        anchors.fill:   parent
        onClicked:      mainWindow.showIndicatorDrawer(rcRSSIInfoPage, control)
    }
}
