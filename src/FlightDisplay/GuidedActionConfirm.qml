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

import QGroundControl

import QGroundControl.Controls

import QGroundControl.UTMSP

Rectangle {
    id:         control
    width:      mainLayout.width + (_margins * 2)
    height:     mainLayout.height + (_margins * 2)
    radius:     ScreenTools.defaultFontPixelWidth / 2
    color:      qgcPal.window
    visible:    _utmspEnabled === true ? utmspSliderTrigger: false

    property var    guidedController
    property var    guidedValueSlider
    property string title
    property string message
    property int    action
    property var    actionData
    property bool   hideTrigger:        false
    property var    mapIndicator
    property alias  optionText:         optionCheckBox.text
    property alias  optionChecked:      optionCheckBox.checked

    property real _margins:         ScreenTools.defaultFontPixelHeight / 2
    property bool _emergencyAction: action === guidedController.actionEmergencyStop

    // Properties of UTM adapter
    property bool   utmspSliderTrigger
    property bool   _utmspEnabled:                       QGroundControl.utmspSupported

    Component.onCompleted: guidedController.confirmDialog = this

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
        id:         mainLayout
        x:          control._margins
        y:          control._margins
        spacing:    control._margins

        QGCLabel {
            Layout.fillWidth:       true
            Layout.leftMargin:      closeButton.width + closeButton.anchors.rightMargin
            Layout.rightMargin:     Layout.leftMargin
            text:                   control.message
            horizontalAlignment:    Text.AlignHCenter
        }

        QGCCheckBox {
            id:                 optionCheckBox
            Layout.alignment:   Qt.AlignHCenter
            text:               ""
            visible:            text !== ""
        }

        QGCDelayButton {
            Layout.fillWidth:   true
            text:               control.title
            enabled:            _utmspEnabled === true? utmspSliderTrigger : true
            opacity:            if(_utmspEnabled){utmspSliderTrigger === true ? 1 : 0.5} else{1}

            onActivated: {
                control.visible = false
                var sliderOutputValue = 0
                if (guidedValueSlider.visible) {
                    sliderOutputValue = guidedValueSlider.getOutputValue()
                    guidedValueSlider.visible = false
                }
                hideTrigger = false
                guidedController.executeAction(control.action, control.actionData, sliderOutputValue, control.optionChecked)
                if (mapIndicator) {
                    mapIndicator.actionConfirmed()
                    mapIndicator = undefined
                }

                UTMSPStateStorage.indicatorOnMissionStatus = true
                UTMSPStateStorage.currentNotificationIndex = 7
                UTMSPStateStorage.currentStateIndex = 3
            }
        }
    }

    QGCColoredImage {
        id:                     closeButton
        anchors.topMargin:      _margins / 2
        anchors.rightMargin:    _margins / 2
        anchors.top:            parent.top
        anchors.right:          parent.right
        height:                 ScreenTools.defaultFontPixelHeight * 0.5
        width:                  height
        source:                 "/res/XDelete.svg"
        fillMode:               Image.PreserveAspectFit
        color:                  qgcPal.text

        QGCMouseArea {
            fillItem:   parent
            onClicked:  confirmCancelled()
        }
    }
}
