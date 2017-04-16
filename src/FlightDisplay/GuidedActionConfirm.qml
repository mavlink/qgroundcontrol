/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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
    border.color:   qgcPal.alertBorder
    border.width:   1
    width:          confirmColumn.width  + (_margins * 4)
    height:         confirmColumn.height + (_margins * 4)
    radius:         ScreenTools.defaultFontPixelHeight / 2
    color:          qgcPal.alertBackground
    opacity:        0.9
    z:              guidedController.z
    visible:        false

    property var    guidedController
    property var    altitudeSlider
    property alias  title:              titleText.text
    property alias  message:            messageText.text
    property int    action
    property var    actionData
    property bool   hideTrigger:        false

    property real _margins: ScreenTools.defaultFontPixelWidth

    onHideTriggerChanged: {
        if (hideTrigger) {
            hideTrigger = false
            altitudeSlider.visible = false
            visible = false
        }
    }

    QGCPalette { id: qgcPal }

    Column {
        id:                 confirmColumn
        anchors.margins:    _margins
        anchors.centerIn:   parent
        spacing:            _margins

        QGCLabel {
            id:                     titleText
            color:                  qgcPal.alertText
            anchors.left:           slider.left
            anchors.right:          slider.right
            horizontalAlignment:    Text.AlignHCenter
        }

        QGCLabel {
            id:                     messageText
            color:                  qgcPal.alertText
            anchors.left:           slider.left
            anchors.right:          slider.right
            horizontalAlignment:    Text.AlignHCenter
            wrapMode:               Text.WordWrap
        }

        // Action confirmation control
        SliderSwitch {
            id:             slider
            confirmText:    qsTr("Slide to confirm")
            width:          Math.max(implicitWidth, ScreenTools.defaultFontPixelWidth * 30)

            onAccept: {
                _root.visible = false
                if (altitudeSlider.visible) {
                    _root.actionData = altitudeSlider.getValue()
                    altitudeSlider.visible = false
                }
                hideTrigger = false
                guidedController.executeAction(_root.action, _root.actionData)
            }

            onReject: {
                altitudeSlider.visible = false
                _root.visible = false
                hideTrigger = false
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
        color:              qgcPal.alertText
        QGCMouseArea {
            fillItem:   parent
            onClicked: {
                altitudeSlider.visible = false
                _root.visible = false
            }
        }
    }
}
