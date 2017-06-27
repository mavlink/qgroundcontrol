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
import QtQuick.Dialogs  1.2
import QtQuick.Layouts  1.2

import QGroundControl               1.0
import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0

import TyphoonHQuickInterface       1.0

SetupPage {
    id:             gimbalPage
    pageComponent:  pageComponent
    Component {
        id: pageComponent
        Item {
            width:  ScreenTools.defaultFontPixelWidth * 40
            height: innerColumn.height

            FactPanelController { id: controller; factPanel: gimbalPage.viewPanel }

            property var    _activeVehicle:     QGroundControl.multiVehicleManager.activeVehicle

            ColumnLayout {
                id:                         innerColumn
                anchors.horizontalCenter:   parent.horizontalCenter
                spacing:                    ScreenTools.defaultFontPixelHeight
                QGCGroupBox {
                    title:  qsTr("Calibration")
                    Column {
                        spacing:        ScreenTools.defaultFontPixelHeight
                        width:          parent.width
                        QGCButton {
                            text:       "Calibrate Gimbal"
                            enabled:    _activeVehicle
                            onClicked:  TyphoonHQuickInterface.cameraControl.calibrateGimbal()
                        }
                    }
                }
            }
        }
    }
}
