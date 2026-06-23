import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

/// Parameters tab for the Log Viewer.
/// Owns all filter state; call applyFilter() after a log loads or clears.
ColumnLayout {
    id: control

    required property var logParser

    spacing: ScreenTools.defaultFontPixelHeight * 0.5

    // -------------------------------------------------------------------------
    // Internal state
    // -------------------------------------------------------------------------
    property string _parameterSearchText: ""
    property bool _showOnlyChangedParameters: true
    property var _filteredParameters: []

    QGCPalette { id: qgcPal }

    // -------------------------------------------------------------------------
    // Public API
    // -------------------------------------------------------------------------

    /// Recompute the filtered parameter list. Call after log load or clear.
    function applyFilter() {
        const query = String(_parameterSearchText).trim().toLowerCase()
        const onlyChanged = _showOnlyChangedParameters

        const output = []
        for (let i = 0; i < logParser.parameters.length; i++) {
            const item = logParser.parameters[i]
            // "Show only changed" filter: skip parameters that equal their system default
            if (onlyChanged && item.hasDefault && item.isDefault) {
                continue
            }
            if (query.length > 0) {
                const name = String(item.name)
                const desc = item.shortDescription ? String(item.shortDescription) : ""
                const value = String(item.value)
                if ((name + " " + value + " " + desc).toLowerCase().indexOf(query) === -1) {
                    continue
                }
            }
            output.push(item)
        }
        _filteredParameters = output
    }

    // -------------------------------------------------------------------------
    // UI
    // -------------------------------------------------------------------------

    RowLayout {
        Layout.fillWidth: true
        spacing: ScreenTools.defaultFontPixelWidth

        QGCTextField {
            id: _parameterSearchField
            Layout.fillWidth: true
            textColor: qgcPal.textFieldText
            placeholderTextColor: Qt.rgba(qgcPal.textFieldText.r, qgcPal.textFieldText.g, qgcPal.textFieldText.b, 0.7)
            placeholderText: qsTr("Search parameters")

            onTextChanged: {
                control._parameterSearchText = text
                if (text.trim().length === 0) {
                    _parameterSearchTimer.stop()
                    control.applyFilter()
                } else {
                    _parameterSearchTimer.restart()
                }
            }

            onAccepted: {
                control._parameterSearchText = text
                _parameterSearchTimer.stop()
                control.applyFilter()
            }
        }

        QGCCheckBoxSlider {
            text: qsTr("Changed only")
            checked: _showOnlyChangedParameters
            onToggled: {
                control._showOnlyChangedParameters = checked
                control.applyFilter()
            }
        }
    }

    ScrollView {
        Layout.fillWidth: true
        Layout.fillHeight: true
        clip: true

        ListView {
            anchors.fill: parent
            model: _filteredParameters
            spacing: ScreenTools.defaultFontPixelHeight * 0.1
            clip: true
            ScrollBar.vertical: ScrollBar { }

            delegate: Rectangle {
                width: ListView.view.width
                height: _paramRow.implicitHeight + ScreenTools.defaultFontPixelHeight * 0.4
                color: index % 2 === 0 ? qgcPal.windowShade : qgcPal.windowShadeDark
                radius: 2

                // Format value: use metadata decimalPlaces when available,
                // fall back to isFloat heuristic. Show enum label when applicable.
                readonly property string _formattedValue: {
                    const v = modelData.value
                    if (v === undefined || v === null) return qsTr("N/A")
                    const numV = Number(v)
                    // Enum: find matching label
                    const eStrs = modelData.enumStrings
                    const eVals = modelData.enumValues
                    if (eStrs && eVals && eStrs.length > 0 && eVals.length === eStrs.length) {
                        for (let ei = 0; ei < eVals.length; ei++) {
                            if (Number(eVals[ei]) === numV) {
                                return eStrs[ei] + " (" + Math.round(numV) + ")"
                            }
                        }
                    }
                    // Numeric: metadata decimalPlaces wins
                    const dp = (modelData.decimalPlaces !== undefined) ? modelData.decimalPlaces : -1
                    let formatted
                    if (dp >= 0) {
                        formatted = numV.toFixed(dp)
                    } else if (modelData.isFloat) {
                        formatted = numV.toFixed(6)
                    } else {
                        formatted = String(Math.round(numV))
                    }
                    const units = (modelData.units && modelData.units.length > 0) ? (" " + modelData.units) : ""
                    return formatted + units
                }

                readonly property string _defaultText: {
                    if (!modelData.hasDefault) return ""
                    const d = modelData.defaultValue
                    if (d === undefined || d === null) return ""
                    const numD = Number(d)
                    const dp = (modelData.decimalPlaces !== undefined) ? modelData.decimalPlaces : -1
                    let formatted
                    if (dp >= 0) {
                        formatted = numD.toFixed(dp)
                    } else if (modelData.isFloat) {
                        formatted = numD.toFixed(6)
                    } else {
                        formatted = String(Math.round(numD))
                    }
                    return qsTr(" (default: %1)").arg(formatted)
                }

                RowLayout {
                    id: _paramRow
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.margins: ScreenTools.defaultFontPixelWidth * 0.5
                    spacing: ScreenTools.defaultFontPixelWidth * 0.5

                    // Changed-from-default indicator dot
                    Rectangle {
                        visible: modelData.hasDefault && !modelData.isDefault
                        width: ScreenTools.defaultFontPixelHeight * 0.5
                        height: width
                        radius: width / 2
                        color: qgcPal.colorOrange
                        Layout.alignment: Qt.AlignVCenter
                    }

                    // Spacer to keep alignment when dot is hidden
                    Item {
                        visible: !(modelData.hasDefault && !modelData.isDefault)
                        width: ScreenTools.defaultFontPixelHeight * 0.5
                        height: width
                    }

                    QGCLabel {
                        text: modelData.name
                        font.bold: modelData.hasDefault && !modelData.isDefault
                        Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 22
                        elide: Text.ElideRight
                    }

                    QGCLabel {
                        text: _formattedValue
                        Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 14
                        horizontalAlignment: Text.AlignRight
                    }

                    QGCLabel {
                        text: _defaultText
                        color: Qt.rgba(qgcPal.text.r, qgcPal.text.g, qgcPal.text.b, 0.6)
                        Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 16
                        elide: Text.ElideRight
                    }

                    QGCLabel {
                        text: modelData.shortDescription ? String(modelData.shortDescription) : ""
                        color: Qt.rgba(qgcPal.text.r, qgcPal.text.g, qgcPal.text.b, 0.6)
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                    }
                }
            }
        }
    }

    Timer {
        id: _parameterSearchTimer
        interval: 250
        repeat: false
        onTriggered: control.applyFilter()
    }
}
