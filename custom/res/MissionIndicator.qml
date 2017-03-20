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
    width:          missionLabel.width

    property var    _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle
    property var    _missionModes:   ["Execute Mission", "Pause Mission", "Resume Mission", "Restart Mission", "Cancel Mission", "End Mission"]

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
                Repeater {
                    model:          _missionModes
                    QGCButton {
                        text:       modelData
                        width:      ScreenTools.defaultFontPixelWidth * 14
                        onClicked: {
                            if(_activeVehicle) {
                                //-- TODO: Do Something
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
        id:                     missionLabel
        text:                   qsTr("Mission")
        font.pointSize:         ScreenTools.mediumFontPointSize
        color:                  qgcPal.buttonText
        anchors.centerIn:       parent
    }

    MouseArea {
        anchors.fill:       parent
        onClicked: {
            if(_activeVehicle) {
                var centerX = mapToItem(toolBar, x, y).x + (width / 2)
                mainWindow.showPopUp(modeMenu, centerX)
            }
        }
    }

}
