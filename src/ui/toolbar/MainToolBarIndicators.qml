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
import QtQuick.Layouts  1.2

import QGroundControl                       1.0
import QGroundControl.Controls              1.0
import QGroundControl.MultiVehicleManager   1.0
import QGroundControl.ScreenTools           1.0
import QGroundControl.Palette               1.0

Item {
    property var  _activeVehicle:        QGroundControl.multiVehicleManager.activeVehicle
    property bool _communicationLost:    _activeVehicle ? _activeVehicle.connectionLost : false

    QGCPalette { id: qgcPal }


    QGCLabel {
        id:                     waitForVehicle
        anchors.verticalCenter: parent.verticalCenter
        text:                   qsTr("Waiting For Vehicle Connection")
        font.pointSize:         ScreenTools.mediumFontPointSize
        font.family:            ScreenTools.demiboldFontFamily
        color:                  colorRed
        visible:                !_activeVehicle
    }

    Row {
        id:             indicatorRow
        anchors.top:    parent.top
        anchors.bottom: parent.bottom
        spacing:        ScreenTools.defaultFontPixelWidth * 1.5
        visible:        _activeVehicle && !_communicationLost

        Repeater {
            model:      _activeVehicle ? _activeVehicle.toolBarIndicators : []
            Loader {
                anchors.top:    parent.top
                anchors.bottom: parent.bottom
                source:         modelData;
            }
        }
    }

    Image {
        anchors.right:          parent.right
        anchors.top:            parent.top
        anchors.bottom:         parent.bottom
        visible:                x > indicatorRow.width && !_communicationLost
        fillMode:               Image.PreserveAspectFit
        source:                 _outdoorPalette ? _brandImageOutdoor : _brandImageIndoor

        property bool   _outdoorPalette:        qgcPal.globalTheme === QGCPalette.Light
        property bool   _corePluginBranding:    QGroundControl.corePlugin.brandImageIndoor.length != 0
        property string _brandImageIndoor:      _corePluginBranding ? QGroundControl.corePlugin.brandImageIndoor : (_activeVehicle ? _activeVehicle.brandImageIndoor : "")
        property string _brandImageOutdoor:     _corePluginBranding ? QGroundControl.corePlugin.brandImageOutdoor : (_activeVehicle ? _activeVehicle.brandImageOutdoor : "")
    }

    Row {
        anchors.fill:       parent
        layoutDirection:    Qt.RightToLeft
        spacing:            ScreenTools.defaultFontPixelWidth
        visible:            _communicationLost

        QGCButton {
            id:                     disconnectButton
            anchors.verticalCenter: parent.verticalCenter
            text:                   qsTr("Disconnect")
            primary:                true
            onClicked:              _activeVehicle.disconnectInactiveVehicle()
        }

        QGCLabel {
            id:                     connectionLost
            anchors.verticalCenter: parent.verticalCenter
            text:                   qsTr("COMMUNICATION LOST")
            font.pointSize:         ScreenTools.largeFontPointSize
            font.family:            ScreenTools.demiboldFontFamily
            color:                  colorRed
        }
    }
}
