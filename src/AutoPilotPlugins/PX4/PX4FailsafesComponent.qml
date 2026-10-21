import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

import "PX4FailsafeRows.js" as FailsafeRows

SetupPage {
    id:             failsafesPage
    pageComponent:  pageComponent

    property real _cellPadding:     ScreenTools.defaultFontPixelWidth * 1.5
    property bool _showActiveOnly:  false
    // Parameter values when the page first showed them, so a reboot is only flagged after a change
    property var _initialValues:    Object.create(null)

    readonly property var _vehicle: controller.vehicle
    readonly property var _rows: FailsafeRows.visibleRows(
        FailsafeRows.failsafeRows(),
        {
            "multiRotor":   _vehicle ? _vehicle.multiRotor : false,
            "fixedWing":    _vehicle ? _vehicle.fixedWing : false,
            "vtol":         _vehicle ? _vehicle.vtol : false
        },
        function (name) { return controller.parameterExists(-1, name) })
    readonly property var _groups: {
        const groups = []
        for (const row of _rows) {
            if (groups.length === 0 || groups[groups.length - 1].name !== row.group) {
                groups.push({ "name": row.group, "rows": [] })
            }
            groups[groups.length - 1].rows.push(row)
        }
        return groups
    }
    readonly property int _shownCount: {
        let count = 0
        for (const row of _rows) {
            if (_rowShown(row)) {
                count++
            }
        }
        return count
    }

    function _fact(name) {
        return controller.getParameterFact(-1, name, false)
    }

    function _rowActive(row) {
        return FailsafeRows.isRowActive(row, function (name) {
            const fact = _fact(name)
            return fact ? fact.rawValue : undefined
        })
    }

    function _rowShown(row) {
        return !_showActiveOnly || _rowActive(row)
    }

    function _groupShown(group) {
        return group.rows.some(row => _rowShown(row))
    }

    function _vehicleLabel(vehicles) {
        const labels = {
            "multiRotor":   qsTr("Multicopter"),
            "fixedWing":    qsTr("Fixed wing"),
            "vtol":         qsTr("VTOL")
        }
        return vehicles.map(name => labels[name]).join(" / ")
    }

    FactPanelController {
        id: controller
    }

    QGCPalette {
        id: qgcPal
    }

    component SeverityDot: Rectangle {
        property string severity: "none"

        implicitWidth:  ScreenTools.defaultFontPixelHeight * 0.5
        implicitHeight: implicitWidth
        radius:         ScreenTools.defaultBorderRadius / 2
        color: {
            switch (severity) {
            case "warning":     return dotPal.colorYellow
            case "fallback":    return dotPal.colorYellowGreen
            case "hold":        return dotPal.colorBlue
            case "return":      return dotPal.accent
            case "land":        return dotPal.colorOrange
            case "terminate":   return dotPal.colorRed
            default:            return dotPal.textSecondary
            }
        }

        QGCPalette {
            id: dotPal
        }
    }

    component ThresholdBar: ColumnLayout {
        property real emergency:    0
        property real critical:     0
        property real low:          0

        spacing: ScreenTools.defaultFontPixelHeight * 0.25

        QGCPalette {
            id: barPal
        }

        Row {
            Layout.preferredWidth:  ScreenTools.defaultFontPixelWidth * 28
            Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 0.75

            Repeater {
                model: [
                    { "from": 0,                "to": emergency,    "color": barPal.colorRed },
                    { "from": emergency,        "to": critical,     "color": barPal.colorOrange },
                    { "from": critical,         "to": low,          "color": barPal.colorYellow },
                    { "from": low,              "to": 1,            "color": barPal.colorGreen }
                ]

                Rectangle {
                    width:  Math.max(0, (modelData.to - modelData.from)) * parent.width
                    height: parent.height
                    color:  modelData.color

                    required property var modelData
                }
            }
        }

        QGCLabel {
            font.pointSize: ScreenTools.smallFontPointSize
            color:          barPal.textSecondary
            text:           qsTr("Emergency %1%, critical %2%, warn %3%")
                                .arg(Math.round(emergency * 100))
                                .arg(Math.round(critical * 100))
                                .arg(Math.round(low * 100))
        }
    }

    Component {
        id: pageComponent

        ColumnLayout {
            width:   Math.max(availableWidth, implicitWidth)
            spacing: _margins

            Rectangle {
                Layout.fillWidth:   true
                implicitWidth:      panelLayout.implicitWidth + _margins * 2
                implicitHeight:     panelLayout.implicitHeight + _margins * 2
                color:              qgcPal.windowPanel
                border.color:       qgcPal.borderSubtle
                radius:             ScreenTools.defaultBorderRadius * 2

                ColumnLayout {
                    id:                 panelLayout
                    anchors.fill:       parent
                    anchors.margins:    _margins
                    spacing:            _margins

                    RowLayout {
                        Layout.fillWidth:   true
                        spacing:            _margins

                        QGCRadioButtonBar {
                            model:          [ qsTr("All Failsafes"), qsTr("Active") ]
                            currentIndex:   _showActiveOnly ? 1 : 0
                            onActivated:    (index) => { _showActiveOnly = index === 1 }
                        }

                        QGCLabel {
                            Layout.fillWidth:   true
                            color:              qgcPal.textSecondary
                            text:               _showActiveOnly ?
                                                    qsTr("Showing %1 of %2 failsafes.").arg(_shownCount).arg(_rows.length) :
                                                    qsTr("Showing all %1 failsafes.").arg(_rows.length)
                        }
                    }

                    RowLayout {
                        spacing: ScreenTools.defaultFontPixelWidth * 2

                        Repeater {
                            model: [
                                { "severity": "warning",    "text": qsTr("Warning") },
                                { "severity": "hold",       "text": qsTr("Hold") },
                                { "severity": "fallback",   "text": qsTr("Mode fallback") },
                                { "severity": "return",     "text": qsTr("Return") },
                                { "severity": "land",       "text": qsTr("Land") },
                                { "severity": "terminate",  "text": qsTr("Terminate / Disarm") }
                            ]

                            RowLayout {
                                spacing: ScreenTools.defaultFontPixelWidth / 2

                                required property var modelData

                                SeverityDot {
                                    severity: modelData.severity
                                }

                                QGCLabel {
                                    text:           modelData.text
                                    font.pointSize: ScreenTools.smallFontPointSize
                                    color:          qgcPal.textSecondary
                                }
                            }
                        }
                    }

                    Repeater {
                        model: _groups

                        ColumnLayout {
                            Layout.fillWidth:   true
                            spacing:            _margins
                            visible:            _groupShown(group)

                            required property var modelData

                            readonly property var group: modelData

                            RowLayout {
                                Layout.fillWidth:   true
                                spacing:            _margins

                                QGCCaptionLabel {
                                    text:           group.name
                                    color:          qgcPal.textPrimary
                                }

                                Rectangle {
                                    Layout.fillWidth:   true
                                    implicitHeight:     1
                                    color:              qgcPal.borderSubtle
                                }
                            }

                            Repeater {
                                model: group.rows

                                ColumnLayout {
                                    id:                 rowLayout
                                    Layout.fillWidth:   true
                                    spacing:            _margins
                                    visible:            _rowShown(row)

                                    required property var modelData
                                    required property int index

                                    readonly property var row:      modelData
                                    readonly property bool _active: _rowActive(row)

                                    Rectangle {
                                        Layout.fillWidth:   true
                                        implicitHeight:     1
                                        color:              qgcPal.borderSubtle
                                        visible:            rowLayout.index > 0
                                    }

                                    RowLayout {
                                        Layout.fillWidth:   true
                                        spacing:            _margins

                                        ColumnLayout {
                                            Layout.alignment:       Qt.AlignTop
                                            Layout.preferredWidth:  ScreenTools.defaultFontPixelWidth * 30
                                            Layout.maximumWidth:    Layout.preferredWidth
                                            Layout.topMargin:       _cellPadding
                                            spacing:                ScreenTools.defaultFontPixelHeight * 0.25

                                            RowLayout {
                                                spacing: ScreenTools.defaultFontPixelWidth

                                                Rectangle {
                                                    implicitWidth:  ScreenTools.defaultFontPixelHeight * 0.5
                                                    implicitHeight: implicitWidth
                                                    radius:         implicitWidth / 2
                                                    color:          rowLayout._active ? qgcPal.colorGreen : "transparent"
                                                    border.color:   rowLayout._active ? color : qgcPal.textSecondary
                                                }

                                                QGCLabel {
                                                    Layout.fillWidth:       true
                                                    text:                   rowLayout.row.title
                                                    wrapMode:               Text.WordWrap
                                                    font.bold:              true
                                                    font.capitalization:    Font.AllUppercase
                                                    font.letterSpacing:     ScreenTools.defaultFontPixelWidth * 0.1
                                                }
                                            }

                                            Flow {
                                                Layout.fillWidth:   true
                                                spacing:            ScreenTools.defaultFontPixelWidth

                                                Repeater {
                                                    model: rowLayout.row.params

                                                    QGCLabel {
                                                        text:           modelData
                                                        font.family:    ScreenTools.fixedFontFamily
                                                        font.pointSize: ScreenTools.smallFontPointSize
                                                        color:          qgcPal.textSecondary

                                                        required property string modelData
                                                    }
                                                }
                                            }

                                            RowLayout {
                                                spacing:    ScreenTools.defaultFontPixelWidth / 2
                                                visible:    rowLayout.row.warnOnly || rowLayout.row.vehicles !== undefined

                                                QGCChip {
                                                    text:       qsTr("Warning only")
                                                    visible:    rowLayout.row.warnOnly
                                                }

                                                QGCChip {
                                                    text:       rowLayout.row.vehicles ? _vehicleLabel(rowLayout.row.vehicles) : ""
                                                    visible:    rowLayout.row.vehicles !== undefined
                                                }
                                            }

                                            QGCLabel {
                                                Layout.fillWidth:   true
                                                wrapMode:           Text.WordWrap
                                                font.pointSize:     ScreenTools.smallFontPointSize
                                                color:              qgcPal.textSecondary
                                                text:               rowLayout.row.description
                                            }
                                        }

                                        // Cells wrap instead of widening the page, so the column only needs its widest single cell
                                        Flow {
                                            Layout.alignment:       Qt.AlignTop
                                            Layout.fillWidth:       true
                                            Layout.minimumWidth:    _widestChildWidth
                                            Layout.preferredWidth:  _widestChildWidth
                                            spacing:                _margins

                                            readonly property real _widestChildWidth: {
                                                let widest = 0
                                                for (let i = 0; i < children.length; i++) {
                                                    if (children[i].visible) {
                                                        widest = Math.max(widest, children[i].implicitWidth)
                                                    }
                                                }
                                                return widest
                                            }

                                            Repeater {
                                                model: rowLayout.row.cells

                                                QGCCaptionCell {
                                                    id:         cell
                                                    caption:    modelData.label

                                                    required property var modelData

                                                    readonly property var _fact:        modelData.param ? failsafesPage._fact(modelData.param) : null
                                                    readonly property bool _isBitmask:  _fact ? _fact.bitmaskStrings.length > 0 : false
                                                    readonly property bool _isEnum:     _fact ? !_isBitmask && _fact.enumStrings.length > 0 : false
                                                    readonly property bool _isNumber:   _fact ? !_isBitmask && !_isEnum : false
                                                    readonly property bool _rebootPending: _fact !== null && _fact.vehicleRebootRequired &&
                                                                                           _initialValue !== undefined && _fact.rawValue !== _initialValue

                                                    property var _initialValue: undefined

                                                    Component.onCompleted: {
                                                        if (!_fact) {
                                                            return
                                                        }
                                                        if (!(modelData.param in failsafesPage._initialValues)) {
                                                            failsafesPage._initialValues[modelData.param] = _fact.rawValue
                                                        }
                                                        _initialValue = failsafesPage._initialValues[modelData.param]
                                                    }

                                                    RowLayout {
                                                        spacing:    ScreenTools.defaultFontPixelWidth
                                                        visible:    cell._isEnum

                                                        SeverityDot {
                                                            severity:   FailsafeRows.actionSeverity(modelData, cell._fact ? cell._fact.rawValue : undefined)
                                                            visible:    modelData.severity !== undefined
                                                        }

                                                        FactComboBox {
                                                            Layout.fillWidth:       true
                                                            Layout.maximumWidth:    ScreenTools.defaultFontPixelWidth * 50
                                                            fact:                   cell._fact
                                                            indexModel:         false
                                                            sizeToContents:     true
                                                        }
                                                    }

                                                    ColumnLayout {
                                                        Layout.fillWidth:       true
                                                        Layout.maximumWidth:    ScreenTools.defaultFontPixelWidth * 36
                                                        spacing:                ScreenTools.defaultFontPixelHeight * 0.25
                                                        visible:                cell._isBitmask

                                                        Repeater {
                                                            model: cell._isBitmask ? cell._fact.bitmaskStrings : []

                                                            FactBitMaskCheckBoxSlider {
                                                                objectName:         "failsafes_bit_" + cell._fact.name + "_" + index
                                                                Layout.fillWidth:   true
                                                                text:               modelData
                                                                fact:               cell._fact
                                                                bitMask:            cell._fact.bitmaskValues[index]

                                                                required property int index
                                                                required property string modelData
                                                            }
                                                        }
                                                    }

                                                    FactTextField {
                                                        Layout.preferredWidth:  ScreenTools.defaultFontPixelWidth * 14
                                                        fact:                   cell._fact
                                                        visible:                cell._isNumber
                                                    }

                                                    RowLayout {
                                                        spacing:    ScreenTools.defaultFontPixelWidth
                                                        visible:    modelData.fixed !== undefined

                                                        SeverityDot {
                                                            severity:   FailsafeRows.actionSeverity(modelData)
                                                            visible:    modelData.severity !== undefined
                                                        }

                                                        QGCLabel {
                                                            text:   modelData.fixed !== undefined ? modelData.fixed : ""
                                                        }
                                                    }

                                                    ThresholdBar {
                                                        // visibleRows only keeps this cell when all three parameters exist
                                                        visible:    modelData.thresholdBar !== undefined
                                                        emergency:  modelData.thresholdBar ? failsafesPage._fact(modelData.thresholdBar[0]).rawValue : 0
                                                        critical:   modelData.thresholdBar ? failsafesPage._fact(modelData.thresholdBar[1]).rawValue : 0
                                                        low:        modelData.thresholdBar ? failsafesPage._fact(modelData.thresholdBar[2]).rawValue : 0
                                                    }

                                                    QGCChip {
                                                        objectName: "failsafes_rebootChip_" + (modelData.param !== undefined ? modelData.param : "")
                                                        text:       cell._rebootPending ? qsTr("Reboot required") : qsTr("Applies after reboot")
                                                        textColor:  cell._rebootPending ? qgcPal.warningText : qgcPal.textSecondary
                                                        visible:    cell._fact ? cell._fact.vehicleRebootRequired : false
                                                    }

                                                    QGCLabel {
                                                        Layout.fillWidth:       true
                                                        Layout.preferredWidth:  Math.min(implicitWidth, ScreenTools.defaultFontPixelWidth * 20)
                                                        wrapMode:               Text.WordWrap
                                                        font.pointSize:         ScreenTools.smallFontPointSize
                                                        color:                  qgcPal.textSecondary
                                                        text:                   modelData.note !== undefined ? modelData.note : ""
                                                        visible:                modelData.note !== undefined
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
