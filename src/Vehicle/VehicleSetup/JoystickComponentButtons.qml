import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.VehicleSetup
import QGroundControl.FactControls

ColumnLayout {
    id: root

    required property var joystick
    required property var controller

    property int _maxButtons: 64

    property var _draft: null
    property bool _justSaved: false
    readonly property int _button: buttonIndexCombo.currentIndex
    readonly property var _actions: joystick?.assignableActionTitles ?? []
    readonly property var _assignments: joystick ? joystick.buttonActions.map((action, button) => ({
            button: button,
            action: action,
            repeat: action !== joystick.buttonActionNone && joystick.getButtonRepeat(button)
        })) : []
    readonly property var _saved: _assignments[_button]
    readonly property string _action: _draft?.action ?? _saved?.action ?? joystick?.buttonActionNone ?? ""
    readonly property bool _repeat: _draft?.repeat ?? _saved?.repeat ?? false
    readonly property var _assignedButtons: _assignments.filter(binding => binding.action !== joystick?.buttonActionNone)

    function _edit(action, repeat) {
        _justSaved = false
        _draft = action === _saved?.action && repeat === _saved?.repeat ? null : { action: action, repeat: repeat }
    }

    function _save() {
        const button = _button
        const draft = _draft
        if (!_saved || !draft) {
            return
        }
        _draft = null
        if (draft.action !== _saved.action) {
            joystick.setButtonAction(button, draft.action)
        }
        if (draft.action !== joystick.buttonActionNone && draft.repeat !== joystick.getButtonRepeat(button)) {
            joystick.setButtonRepeat(button, draft.repeat)
        }
        _justSaved = true
    }

    on_ButtonChanged: _resetEditor()
    onJoystickChanged: _resetEditor()

    function _resetEditor() {
        _draft = null
        _justSaved = false
    }

    spacing: ScreenTools.defaultFontPixelHeight / 2

    QGCLabel {
        Layout.fillWidth: true
        wrapMode: Text.WordWrap
        text: qsTr("Multiple buttons that have the same action must be pressed simultaneously to invoke the action.")
    }

    RowLayout {
        Layout.fillWidth: true
        visible: buttonIndexCombo.count > 0

        QGCLabel { text: qsTr("Button") }

        QGCComboBox {
            id: buttonIndexCombo
            objectName: "joystickButtonNumber"
            model: root.joystick?.buttonCount ?? 0
            enabled: !root._draft
            Accessible.name: qsTr("Button number")
        }

        QGCComboBox {
            id: buttonActionCombo
            objectName: "joystickButtonAction"
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            model: root._actions
            sizeToContents: true
            Accessible.name: qsTr("Button action")
            currentIndex: root._actions.indexOf(root._action)
            alternateText: currentIndex < 0 ? root._action : ""

            onActivated: index => {
                const repeatable = !!root.joystick.assignableActions.get(index)?.canRepeat
                root._edit(textAt(index), repeatable && root._repeat)
            }
        }
    }

    QGCCheckBox {
        objectName: "joystickButtonRepeat"
        text: qsTr("Repeat")
        visible: root._button >= 0
        checked: root._repeat
        enabled: buttonActionCombo.currentIndex >= 0 &&
                 !!root.joystick.assignableActions.get(buttonActionCombo.currentIndex)?.canRepeat
        onClicked: root._edit(root._action, checked)
    }

    RowLayout {
        visible: root._button >= 0

        QGCButton {
            objectName: "joystickButtonSave"
            text: qsTr("Save")
            primary: enabled
            enabled: !!root._saved && !!root._draft && buttonActionCombo.currentIndex >= 0
            onClicked: root._save()
        }

        QGCButton {
            objectName: "joystickButtonCancel"
            text: qsTr("Cancel")
            enabled: !!root._draft
            onClicked: root._draft = null
        }

        QGCButton {
            objectName: "joystickButtonRemove"
            text: qsTr("Remove")
            Accessible.name: qsTr("Remove button assignment")
            enabled: !!root._saved && root._action !== root.joystick.buttonActionNone
            onClicked: root._edit(root.joystick.buttonActionNone, false)
        }
    }

    QGCLabel {
        objectName: "joystickButtonSaveStatus"
        visible: !!root._draft || root._justSaved
        text: root._draft ? qsTr("Unsaved changes") : qsTr("Saved")
    }

    QGCLabel {
        visible: root._assignedButtons.length > 0
        text: qsTr("Saved assignments")
        font.bold: true
    }

    Repeater {
        model: root._assignedButtons

        RowLayout {
            required property var modelData
            Layout.fillWidth: true

            QGCLabel { text: qsTr("Button %1").arg(modelData.button) }
            QGCLabel {
                Layout.fillWidth: true
                text: modelData.repeat ? qsTr("%1 (repeat)").arg(modelData.action) : modelData.action
                elide: Text.ElideRight
            }
            QGCButton {
                objectName: "joystickButtonEdit" + modelData.button
                text: qsTr("Edit")
                Accessible.name: qsTr("Edit button %1").arg(modelData.button)
                enabled: !root._draft
                onClicked: {
                    buttonIndexCombo.currentIndex = modelData.button
                    buttonActionCombo.forceActiveFocus()
                }
            }
        }
    }

    /*Column {
        id:         buttonCol
        Layout.fillWidth: true
        visible:    globals.activeVehicle.supports.jsButton
        spacing:    ScreenTools.defaultFontPixelHeight / 3

        Row {
            spacing: ScreenTools.defaultFontPixelWidth
            QGCLabel {
                horizontalAlignment: Text.AlignHCenter
                width: ScreenTools.defaultFontPixelHeight * 1.5
                text: qsTr("#")
            }
            QGCLabel {
                width: ScreenTools.defaultFontPixelWidth * 26
                text: qsTr("Function: ")
            }
            QGCLabel {
                width: ScreenTools.defaultFontPixelWidth * 26
                visible: globals.activeVehicle.supports.jsButton
                text: qsTr("Shift Function: ")
            }
        }
        Repeater {
            id: jsButtonActionRepeater
            model: joystick ? Math.min(joystick.buttonCount, _maxButtons) : 0

            Row {
                spacing: ScreenTools.defaultFontPixelWidth
                visible: globals.activeVehicle.supports.jsButton
                property var parameterName: `BTN${index}_FUNCTION`
                property var parameterShiftName: `BTN${index}_SFUNCTION`
                property bool hasFirmwareSupport: controller.parameterExists(-1, parameterName)

                property bool pressed
                property var currentAssignableAction: joystick ? joystick.assignableActions.get(buttonActionCombo.currentIndex) : null

                Rectangle {
                    anchors.verticalCenter: parent.verticalCenter
                    width: ScreenTools.defaultFontPixelHeight * 1.5
                    height: width
                    border.width: 1
                    border.color: qgcPal.textPrimary
                    color: pressed ? qgcPal.accent : qgcPal.windowCard


                    QGCLabel {
                        anchors.fill: parent
                        color: pressed ? qgcPal.accentText : qgcPal.textPrimary
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        text: modelData
                    }
                }

                QGCComboBox {
                    id: buttonActionCombo
                    width: ScreenTools.defaultFontPixelWidth * 26
                    property Fact fact: controller.parameterExists(-1, parameterName) ? controller.getParameterFact(-1, parameterName) : null
                    property Fact fact_shift: controller.parameterExists(-1, parameterShiftName) ? controller.getParameterFact(-1, parameterShiftName) : null
                    property var factOptions: fact ? fact.enumStrings : [];
                    property var qgcActions: joystick.assignableActionTitles.filter(
                        function (s) {
                            return [
                                s.includes("Camera"),
                                s.includes("Stream"),
                                s.includes("Stream"),
                                s.includes("Zoom"),
                                s.includes("Gimbal"),
                                s.includes("No Action")
                            ].some(Boolean)
                        }
                    )

                    model: [...qgcActions, ...factOptions]
                    property var isFwAction: currentIndex >= qgcActions.length
                    sizeToContents: true

                    function _findCurrentButtonAction() {
                        // Find the index in the dropdown of the current action, checks FW and QGC actions
                        if (joystick) {
                            if (fact && fact.value > 0) {
                                // This is a firmware function
                                currentIndex = qgcActions.length + fact.enumIndex
                                // For sanity reasons, make sure qgc is set to "no action" if the firmware is set to do something
                                joystick.setButtonAction(modelData, "No Action")
                            } else {
                                // If there is not firmware function, check QGC ones
                                currentIndex = find(joystick.buttonActions[modelData])
                            }
                        }
                    }

                    Component.onCompleted: _findCurrentButtonAction()
                    onModelChanged: _findCurrentButtonAction()
                    onActivated: function (optionIndex) {
                        var func = textAt(optionIndex)
                        if (factOptions.indexOf(func) > -1) {
                            // This is a FW action, set parameter to the action and set QGC's handler to No Action
                            fact.enumStringValue = func
                            joystick.setButtonAction(modelData, "No Action")
                        } else {
                            // This is a QGC action, set parameters to Disabled and QGC to the desired action
                            joystick.setButtonAction(modelData, func)
                            fact.value = 0
                            fact_shift.value = 0
                        }
                    }
                }
                QGCCheckBox {
                    id: repeatCheck
                    text: qsTr("Repeat")
                    enabled: currentAssignableAction && joystick.calibrated && currentAssignableAction.canRepeat
                    visible: !globals.activeVehicle.supports.jsButton

                    onClicked: {
                        joystick.setButtonRepeat(modelData, checked)
                    }
                    Component.onCompleted: {
                        if (joystick) {
                            checked = joystick.getButtonRepeat(modelData)
                        }
                    }
                    anchors.verticalCenter: parent.verticalCenter
                }
                Item {
                    width: ScreenTools.defaultFontPixelWidth * 2
                    height: 1
                }

                FactComboBox {
                    id: shiftJSButtonActionCombo
                    width: ScreenTools.defaultFontPixelWidth * 26
                    fact: controller.parameterExists(-1, parameterShiftName) ? controller.getParameterFact(-1, parameterShiftName) : null;
                    indexModel: false
                    visible: buttonActionCombo.isFwAction
                    sizeToContents: true
                }

                QGCLabel {
                    text: qsTr("QGC functions do not support shift actions")
                    width: ScreenTools.defaultFontPixelWidth * 15
                    visible: hasFirmwareSupport && !buttonActionCombo.isFwAction
                    anchors.verticalCenter: parent.verticalCenter
                }
                QGCLabel {
                    text: qsTr("No firmware support")
                    width: ScreenTools.defaultFontPixelWidth * 15
                    visible: !hasFirmwareSupport
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
        }
    }*/
}
