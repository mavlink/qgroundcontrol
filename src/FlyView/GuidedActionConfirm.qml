import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

Item {
    id:         control
    width:      mainLayout.width
    visible:    false

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

    function reset() {
        visible = false
        guidedValueSlider.visible = false
        hideTrigger = false
        visibleTimer.stop()
        messageDisplay.opacity = 1.0
        messageFadeTimer.stop()
        messageOpacityAnimation.stop()
    }

    // Cancel the current pending action and notify its map indicator.
    // Pass incomingIndicator when superseding one action with another (e.g. from confirmAction):
    // if the old and new indicator are the same object, actionCancelled() is intentionally skipped
    // so that a show() call made before confirmAction() is not undone (e.g. goto -> goto).
    // Omit incomingIndicator (or pass undefined) for explicit user cancellation via the X button
    // or auto-hide trigger, where the indicator must always be notified.
    function confirmCancelled(incomingIndicator) {
        reset()
        if (mapIndicator && mapIndicator !== incomingIndicator) {
            mapIndicator.actionCancelled()
        }
        mapIndicator = undefined
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
            enabled:            true

            onActivated: {
                control.visible = false
                var sliderOutputValue = 0
                if (guidedValueSlider.visible) {
                    sliderOutputValue = guidedValueSlider.getOutputValue()
                    guidedValueSlider.visible = false
                }
                hideTrigger = false
                let success = guidedController.executeAction(control.action, control.actionData, sliderOutputValue, control.optionChecked)
                if (mapIndicator) {
                    if (success) {
                        mapIndicator.actionConfirmed()
                    } else {
                        mapIndicator.actionCancelled()
                    }
                    mapIndicator = undefined
                }
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
