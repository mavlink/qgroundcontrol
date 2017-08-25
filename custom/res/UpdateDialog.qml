/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick              2.3
import QtQuick.Controls     1.2
import QtGraphicalEffects   1.0

import QGroundControl                       1.0
import QGroundControl.Controls              1.0
import QGroundControl.MultiVehicleManager   1.0
import QGroundControl.ScreenTools           1.0
import QGroundControl.Palette               1.0

import TyphoonHQuickInterface               1.0

Item {
    id: dlgRoot
    anchors.fill: parent
    QGCPalette { id: qgcPal; colorGroupEnabled: true }
    MouseArea {
        anchors.fill:   parent
        onWheel:        { wheel.accepted = true; }
        onPressed:      { mouse.accepted = true; }
        onReleased:     { mouse.accepted = true; }
    }
    Rectangle {
        id:             updateDialogShadow
        anchors.fill:   updateDialogRect
        radius:         updateDialogRect.radius
        color:          qgcPal.window
        visible:        false
    }
    DropShadow {
        anchors.fill:       updateDialogShadow
        visible:            updateDialogRect.visible
        horizontalOffset:   4
        verticalOffset:     4
        radius:             32.0
        samples:            65
        color:              Qt.rgba(0,0,0,0.75)
        source:             updateDialogShadow
    }
    Rectangle {
        id:             updateDialogRect
        width:          mainWindow.width   * 0.65
        height:         updateDialogCol.height * 1.2
        radius:         ScreenTools.defaultFontPixelWidth
        color:          qgcPal.alertBackground
        border.color:   qgcPal.alertBorder
        border.width:   2
        anchors.centerIn: parent
        Column {
            id:                 updateDialogCol
            width:              updateDialogRect.width
            spacing:            ScreenTools.defaultFontPixelHeight * 3
            anchors.margins:    ScreenTools.defaultFontPixelHeight
            anchors.centerIn:   parent
            QGCLabel {
                text:           qsTr("Software Update")
                font.family:    ScreenTools.demiboldFontFamily
                font.pointSize: ScreenTools.largeFontPointSize
                color:          qgcPal.alertText
                anchors.horizontalCenter: parent.horizontalCenter
            }
            QGCLabel {
                text:           qsTr("\
Thanks for being a Yuneec customer. We put safety first. Before using the product, \
please make sure you upgrade to the latest software versions. This is very important \
to ensure that you have all the latest fixes for the best possible user experience.")
                color:          qgcPal.alertText
                width:          updateDialogRect.width * 0.75
                wrapMode:       Text.WordWrap
                font.family:    ScreenTools.demiboldFontFamily
                font.pointSize: ScreenTools.largeFontPointSize
                horizontalAlignment: Text.AlignJustify
                anchors.horizontalCenter: parent.horizontalCenter
            }
            QGCLabel {
                text:           qsTr("Updater app not installed.")
                color:          qgcPal.alertText
                width:          updateDialogRect.width * 0.75
                visible:        !TyphoonHQuickInterface.isUpdaterApp
                font.family:    ScreenTools.demiboldFontFamily
                font.pointSize: ScreenTools.largeFontPointSize
                horizontalAlignment: Text.AlignHCenter
                anchors.horizontalCenter: parent.horizontalCenter
            }
            Row {
                spacing:        ScreenTools.defaultFontPixelWidth * 4
                anchors.horizontalCenter: parent.horizontalCenter
                QGCButton {
                    text:           qsTr("Update")
                    width:          ScreenTools.defaultFontPixelWidth  * 16
                    height:         ScreenTools.defaultFontPixelHeight * 2
                    visible:        TyphoonHQuickInterface.isUpdaterApp
                    onClicked: {
                        TyphoonHQuickInterface.launchUpdater()
                        mainWindow.enableToolbar()
                        rootLoader.sourceComponent = null
                    }
                }
                QGCButton {
                    text:           TyphoonHQuickInterface.isUpdaterApp ? qsTr("Cancel") : qsTr("Close")
                    width:          ScreenTools.defaultFontPixelWidth  * 16
                    height:         ScreenTools.defaultFontPixelHeight * 2
                    onClicked: {
                        mainWindow.enableToolbar()
                        rootLoader.sourceComponent = null
                    }
                }
            }
        }
    }
    Component.onCompleted: {
        mainWindow.disableToolbar()
    }
}
