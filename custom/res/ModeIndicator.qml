/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick          2.3
import QtQuick.Controls 1.2

import QGroundControl                       1.0
import QGroundControl.Controls              1.0
import QGroundControl.MultiVehicleManager   1.0
import QGroundControl.ScreenTools           1.0
import QGroundControl.Palette               1.0

//-------------------------------------------------------------------------
//-- Mode Indicator
Item {
    anchors.top:    parent.top
    anchors.bottom: parent.bottom
    width:          ScreenTools.defaultFontPixelWidth * 12

    property var    _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle
    property var    _flightModes:   []

    Component.onCompleted: updateFlightModesMenu()

    Connections {
        target:                 QGroundControl.multiVehicleManager
        onActiveVehicleChanged: updateFlightModesMenu()
    }

    function updateFlightModesMenu() {
        if (_activeVehicle && _activeVehicle.flightModeSetAvailable) {
            // Remove old menu items
            _flightModes = [];
            // Add new items
            for (var i = 0; i < _activeVehicle.flightModes.length; i++) {
                _flightModes.push(_activeVehicle.flightModes[i]);
                //console.log(_activeVehicle.flightModes[i])
            }
            _flightModes.sort();
        }
    }

    Component {
        id: modeMenu
        Rectangle {
            width:  modesCol.width   + ScreenTools.defaultFontPixelWidth  * 3
            height: modesCol.height  + ScreenTools.defaultFontPixelHeight * 2
            radius: ScreenTools.defaultFontPixelHeight * 0.5
            color:  qgcPal.window
            border.color:   qgcPal.text
            Column {
                id:                 modesCol
                spacing:            ScreenTools.defaultFontPixelHeight * 0.5
                width:              ScreenTools.defaultFontPixelWidth * 14
                visible:            _activeVehicle
                anchors.margins:    ScreenTools.defaultFontPixelHeight
                anchors.centerIn:   parent
                QGCLabel {
                    text:   qsTr("Flight Modes")
                    anchors.horizontalCenter: parent.horizontalCenter
                }
                Repeater {
                    model:          _flightModes
                    QGCButton {
                        text:       modelData
                        width:      ScreenTools.defaultFontPixelWidth * 14
                        onClicked: {
                            if(_activeVehicle) {
                                _activeVehicle.flightMode = modelData
                                if(mainWindow.currentPopUp) {
                                    mainWindow.currentPopUp.close();
                                }
                            }
                        }
                    }
                }
            }
            Component.onCompleted: {
                var pos = mapFromItem(toolBar, centerX - (width / 2), toolBar.height)
                x = pos.x
                y = pos.y + ScreenTools.defaultFontPixelHeight
            }
        }
    }

    QGCLabel {
        text:                   _activeVehicle ? (QGroundControl.multiVehicleManager.parameterReadyVehicleAvailable ? _activeVehicle.flightMode : qsTr("Waiting For Vehicle")) : qsTr("Not Connected")
        font.pointSize:         ScreenTools.mediumFontPointSize
        color:                  qgcPal.buttonText
        anchors.centerIn:       parent
    }

    MouseArea {
        anchors.fill:       parent
        onClicked: {
            if(_activeVehicle && QGroundControl.multiVehicleManager.parameterReadyVehicleAvailable) {
                var centerX = mapToItem(toolBar, x, y).x + (width / 2)
                mainWindow.showPopUp(modeMenu, centerX)
            }
        }
    }

}
