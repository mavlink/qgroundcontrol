/****************************************************************************
 *
 * (c) 2009-2022 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts  1.11

import QGroundControl                       1.0
import QGroundControl.Controls              1.0
import QGroundControl.MultiVehicleManager   1.0
import QGroundControl.ScreenTools           1.0
import QGroundControl.Palette               1.0
import QGroundControl.FactSystem            1.0
import QGroundControl.FactControls          1.0

IndicatorPageGroupLayout {
    spacing:        ScreenTools.defaultFontPixelHeight / 2
    showDivider:    false

    GridLayout {
        columns:        2
        rowSpacing:     ScreenTools.defaultFontPixelHeight / 2
        columnSpacing:  ScreenTools.defaultFontPixelWidth *2

        QGCLabel { Layout.fillWidth: true; text: qsTr("Vehicle Parameters") }
        QGCButton {
            text: qsTr("Configure")
            onClicked: {                            
                mainWindow.showVehicleSetupTool(qsTr("Parameters"))
                drawer.close()
            }
        }

        QGCLabel { Layout.fillWidth: true; text: qsTr("Initial Vehicle Setup") }
        QGCButton {
            text: qsTr("Configure")
            onClicked: {                            
                mainWindow.showVehicleSetupTool()
                drawer.close()
            }
        }
    }
}
