/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

import QGroundControl
import QGroundControl.Controls
import QGroundControl.Palette
import QGroundControl.MultiVehicleManager
import QGroundControl.ScreenTools
import QGroundControl.Controllers

Rectangle {
    id:     _root
    width:  parent.width
    height: ScreenTools.toolbarHeight
    color:  "transparent"

    property var    planMasterController
    property Item   backdropSource

    property var    _activeVehicle:         QGroundControl.multiVehicleManager.activeVehicle
    property real   _controllerProgressPct: planMasterController.missionController.progressPct
    
    QGCPalette { id: qgcPal }

    GlassBackdrop {
        anchors.fill:       parent
        sourceItem:         _root.backdropSource
        targetItem:         _root
        sampleAtItemPosition: false
        sampleY:            0
        sourceScale:        0.42
        blurAmount:         0.84
        blurMax:            36
        sourceBrightness:   -0.05
        sourceSaturation:   0.54
        tintColor:          Qt.rgba(0.045, 0.048, 0.052, 0.80)
        sheenColor:         "transparent"
    }

    /// Bottom single pixel divider
    Rectangle {
        anchors.left:   parent.left
        anchors.right:  parent.right
        anchors.bottom: parent.bottom
        height:         1
        color:          Qt.rgba(0.82, 0.88, 0.94, 0.035)
    }

    RowLayout {
        id:                     viewButtonRow
        anchors.bottomMargin:   1
        anchors.top:            parent.top
        anchors.bottom:         parent.bottom
        spacing:                ScreenTools.defaultFontPixelWidth / 2

        QGCLabel {
            font.pointSize: ScreenTools.largeFontPointSize
            text:           "<"
        }

        QGCLabel {
            text:           qsTr("Exit Plan")
            font.pointSize: ScreenTools.largeFontPointSize
        }
    }

    QGCMouseArea {
        anchors.fill:   viewButtonRow
        onClicked:      mainWindow.showFlyView()
    }

    QGCFlickable {
        id:                     toolsFlickable
        //anchors.leftMargin:     ScreenTools.defaultFontPixelWidth * ScreenTools.largeFontPointRatio * 1.5
        anchors.left:           viewButtonRow.right
        anchors.bottomMargin:   1
        anchors.top:            parent.top
        anchors.bottom:         parent.bottom
        anchors.right:          parent.right
        contentWidth:           toolIndicators.width
        flickableDirection:     Flickable.HorizontalFlick

        PlanToolBarIndicators {
            id:                     toolIndicators
            anchors.top:            parent.top
            anchors.bottom:         parent.bottom
            planMasterController:   _root.planMasterController
        }
    }

    // Small mission download progress bar
    Rectangle {
        id:             progressBar
        anchors.left:   parent.left
        anchors.bottom: parent.bottom
        height:         4
        width:          _controllerProgressPct * parent.width
        color:          qgcPal.primaryButton
        visible:        false

        onVisibleChanged: {
            if (visible) {
                largeProgressBar._userHide = false
            }
        }
    }

    // Large mission download progress bar
    Rectangle {
        id:             largeProgressBar
        anchors.bottom: parent.bottom
        anchors.left:   parent.left
        anchors.right:  parent.right
        height:         parent.height
        color:          Qt.rgba(0.045, 0.048, 0.052, 0.96)
        visible:        _showLargeProgress

        property bool _userHide:                false
        property bool _showLargeProgress:       progressBar.visible && !_userHide && qgcPal.globalTheme === QGCPalette.Light

        Connections {
            target:                 QGroundControl.multiVehicleManager
            onActiveVehicleChanged: largeProgressBar._userHide = false
        }

        Rectangle {
            anchors.top:    parent.top
            anchors.bottom: parent.bottom
            width:          _controllerProgressPct * parent.width
            color:          qgcPal.primaryButton
        }

        QGCLabel {
            anchors.centerIn:   parent
            text:               qsTr("Syncing Mission")
            font.pointSize:     ScreenTools.largeFontPointSize
            visible:            _controllerProgressPct !== 1
        }

        QGCLabel {
            anchors.centerIn:   parent
            text:               qsTr("Done")
            font.pointSize:     ScreenTools.largeFontPointSize
            visible:            _controllerProgressPct === 1
        }

        QGCLabel {
            anchors.margins:    _margin
            anchors.right:      parent.right
            anchors.bottom:     parent.bottom
            text:               qsTr("Click anywhere to hide")

            property real _margin: ScreenTools.defaultFontPixelWidth / 2
        }

        MouseArea {
            anchors.fill:   parent
            onClicked:      largeProgressBar._userHide = true
        }
    }
    // Progress bar
    Connections {
        target: planMasterController.missionController

        onProgressPctChanged: {
            if (_controllerProgressPct === 1) {
                if (_root.visible) {
                    resetProgressTimer.start()
                } else {
                    progressBar.visible = false
                }
            } else if (_controllerProgressPct > 0) {
                progressBar.visible = true
            }
        }
    }

    Timer {
        id:             resetProgressTimer
        interval:       3000
        onTriggered:    progressBar.visible = false
    }
}
