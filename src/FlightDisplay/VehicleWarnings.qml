/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick

import QGroundControl

import QGroundControl.Controls

Rectangle {
    anchors.margins:    -ScreenTools.defaultFontPixelHeight
    property real _padding: ScreenTools.defaultFontPixelHeight * 1.5
    height:             warningsCol.implicitHeight + (_padding * 2)
    width:              Math.max(warningsCol.implicitWidth, ScreenTools.defaultFontPixelWidth * 50) + (_padding * 2)
    color:              Qt.rgba(QGroundControl.globalPalette.toolbarBackground.r,
                                QGroundControl.globalPalette.toolbarBackground.g,
                                QGroundControl.globalPalette.toolbarBackground.b,
                                0.7)
    border.color:       QGroundControl.globalPalette.brandingBlue
    border.width:       2
    radius:             ScreenTools.defaultFontPixelHeight
    visible:            _noGPSLockVisible || _prearmErrorVisible

    property var  _activeVehicle:       QGroundControl.multiVehicleManager.activeVehicle
    property bool _noGPSLockVisible:    _activeVehicle && _activeVehicle.requiresGpsFix && !_activeVehicle.coordinate.isValid
    property bool _prearmErrorVisible:  _activeVehicle && !_activeVehicle.armed && _activeVehicle.prearmError && !_activeVehicle.healthAndArmingCheckReport.supported

    Column {
        id:         warningsCol
        spacing:    ScreenTools.defaultFontPixelHeight
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: parent._padding

        QGCLabel {
            anchors.horizontalCenter:   parent.horizontalCenter
            visible:                    _noGPSLockVisible
            color:                      QGroundControl.globalPalette.windowTransparentText
            font.pointSize:             ScreenTools.largeFontPointSize
            text:                       qsTr("No GPS Lock for Vehicle")
        }

        QGCLabel {
            anchors.horizontalCenter:   parent.horizontalCenter
            visible:                    _prearmErrorVisible
            color:                      QGroundControl.globalPalette.windowTransparentText
            font.pointSize:             ScreenTools.largeFontPointSize
            text:                       _activeVehicle ? _activeVehicle.prearmError : ""
        }

        QGCLabel {
            anchors.horizontalCenter:   parent.horizontalCenter
            visible:                    _prearmErrorVisible
            width:                      ScreenTools.defaultFontPixelWidth * 50
            horizontalAlignment:        Text.AlignHCenter
            wrapMode:                   Text.WordWrap
            color:                      QGroundControl.globalPalette.windowTransparentText
            font.pointSize:             ScreenTools.largeFontPointSize
            text:                       qsTr("The vehicle has failed a pre-arm check. In order to arm the vehicle, resolve the failure.")
        }
    }
}
