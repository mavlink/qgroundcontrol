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
import QtQuick.Dialogs
import QtQuick.Layouts

import QGroundControl.Controls
import QGroundControl.Palette
import QGroundControl.ScreenTools

/// Operator panel for selecting, reviewing and committing an in-flight custom
/// landing. Editing only changes CustomLandingController's local draft.
Rectangle {
    id: _root

    property var controller
    property bool loiterCoordinateValid: false
    property bool landingCoordinateValid: false
    property bool geometryValid: true
    property string geometryError
    property var backdropSourceItem
    property real maximumHeight: parent ? parent.height : 0

    readonly property bool draftComplete: loiterCoordinateValid && landingCoordinateValid
    readonly property bool draftEditable: controller && controller.modeActive && controller.capabilitySupported
                                           && !controller.busy && !controller.planCommitted
    readonly property bool executionAllowed: controller && controller.modeActive && controller.capabilitySupported
                                             && controller.canExecute && draftComplete && geometryValid
                                             && !controller.busy && !controller.planCommitted
    readonly property string instructionText: {
        if (!controller) {
            return qsTr("Waiting for the custom landing controller.")
        }
        if (!controller.modeActive) {
            return qsTr("Waiting for the vehicle to confirm Custom Landing mode.")
        }
        if (!controller.capabilitySupported) {
            return qsTr("Checking whether the vehicle supports Custom Landing.")
        }
        if (controller.planCommitted) {
            return qsTr("The landing plan is committed. Map editing is locked.")
        }
        if (controller.busy) {
            return qsTr("Communicating with the vehicle. Keep the aircraft in Custom Landing mode.")
        }
        if (!loiterCoordinateValid) {
            return qsTr("Click the map to set the loiter descent point.")
        }
        if (!landingCoordinateValid) {
            return qsTr("Click the map to set the vertical landing point.")
        }
        return qsTr("Drag either marker to refine the path, then review and execute.")
    }

    property var _confirmationDialog
    property real _margin: Math.max(8, ScreenTools.defaultFontPixelWidth * 0.8)

    width: Math.max(330, ScreenTools.defaultFontPixelWidth * 36)
    height: Math.min(contentColumn.implicitHeight + (_margin * 2),
                     Math.max(ScreenTools.minTouchPixels * 5, maximumHeight))
    radius: Math.round(ScreenTools.defaultFontPixelWidth * 0.8)
    color: "transparent"
    border.width: 1
    border.color: Qt.rgba(1, 1, 1, 0.22)
    clip: true

    QGCPalette {
        id: qgcPal
        colorGroupEnabled: true
    }

    GlassBackdrop {
        anchors.fill: parent
        sourceItem: _root.backdropSourceItem
        backdropBlurEnabled: true
        targetItem: _root
        cornerRadius: _root.radius
    }

    Rectangle {
        anchors.fill: parent
        radius: parent.radius
        color: Qt.rgba(0.035, 0.055, 0.075, 0.9)
    }

    // The panel itself must consume clicks, while the rest of the custom layer
    // remains transparent to normal map interaction.
    DeadMouseArea {
        anchors.fill: parent
    }

    function _coordinateText(coordinate) {
        if (!coordinate || !coordinate.isValid) {
            return qsTr("Not set")
        }
        return Number(coordinate.latitude).toFixed(7) + ", "
                + Number(coordinate.longitude).toFixed(7)
    }

    function _numberText(value, decimals) {
        var numericValue = Number(value)
        return isFinite(numericValue) ? numericValue.toFixed(decimals) : "--"
    }

    function _parsedNumber(text, fallback) {
        var value = parseFloat(String(text).replace(",", "."))
        return isFinite(value) ? value : fallback
    }

    function _openExecuteConfirmation() {
        if (!executionAllowed || _confirmationDialog) {
            return
        }
        _confirmationDialog = executeConfirmationComponent.createObject(mainWindow)
        _confirmationDialog.open()
    }

    function _openCancelConfirmation() {
        if (!controller || !controller.modeActive || _confirmationDialog) {
            return
        }
        _confirmationDialog = cancelConfirmationComponent.createObject(mainWindow)
        _confirmationDialog.open()
    }

    onVisibleChanged: {
        if (!visible && _confirmationDialog) {
            _confirmationDialog.close()
            _confirmationDialog = undefined
        }
    }

    component SectionDivider: Rectangle {
        Layout.fillWidth: true
        Layout.preferredHeight: 1
        color: Qt.rgba(1, 1, 1, 0.15)
    }

    component NumericFieldRow: RowLayout {
        id: numericRow

        property string label
        property real value: 0
        property string units
        property real minimumValue: -100000
        property real maximumValue: 100000
        property bool editable: true
        property bool allowUnset: false

        signal valueEdited(real newValue)

        function formattedValue() {
            var numericValue = Number(value)
            if (allowUnset && (!isFinite(numericValue) || numericValue <= 0)) {
                return ""
            }
            return _root._numberText(numericValue, 1)
        }

        Layout.fillWidth: true
        spacing: ScreenTools.defaultFontPixelWidth * 0.5

        QGCLabel {
            Layout.fillWidth: true
            text: numericRow.label
            color: qgcPal.text
        }

        QGCTextField {
            Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 9
            horizontalAlignment: TextInput.AlignRight
            text: numericRow.formattedValue()
            placeholderText: numericRow.allowUnset ? qsTr("Default") : ""
            enabled: numericRow.editable
            numericValuesOnly: true
            showUnits: true
            unitsLabel: numericRow.units

            validator: DoubleValidator {
                bottom: numericRow.minimumValue
                top: numericRow.maximumValue
                decimals: 1
                notation: DoubleValidator.StandardNotation
            }

            onEditingFinished: {
                var parsedValue = numericRow.allowUnset && String(text).trim().length === 0
                        ? NaN
                        : _root._parsedNumber(text, numericRow.value)
                numericRow.valueEdited(parsedValue)
                text = Qt.binding(function() { return numericRow.formattedValue() })
                focus = false
            }
        }
    }

    Flickable {
        id: panelFlickable
        anchors.fill: parent
        anchors.margins: _root._margin
        contentWidth: width
        contentHeight: contentColumn.implicitHeight
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        ScrollBar.vertical: ScrollBar { }

        ColumnLayout {
            id: contentColumn
            width: panelFlickable.width
            spacing: ScreenTools.defaultFontPixelHeight * 0.45

            RowLayout {
                Layout.fillWidth: true
                spacing: ScreenTools.defaultFontPixelWidth * 0.6

                QGCLabel {
                    Layout.fillWidth: true
                    text: qsTr("Custom Landing")
                    font.bold: true
                    font.pointSize: ScreenTools.largeFontPointSize
                    color: "white"
                }

                BusyIndicator {
                    Layout.preferredWidth: ScreenTools.minTouchPixels * 0.65
                    Layout.preferredHeight: width
                    running: controller ? controller.busy : false
                    visible: running
                }
            }

            QGCLabel {
                Layout.fillWidth: true
                text: _root.instructionText
                wrapMode: Text.WordWrap
                color: "white"
            }

            QGCLabel {
                Layout.fillWidth: true
                text: qsTr("Altitudes are relative to the home position.")
                wrapMode: Text.WordWrap
                color: Qt.rgba(1, 1, 1, 0.72)
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: stateLabel.implicitHeight + ScreenTools.defaultFontPixelHeight * 0.45
                radius: ScreenTools.defaultFontPixelWidth * 0.4
                color: controller && controller.planCommitted
                           ? Qt.rgba(0.1, 0.55, 0.24, 0.35)
                           : Qt.rgba(1, 1, 1, 0.08)

                QGCLabel {
                    id: stateLabel
                    anchors.fill: parent
                    anchors.margins: ScreenTools.defaultFontPixelWidth * 0.45
                    text: controller && controller.stateText.length > 0
                              ? controller.stateText
                              : qsTr("Draft not committed")
                    wrapMode: Text.WordWrap
                    color: "white"
                    font.bold: true
                }
            }

            QGCLabel {
                Layout.fillWidth: true
                visible: controller && !controller.capabilitySupported && !controller.busy
                text: qsTr("The connected firmware did not report Custom Landing capability.")
                wrapMode: Text.WordWrap
                color: qgcPal.warningText
            }

            QGCButton {
                Layout.fillWidth: true
                visible: controller && !controller.capabilitySupported && !controller.busy
                text: qsTr("Check capability again")
                onClicked: controller.queryCapability()
            }

            QGCLabel {
                Layout.fillWidth: true
                visible: controller && controller.errorText.length > 0
                text: controller ? controller.errorText : ""
                wrapMode: Text.WordWrap
                color: qgcPal.warningText
                font.bold: true
            }

            QGCLabel {
                Layout.fillWidth: true
                visible: _root.geometryError.length > 0
                text: _root.geometryError
                wrapMode: Text.WordWrap
                color: qgcPal.warningText
                font.bold: true
            }

            SectionDivider { }

            QGCLabel {
                Layout.fillWidth: true
                text: qsTr("Loiter descent point")
                font.bold: true
                color: "#f4b942"
            }

            QGCLabel {
                Layout.fillWidth: true
                text: _root._coordinateText(controller ? controller.loiterCoordinate : undefined)
                elide: Text.ElideRight
                color: "white"
            }

            NumericFieldRow {
                label: qsTr("Target altitude")
                value: controller ? controller.loiterAltitude : 50
                units: qsTr("m")
                minimumValue: controller ? Number(controller.landingAltitude) + 20 : 20
                maximumValue: 10000
                editable: _root.draftEditable
                onValueEdited: (newValue) => controller.loiterAltitude = newValue
            }

            NumericFieldRow {
                label: qsTr("Loiter radius")
                value: controller ? controller.loiterRadius : 100
                units: qsTr("m")
                minimumValue: 10
                maximumValue: 10000
                editable: _root.draftEditable
                onValueEdited: (newValue) => controller.loiterRadius = newValue
            }

            RowLayout {
                Layout.fillWidth: true

                QGCLabel {
                    Layout.fillWidth: true
                    text: qsTr("Loiter direction")
                    color: "white"
                }

                QGCSwitch {
                    text: checked ? qsTr("Clockwise") : qsTr("Counter-clockwise")
                    checked: controller ? controller.clockwise : true
                    enabled: _root.draftEditable
                    onToggled: controller.clockwise = checked
                }
            }

            SectionDivider { }

            QGCLabel {
                Layout.fillWidth: true
                text: qsTr("Vertical landing point")
                font.bold: true
                color: "#4bc0ff"
            }

            QGCLabel {
                Layout.fillWidth: true
                text: _root._coordinateText(controller ? controller.landingCoordinate : undefined)
                elide: Text.ElideRight
                color: "white"
            }

            NumericFieldRow {
                label: qsTr("Landing altitude")
                value: controller ? controller.landingAltitude : 0
                units: qsTr("m")
                minimumValue: -1000
                maximumValue: controller ? Math.min(10000, Number(controller.loiterAltitude) - 20) : 30
                editable: _root.draftEditable
                onValueEdited: (newValue) => controller.landingAltitude = newValue
            }

            NumericFieldRow {
                label: qsTr("Approach airspeed")
                value: controller ? controller.approachAirspeed : 0
                units: qsTr("m/s")
                minimumValue: 0
                maximumValue: 200
                editable: _root.draftEditable
                allowUnset: true
                onValueEdited: (newValue) => controller.approachAirspeed = newValue
            }

            SectionDivider { }

            RowLayout {
                Layout.fillWidth: true
                spacing: ScreenTools.defaultFontPixelWidth * 0.5

                QGCButton {
                    Layout.fillWidth: true
                    text: qsTr("Reset points")
                    enabled: _root.draftEditable && (_root.loiterCoordinateValid || _root.landingCoordinateValid)
                    onClicked: controller.resetDraft()
                }

                QGCButton {
                    Layout.fillWidth: true
                    text: controller && controller.planCommitted ? qsTr("Abort landing") : qsTr("Cancel")
                    enabled: controller && controller.modeActive
                    onClicked: _root._openCancelConfirmation()
                }
            }

            QGCButton {
                Layout.fillWidth: true
                text: controller && controller.planCommitted ? qsTr("Plan committed") : qsTr("Review and execute")
                enabled: _root.executionAllowed
                onClicked: _root._openExecuteConfirmation()
            }
        }
    }

    Component {
        id: executeConfirmationComponent

        QGCPopupDialog {
            id: executeDialog
            title: qsTr("Execute Custom Landing?")
            buttons: Dialog.Yes | Dialog.Cancel
            acceptButtonEnabled: _root.executionAllowed

            onAccepted: {
                if (_root.executionAllowed) {
                    _root.controller.execute()
                }
            }
            onClosed: _root._confirmationDialog = undefined

            ColumnLayout {
                spacing: ScreenTools.defaultFontPixelHeight * 0.6

                QGCLabel {
                    Layout.fillWidth: true
                    text: qsTr("The vehicle will leave its current hold and execute this landing path after the flight controller accepts the plan.")
                    wrapMode: Text.WordWrap
                }

                GridLayout {
                    columns: 2
                    columnSpacing: ScreenTools.defaultFontPixelWidth
                    rowSpacing: ScreenTools.defaultFontPixelHeight * 0.35

                    QGCLabel { text: qsTr("Loiter point") }
                    QGCLabel { text: _root._coordinateText(_root.controller.loiterCoordinate) }
                    QGCLabel { text: qsTr("Loiter altitude") }
                    QGCLabel { text: qsTr("%1 m").arg(_root._numberText(_root.controller.loiterAltitude, 1)) }
                    QGCLabel { text: qsTr("Radius / direction") }
                    QGCLabel {
                        text: qsTr("%1 m, %2").arg(_root._numberText(_root.controller.loiterRadius, 1))
                                                   .arg(_root.controller.clockwise ? qsTr("clockwise")
                                                                                  : qsTr("counter-clockwise"))
                    }
                    QGCLabel { text: qsTr("Landing point") }
                    QGCLabel { text: _root._coordinateText(_root.controller.landingCoordinate) }
                    QGCLabel { text: qsTr("Landing altitude") }
                    QGCLabel { text: qsTr("%1 m").arg(_root._numberText(_root.controller.landingAltitude, 1)) }
                    QGCLabel { text: qsTr("Approach airspeed") }
                    QGCLabel {
                        text: isFinite(Number(_root.controller.approachAirspeed))
                                  ? qsTr("%1 m/s").arg(_root._numberText(_root.controller.approachAirspeed, 1))
                                  : qsTr("Flight controller default")
                    }
                }
            }
        }
    }

    Component {
        id: cancelConfirmationComponent

        QGCPopupDialog {
            title: _root.controller && _root.controller.planCommitted
                       ? qsTr("Abort Custom Landing?")
                       : qsTr("Cancel Custom Landing?")
            buttons: Dialog.Yes | Dialog.Cancel

            onAccepted: _root.controller.cancel()
            onClosed: _root._confirmationDialog = undefined

            QGCLabel {
                text: _root.controller && _root.controller.planCommitted
                          ? qsTr("Cancel is accepted before VTOL approach starts and then holds in Custom Landing. After VTOL approach starts it is denied; explicitly select QLOITER or QRTL if an abort is required.")
                          : qsTr("The draft will not execute. The flight controller will leave the planning hold using its configured cancel behavior.")
                wrapMode: Text.WordWrap
            }
        }
    }
}
