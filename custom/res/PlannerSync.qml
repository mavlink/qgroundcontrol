/*!
 * @file
 * @brief Desktop/ST16 Sync
 * @author Gus Grubba <mavlink@grubba.com>
 */

import QtQuick                  2.5
import QtQuick.Controls         1.2
import QtQuick.Controls.Styles  1.2
import QtQuick.Dialogs          1.2
import QtQuick.Layouts          1.2
import QtGraphicalEffects       1.0

import QGroundControl                       1.0
import QGroundControl.Controllers           1.0
import QGroundControl.Controls              1.0
import QGroundControl.MultiVehicleManager   1.0
import QGroundControl.Palette               1.0
import QGroundControl.ScreenTools           1.0
import QGroundControl.Vehicle               1.0

import TyphoonHQuickInterface               1.0

QGCView {
    id:                 qgcView
    viewPanel:          panel
    color:              qgcPal.window
    anchors.fill:       parent
    anchors.margins:    ScreenTools.defaultFontPixelWidth

    property var    _activeVehicle:     QGroundControl.multiVehicleManager.activeVehicle
    property int    _clientCount:       TyphoonHQuickInterface.clientList.length
    property real   _buttonWidth:       ScreenTools.defaultFontPixelWidth * 16

    QGCPalette      { id: qgcPal }

    QGCViewPanel {
        id:             panel
        anchors.fill:   parent
        QGCLabel {
            text:           qsTr("Cannot sync while connected to vehicle")
            font.family:    ScreenTools.demiboldFontFamily
            font.pointSize: ScreenTools.mediumFontPointSize
            visible:        _activeVehicle
            anchors.centerIn: parent
        }
        QGCLabel {
            text:           qsTr("No remotes detected")
            font.family:    ScreenTools.demiboldFontFamily
            font.pointSize: ScreenTools.mediumFontPointSize
            visible:        !_activeVehicle && !_clientCount
            anchors.centerIn: parent
        }
        Column {
            spacing:        ScreenTools.defaultFontPixelHeight
            visible:        !_activeVehicle && _clientCount
            anchors.top:    parent.top
            anchors.topMargin: ScreenTools.defaultFontPixelHeight
            QGCLabel {
                text:       qsTr("Select Remote")
                anchors.horizontalCenter: parent.horizontalCenter
            }
            QGCComboBox {
                id:             remoteCombo
                model:          TyphoonHQuickInterface.clientList
                width:          _buttonWidth
                currentIndex:   -1
                anchors.horizontalCenter: parent.horizontalCenter
            }
            QGCButton {
                text:           qsTr("Connect")
                width:          _buttonWidth
                primary:        true
                enabled:        remoteCombo.currentIndex >= 0 && remoteCombo.currentIndex < _clientCount
                anchors.horizontalCenter: parent.horizontalCenter
                onClicked: {
                    TyphoonHQuickInterface.connectToNode(TyphoonHQuickInterface.clientList[remoteCombo.currentIndex])
                }
            }
        }
    }
}
