/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.12
import QtQuick.Controls 2.4
import QtQuick.Layouts  1.12

import QGroundControl               1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controls      1.0
import QGroundControl.Palette       1.0

Rectangle {
    id:         _root
    width:      ScreenTools.defaultFontPixelWidth * 45
    height:     mainLayout.height + (_margins * 2)
    radius:     ScreenTools.defaultFontPixelWidth / 2
    color:      qgcPal.window
    visible:    false

    property var    guidedController
    property var    guidedValueSlider
    property string title                                       // Currently unused
    property alias  message:            messageText.text
    property int    action
    property var    actionData
    property bool   hideTrigger:        false
    property var    mapIndicator
    property alias  optionText:         optionCheckBox.text
    property alias  optionChecked:      optionCheckBox.checked

    property real _margins:         ScreenTools.defaultFontPixelWidth / 2
    property bool _emergencyAction: action === guidedController.actionEmergencyStop

    Component.onCompleted: guidedController.confirmDialog = this

    onVisibleChanged: {
        if (visible) {
            slider.focus = true
        }
    }

    onHideTriggerChanged: {
        if (hideTrigger) {
            confirmCancelled()
        }
    }

    function show(immediate) {
        if (immediate) {
            visible = true
        } else {
            // We delay showing the confirmation for a small amount in order for any other state
            // changes to propogate through the system. This way only the final state shows up.
            visibleTimer.restart()
        }
    }

    function confirmCancelled() {
        guidedValueSlider.visible = false
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

    ColumnLayout {
        id:                 mainLayout
        anchors.margins:    _margins
        anchors.left:       parent.left
        anchors.right:      parent.right
        spacing:            _margins

        QGCLabel {
            id:                     messageText
            Layout.fillWidth:       true
            horizontalAlignment:    Text.AlignHCenter
            wrapMode:               Text.WordWrap
            font.pointSize:         ScreenTools.mediumFontPointSize
        }

        QGCCheckBox {
            id:                 optionCheckBox
            Layout.alignment:   Qt.AlignHCenter
            text:               ""
            visible:            text !== ""
        }

        RowLayout {
            Layout.fillWidth:   true
            spacing:            ScreenTools.defaultFontPixelWidth

            SliderSwitch {
                id:                 slider
                confirmText:        ScreenTools.isMobile ? qsTr("Slide to confirm") : qsTr("Slide or hold spacebar")
                Layout.fillWidth:   true

                onAccept: {
                    _root.visible = false
                    var sliderOutputValue = 0
                    if (guidedValueSlider.visible) {
                        sliderOutputValue = guidedValueSlider.getOutputValue()
                        guidedValueSlider.visible = false
                    }
                    hideTrigger = false
                    guidedController.executeAction(_root.action, _root.actionData, sliderOutputValue, _root.optionChecked)
                    if (mapIndicator) {
                        mapIndicator.actionConfirmed()
                        mapIndicator = undefined
                    }
                }
            }

            Rectangle {
                height: slider.height * 0.75
                width:  height
                radius: height / 2
                color:  qgcPal.primaryButton

                QGCColoredImage {
                    anchors.margins:    parent.height / 4
                    anchors.fill:       parent
                    source:             "/res/XDelete.svg"
                    fillMode:           Image.PreserveAspectFit
                    color:              qgcPal.text
                }

                QGCMouseArea {
                    fillItem:   parent
                    onClicked:  confirmCancelled()
                }
            }
        }
    }
}

