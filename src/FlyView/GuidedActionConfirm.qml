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

Item {
    id:         control
    width:      mainLayout.width
    visible:    _utmspEnabled === true ? utmspSliderTrigger: false

    property var    guidedController
    property var    guidedValueSlider
    property var    messageDisplay
    property string title
    property string message
    property int    action
    property var    actionData
    property bool   hideTrigger:        false
    property var    mapIndicator
    property alias  optionText:         optionCheckBox.text
    property alias  optionChecked:      optionCheckBox.checked

    property real _margins:         2
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
            _reallyShow()
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
        messageDisplay.opacity = 1.0
        messageFadeTimer.stop()
        messageOpacityAnimation.stop()
        if (mapIndicator) {
            mapIndicator.actionCancelled()
            mapIndicator = undefined
        }
    }

    function _reallyShow() {
        visible = true
        messageDisplay.opacity = 1.0
        messageFadeTimer.start()
    }

    Timer {
        id:             visibleTimer
        interval:       1000
        repeat:         false
        onTriggered:    _reallyShow()
    }

    QGCPalette { id: qgcPal }

    RowLayout {
        id:         mainLayout
        y:          2
        height:     parent.height - 4
        spacing:    ScreenTools.defaultFontPixelWidth

        QGCDelayButton {
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

        QGCCheckBox {
            id:                 optionCheckBox
            visible:            text !== ""
        }

        QGCColoredImage {
            id:                 closeButton
            Layout.alignment:   Qt.AlignTop
            width:              height
            height:             ScreenTools.defaultFontPixelHeight * 0.5
            source:             "/res/XDelete.svg"
            fillMode:           Image.PreserveAspectFit
            color:              qgcPal.text

            QGCMouseArea {
                fillItem:   parent
                onClicked:  confirmCancelled()
            }
        }
    }
}
