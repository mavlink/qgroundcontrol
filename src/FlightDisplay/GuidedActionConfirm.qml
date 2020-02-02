/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick                  2.3
import QtQuick.Controls         1.2

import QGroundControl               1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controls      1.0
import QGroundControl.Palette       1.0

/// Guided actions confirmation dialog
Rectangle {
    id:             _root
    width:          confirmColumn.width  + (_margins * 4)
    height:         confirmColumn.height + (_margins * 4)
    radius:         ScreenTools.defaultFontPixelHeight / 2
    color:          qgcPal.window
    border.color:   _emergencyAction ? "red" : qgcPal.windowShade
    border.width:   _emergencyAction ? 4 : 1
    z:              guidedController.z
    visible:        false

    property var    guidedController
    property var    altitudeSlider
    property alias  title:              titleText.text
    property alias  message:            messageText.text
    property int    action
    property var    actionData
    property bool   hideTrigger:        false
    property var    mapIndicator
    property alias  optionText:         optionCheckBox.text
    property alias  optionChecked:      optionCheckBox.checked

    property real _margins:         ScreenTools.defaultFontPixelWidth
    property bool _emergencyAction: action === guidedController.actionEmergencyStop

    onHideTriggerChanged: {
        if (hideTrigger) {
            confirmCancelled()
        }
    }

    function show(immediate) {
        if (immediate) {
            visible = true
        } else {
            // We delay showing the confirmation for a small amount in order to any other state
            // changes to propogate through the system. This way only the final state shows up.
            visibleTimer.restart()
        }
    }

    function confirmCancelled() {
        altitudeSlider.visible = false
        visible = false
        hideTrigger = false
        visibleTimer.stop()
        if (mapIndicator) {
            mapIndicator.actionCancelled()
            mapIndicator = undefined
        }
    }

    Timer {
        id:             visibleTimer
        interval:       1000
        repeat:         false
        onTriggered:    visible = true
    }

    QGCPalette { id: qgcPal }

    DeadMouseArea {
        anchors.fill: parent
    }

    Column {
        id:                 confirmColumn
        anchors.margins:    _margins
        anchors.centerIn:   parent
        spacing:            _margins

        QGCLabel {
            id:                     titleText
            anchors.left:           slider.left
            anchors.right:          slider.right
            horizontalAlignment:    Text.AlignHCenter
            font.pointSize:         ScreenTools.largeFontPointSize
        }

        QGCLabel {
            id:                     messageText
            anchors.left:           slider.left
            anchors.right:          slider.right
            horizontalAlignment:    Text.AlignHCenter
            wrapMode:               Text.WordWrap
        }

        QGCCheckBox {
            id:                         optionCheckBox
            anchors.horizontalCenter:   parent.horizontalCenter
            text:                       ""
            visible:                    text !== ""
        }

        // Action confirmation control
        SliderSwitch {
            id:             slider
            confirmText:    qsTr("Slide to confirm")
            width:          Math.max(implicitWidth, ScreenTools.defaultFontPixelWidth * 30)

            onAccept: {
                _root.visible = false
                var altitudeChange = 0
                if (altitudeSlider.visible) {
                    altitudeChange = altitudeSlider.getAltitudeChangeValue()
                    altitudeSlider.visible = false
                }
                hideTrigger = false
                guidedController.executeAction(_root.action, _root.actionData, altitudeChange, _root.optionChecked)
                if (mapIndicator) {
                    mapIndicator.actionConfirmed()
                    mapIndicator = undefined
                }
            }
        }
    }

    QGCColoredImage {
        anchors.margins:    _margins
        anchors.top:        parent.top
        anchors.right:      parent.right
        width:              ScreenTools.defaultFontPixelHeight
        height:             width
        sourceSize.height:  width
        source:             "/res/XDelete.svg"
        fillMode:           Image.PreserveAspectFit
        color:              qgcPal.text

        QGCMouseArea {
            fillItem:   parent
            onClicked:  confirmCancelled()
        }
    }
}
