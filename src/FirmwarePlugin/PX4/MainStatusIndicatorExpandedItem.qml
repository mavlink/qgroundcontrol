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

Column {
    width:      ScreenTools.defaultFontPixelWidth * 80
    spacing:    margins / 2

    property var  activeVehicle: QGroundControl.multiVehicleManager.activeVehicle

    property Fact mpcLandSpeedFact:         controller.getParameterFact(-1, "MPC_LAND_SPEED", false)
    property Fact precisionLandingFact:     controller.getParameterFact(-1, "RTL_PLD_MD", false)
    property Fact sys_vehicle_resp:         controller.getParameterFact(-1, "SYS_VEHICLE_RESP", false)
    property Fact mpc_xy_vel_all:           controller.getParameterFact(-1, "MPC_XY_VEL_ALL", false)
    property Fact mpc_z_vel_all:            controller.getParameterFact(-1, "MPC_Z_VEL_ALL", false)
    property var  qgcPal:                   QGroundControl.globalPalette
    property real margins:                  ScreenTools.defaultFontPixelHeight

    FactPanelController { id: controller }

    RowLayout {
        width: parent.width

        QGCLabel { Layout.fillWidth: true; text: qsTr("Vehicle Parameters") }
        QGCButton {
            text: qsTr("Configure")
            onClicked: {                            
                mainWindow.showVehicleSetupTool(qsTr("Parameters"))
                drawer.close()
            }
        }
    }

    RowLayout {
        width: parent.width

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
