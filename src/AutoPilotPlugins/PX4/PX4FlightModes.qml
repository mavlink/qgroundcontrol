import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

import "PX4ConfigRows.js" as ConfigRows
import "PX4FlightModeRows.js" as ModeRows

SetupPage {
    id:             flightModesPage
    pageComponent:  pageComponent

    property bool   _showAssignedOnly:  false
    // Method picked while neither a mode channel nor mode buttons are set, so its control shows before it has a value
    property string _chosenMethod:      ""
    // Parameter values when the page first showed them, so a reboot is only flagged after a change
    property var    _initialValues:     Object.create(null)

    readonly property var _vehicle: controller.vehicle
    readonly property var _allRows: ModeRows.flightModeRows()
    readonly property var _rows: ConfigRows.visibleRows(
        _allRows,
        {
            "multiRotor":   _vehicle ? _vehicle.multiRotor : false,
            "fixedWing":    _vehicle ? _vehicle.fixedWing : false,
            "vtol":         _vehicle ? _vehicle.vtol : false
        },
        function (name) { return controller.parameterExists(-1, name) })
    readonly property var _groups: ConfigRows.groupRows(_rows)
    readonly property int _shownCount: _rows.filter(row => _rowShown(row)).length

    readonly property var   _inputModeFact:     _fact("COM_RC_IN_MODE")
    readonly property var   _modeChannelFact:   _fact("RC_MAP_FLTMODE")
    readonly property var   _buttonMaskFact:    _fact("RC_MAP_FLTM_BTN")
    readonly property int   _modeChannel:       _modeChannelFact ? _modeChannelFact.rawValue : 0
    readonly property int   _buttonMask:        _buttonMaskFact ? _buttonMaskFact.rawValue : 0
    readonly property string _method: {
        const method = ModeRows.selectionMethod(_modeChannel, _buttonMask)
        return (method === "none" && _chosenMethod !== "") ? _chosenMethod : method
    }
    readonly property var _buttonChannels: ModeRows.buttonChannels(_buttonMask)
    readonly property var _modeChannelValue: _method === "switch" ? _channelValue(_modeChannel) : undefined
    readonly property int _activeSlot: {
        if (_method === "switch") {
            return _modeChannelValue !== undefined ? ModeRows.modeSlot(_modeChannelValue) : 0
        }
        return _method === "buttons" ? _lastButtonSlot : 0
    }
    property int _lastButtonSlot: 0

    on_ButtonMaskChanged: _lastButtonSlot = 0
    on_MethodChanged: _lastButtonSlot = 0

    Connections {
        target: controller

        function onRcChannelValuesChanged() {
            if (_method === "buttons") {
                _lastButtonSlot = ModeRows.nextButtonSlot(_lastButtonSlot, _buttonMask, _channelValue)
            }
        }
    }
    readonly property var _slotRanges: {
        if (_method !== "switch" || _modeChannel <= 0) {
            return []
        }
        const prefix = "RC" + _modeChannel
        const min = _rawValue(prefix + "_MIN")
        const trim = _rawValue(prefix + "_TRIM")
        const max = _rawValue(prefix + "_MAX")
        if (min === undefined || trim === undefined || max === undefined) {
            return []
        }
        return ModeRows.slotPwmRanges(min, trim, max, _rawValue(prefix + "_REV") < 0)
    }
    readonly property var _conflicts: {
        const assignments = [
            { "name": qsTr("Roll"),     "channel": _rawValue("RC_MAP_ROLL") },
            { "name": qsTr("Pitch"),    "channel": _rawValue("RC_MAP_PITCH") },
            { "name": qsTr("Yaw"),      "channel": _rawValue("RC_MAP_YAW") },
            { "name": qsTr("Throttle"), "channel": _rawValue("RC_MAP_THROTTLE") }
        ]
        if (_method === "switch") {
            assignments.push({ "name": qsTr("Mode channel"), "channel": _modeChannel })
        } else if (_method === "buttons") {
            // PX4 reads a mode button as pressed above 75% of its channel
            _buttonChannels.forEach((channel, i) => assignments.push({ "name": qsTr("Mode button %1").arg(i + 1), "channel": channel,
                                                                       "range": ModeRows.switchOnRange(0.75) }))
        }
        for (const row of _rows) {
            if (row.kind === "switch") {
                assignments.push({ "name": row.title, "channel": _rawValue(row.channelParam), "range": _switchOnRange(row) })
            }
        }
        return ModeRows.channelConflicts(assignments)
    }

    function _fact(name) {
        return controller.getParameterFact(-1, name, false)
    }

    function _rawValue(name) {
        const fact = _fact(name)
        return fact ? fact.rawValue : undefined
    }

    /// Normalized live value of an RC channel (1-based), undefined without RC input or calibration
    function _channelValue(channel) {
        const pwm = controller.rcChannelValues[channel - 1]
        if (pwm === undefined || pwm < 0) {
            return undefined
        }
        const prefix = "RC" + channel
        const min = _rawValue(prefix + "_MIN")
        const trim = _rawValue(prefix + "_TRIM")
        const max = _rawValue(prefix + "_MAX")
        if (min === undefined || trim === undefined || max === undefined) {
            return undefined
        }
        return ModeRows.normalizedChannel(pwm, min, trim, max, _rawValue(prefix + "_REV") < 0)
    }

    /// Where a switch row is on along its channel, undefined for one without a threshold such as flaps
    function _switchOnRange(row) {
        const onThreshold = row.thresholdParam ? _rawValue(row.thresholdParam) : row.fixedThreshold
        if (onThreshold === undefined) {
            return undefined
        }
        const midThreshold = row.midThresholdParam ? _rawValue(row.midThresholdParam) : undefined
        return ModeRows.switchOnRange(onThreshold, midThreshold)
    }

    /// "on", "mid", "off" or "" without live input for a switch row
    function _switchPosition(row) {
        const value = _channelValue(_rawValue(row.channelParam))
        if (value === undefined) {
            return ""
        }
        const onThreshold = row.thresholdParam ? _rawValue(row.thresholdParam) : row.fixedThreshold
        if (onThreshold === undefined) {
            return ""
        }
        const midThreshold = row.midThresholdParam ? _rawValue(row.midThresholdParam) : undefined
        return ModeRows.switchPosition(value, onThreshold, midThreshold)
    }

    function _rowActive(row) {
        return ConfigRows.isRowActive(row, _rawValue)
    }

    function _rowShown(row) {
        return !_showAssignedOnly || !ConfigRows.isOptionalRow(row) || _rowActive(row)
    }

    function _groupShown(group) {
        return group.rows.some(row => _rowShown(row))
    }

    function _modeChoices(fact) {
        return ModeRows.modeChoices(fact.name, fact.enumStrings, fact.enumValues, controller.externalModeNames, fact.rawValue)
    }

    /// Name of the mode a slot selects, empty when the slot is unassigned
    function _slotModeName(slot) {
        const fact = _fact("COM_FLTMODE" + slot)
        if (!fact || fact.rawValue < 0 || fact.enumIndex < 0) {
            return ""
        }
        return ModeRows.modeName(fact.name, fact.enumStrings, fact.enumValues, controller.externalModeNames, fact.rawValue)
    }

    function _setMethod(method) {
        _chosenMethod = method
        for (const write of ModeRows.selectionWrites(method, _modeChannel, _buttonMask)) {
            _fact(write.param).rawValue = write.value
        }
    }

    function _slotRangeText(slot) {
        const range = _slotRanges[slot - 1]
        if (!range) {
            return ""
        }
        if (range.low === null) {
            return qsTr("up to %1 µs").arg(range.high)
        }
        if (range.high === null) {
            return qsTr("%1 µs and up").arg(range.low)
        }
        return qsTr("%1 – %2 µs").arg(range.low).arg(range.high)
    }

    PX4FlightModesController {
        id:                     controller
        monitorRc:              true
        // Reboot-required settings are marked on the page instead
        suppressRebootMessage:  true
    }

    QGCPalette {
        id: qgcPal
    }

    Component {
        id: pageComponent

        ColumnLayout {
            width:   Math.max(availableWidth, implicitWidth)
            spacing: _margins

            ConfigRowsPanel {
                ConfigRowsFilterBar {
                    objectName:         "flightModes_filter"
                    filterLabels:       [ qsTr("All"), qsTr("Assigned") ]
                    filterIndex:        _showAssignedOnly ? 1 : 0
                    summary:            _showAssignedOnly ?
                                            qsTr("Showing %1 of %2 settings.").arg(_shownCount).arg(_rows.length) :
                                            qsTr("Showing all %1 settings.").arg(_rows.length)
                    onFilterActivated:  (index) => { _showAssignedOnly = index === 1 }
                }

                QGCLabel {
                    objectName:         "flightModes_inputSource"
                    Layout.fillWidth:   true
                    wrapMode:           Text.WordWrap
                    visible:            _inputModeFact !== null
                    // 1: MAVLink only, 4: manual control disabled
                    color:              _rcUnused ? qgcPal.warningText : qgcPal.textSecondary
                    text:               _inputModeFact ?
                                            (_rcUnused ?
                                                 qsTr("Manual control source: %1. RC input is not used, so the mode channel and switches have no effect.").arg(_inputModeFact.enumStringValue) :
                                                 qsTr("Manual control source: %1. The mode channel, mode buttons and switches act on RC input.").arg(_inputModeFact.enumStringValue)) :
                                            ""

                    readonly property bool _rcUnused: _inputModeFact ? (_inputModeFact.rawValue === 1 || _inputModeFact.rawValue === 4) : false
                }

                Repeater {
                    model: _conflicts

                    Rectangle {
                        objectName:         "flightModes_conflict_" + modelData.channel
                        Layout.fillWidth:   true
                        implicitHeight:     conflictLabel.implicitHeight + _margins * 2
                        color:              "transparent"
                        border.color:       qgcPal.warningText
                        radius:             ScreenTools.defaultBorderRadius * 2

                        required property var modelData

                        QGCLabel {
                            id:                     conflictLabel
                            anchors.left:           parent.left
                            anchors.right:          parent.right
                            anchors.margins:        ScreenTools.defaultFontPixelWidth * 1.5
                            anchors.verticalCenter: parent.verticalCenter
                            wrapMode:               Text.WordWrap
                            color:                  qgcPal.warningText
                            text:                   qsTr("Channel %1: %2 can be on at the same time. Use separate channels, or thresholds that do not overlap.")
                                                        .arg(modelData.channel).arg(modelData.users.join(", "))
                        }
                    }
                }

                ConfigRowGroups {
                    model:      _groups
                    groupShown: flightModesPage._groupShown

                    rowDelegate: ConfigRow {
                        id:             rowLayout
                        objectName:     "flightModes_row_" + row.id
                        title:          row.title
                        params:         row.params
                        active:         _rowActive(row)
                        activeColor:    _livePosition === "on" || _livePosition === "mid" ? qgcPal.colorYellow : qgcPal.colorGreen
                        showSeparator:  index > 0
                        visible:        _rowShown(row)

                        required property var modelData
                        required property int index

                        readonly property var       row:            modelData
                        readonly property bool      _isSwitch:      row.kind === "switch"
                        readonly property string    _livePosition:  _isSwitch && active ? _switchPosition(row) : ""

                        QGCChip {
                            text:       qsTr("Cannot be undone until reboot")
                            textColor:  qgcPal.warningText
                            visible:    rowLayout.row.irreversible === true
                        }

                        ConfigNote {
                            text: rowLayout.row.description
                        }

                        cellsData: [
                            Repeater {
                                model: rowLayout.row.cells.filter(cell => !cell.customUi)

                                ConfigParamCell {
                                    cell:               modelData
                                    factFor:            flightModesPage._fact
                                    initialValues:      flightModesPage._initialValues
                                    objectNamePrefix:   "flightModes"
                                    enumChoices:        modelData.param === "COM_FLTMODE_BOOT" && fact ? _modeChoices(fact) : null
                                    // An unassigned switch only needs its channel
                                    visible:            !rowLayout._isSwitch || rowLayout.active || modelData.param === rowLayout.row.channelParam

                                    required property var modelData
                                }
                            },

                            Loader {
                                active:     rowLayout.row.kind === "method"
                                visible:    active

                                sourceComponent: QGCCaptionCell {
                                    caption: qsTr("Method")

                                    QGCRadioButtonBar {
                                        objectName:     "flightModes_method"
                                        model:          _buttonMaskFact ?
                                                            [ qsTr("Switch channel"), qsTr("Buttons"), qsTr("None") ] :
                                                            [ qsTr("Switch channel"), qsTr("None") ]
                                        currentIndex:   _methods.indexOf(_method)
                                        onActivated:    (index) => _setMethod(_methods[index])

                                        readonly property var _methods: _buttonMaskFact ? [ "switch", "buttons", "none" ] : [ "switch", "none" ]
                                    }
                                }
                            },

                            Loader {
                                active:     rowLayout.row.kind === "method" && _method === "switch"
                                visible:    active

                                sourceComponent: QGCCaptionCell {
                                    caption: qsTr("Mode channel")

                                    FactComboBox {
                                        objectName:     "flightModes_modeChannel"
                                        fact:           _modeChannelFact
                                        indexModel:     false
                                        sizeToContents: true
                                    }
                                }
                            },

                            Loader {
                                active:     rowLayout.row.kind === "method" && _method === "buttons" && _buttonMaskFact !== null
                                visible:    active

                                sourceComponent: QGCCaptionCell {
                                    caption: qsTr("Mode button channels")

                                    GridLayout {
                                        columns:        6
                                        columnSpacing:  ScreenTools.defaultFontPixelWidth
                                        rowSpacing:     ScreenTools.defaultFontPixelHeight * 0.25

                                        Repeater {
                                            model: 18

                                            FactBitMaskCheckBoxSlider {
                                                objectName: "flightModes_button_" + (index + 1)
                                                text:       qsTr("Ch %1").arg(index + 1)
                                                fact:       _buttonMaskFact
                                                bitMask:    1 << index

                                                required property int index
                                            }
                                        }
                                    }

                                    ConfigNote {
                                        Layout.preferredWidth:  ScreenTools.defaultFontPixelWidth * 40
                                        text:                   qsTr("The lowest channel selects slot 1, the next slot 2, and so on. PX4 uses up to six.")
                                    }

                                    ConfigNote {
                                        objectName:             "flightModes_ignoredButtons"
                                        Layout.preferredWidth:  ScreenTools.defaultFontPixelWidth * 40
                                        color:                  qgcPal.warningText
                                        text: {
                                            const ignored = ModeRows.ignoredButtonChannels(_buttonMask)
                                            return ignored.length > 0 ?
                                                       qsTr("PX4 ignores %1, as only the lowest six channels select a slot.")
                                                           .arg(ignored.map(channel => qsTr("Ch %1").arg(channel)).join(", ")) :
                                                       ""
                                        }
                                    }
                                }
                            },

                            Loader {
                                active:     rowLayout.row.kind === "slots" && _slotRanges.length > 0
                                visible:    active

                                sourceComponent: QGCCaptionCell {
                                    caption: _modeChannelValue !== undefined ?
                                                 qsTr("Channel %1 live").arg(_modeChannel) :
                                                 qsTr("Channel %1, no RC input").arg(_modeChannel)

                                    Item {
                                        id:             slotStrip
                                        implicitWidth:  ScreenTools.defaultFontPixelWidth * 72
                                        implicitHeight: ScreenTools.defaultFontPixelHeight * 2.5

                                        readonly property real _slotWidth: width / ModeRows.modeSlotCount

                                        Repeater {
                                            model: ModeRows.modeSlotCount

                                            Rectangle {
                                                objectName:     "flightModes_strip_" + (index + 1)
                                                x:              slotStrip._slotWidth * index
                                                width:          slotStrip._slotWidth
                                                height:         slotStrip.height
                                                color:          _active ? qgcPal.colorYellow : qgcPal.windowPanel
                                                border.color:   qgcPal.borderSubtle

                                                required property int index

                                                readonly property bool      _active:    _activeSlot === index + 1
                                                readonly property string    _modeName:  _slotModeName(index + 1)

                                                Column {
                                                    anchors.left:           parent.left
                                                    anchors.right:          parent.right
                                                    anchors.margins:        ScreenTools.defaultFontPixelWidth / 2
                                                    anchors.verticalCenter: parent.verticalCenter

                                                    QGCLabel {
                                                        width:                  parent.width
                                                        horizontalAlignment:    Text.AlignHCenter
                                                        text:                   index + 1
                                                        font.bold:              _active
                                                        color:                  _active ? qgcPal.window : qgcPal.textSecondary
                                                    }

                                                    QGCLabel {
                                                        width:                  parent.width
                                                        horizontalAlignment:    Text.AlignHCenter
                                                        elide:                  Text.ElideRight
                                                        font.pointSize:         ScreenTools.smallFontPointSize
                                                        text:                   _modeName !== "" ? _modeName : qsTr("Unassigned")
                                                        color:                  _active ? qgcPal.window : (_modeName !== "" ? qgcPal.textPrimary : qgcPal.textSecondary)
                                                    }
                                                }
                                            }
                                        }

                                        Rectangle {
                                            x:          slotStrip.width * ModeRows.slotStripPosition(_modeChannelValue) - width / 2
                                            width:      ScreenTools.defaultFontPixelWidth / 3
                                            height:     slotStrip.height
                                            color:      qgcPal.textPrimary
                                            visible:    _modeChannelValue !== undefined
                                        }
                                    }
                                }
                            },

                            Loader {
                                active:     rowLayout.row.kind === "slots"
                                visible:    active

                                sourceComponent: QGCCaptionCell {
                                    GridLayout {
                                        columns:        4
                                        columnSpacing:  ScreenTools.defaultFontPixelWidth * 1.5
                                        rowSpacing:     ScreenTools.defaultFontPixelHeight * 0.25

                                        Repeater {
                                            model: ModeRows.modeSlotCount

                                            QGCLabel {
                                                Layout.row:     index
                                                Layout.column:  0
                                                text:           qsTr("Slot %1").arg(index + 1)
                                                font.bold:      _activeSlot === index + 1
                                                color:          _activeSlot === index + 1 ? qgcPal.colorYellow : qgcPal.textPrimary

                                                required property int index
                                            }
                                        }

                                        Repeater {
                                            model: ModeRows.modeSlotCount

                                            QGCLabel {
                                                Layout.row:     index
                                                Layout.column:  1
                                                font.family:    ScreenTools.fixedFontFamily
                                                font.pointSize: ScreenTools.smallFontPointSize
                                                color:          qgcPal.textSecondary
                                                text: {
                                                    if (_method === "switch") {
                                                        return _slotRangeText(index + 1)
                                                    }
                                                    if (_method === "buttons") {
                                                        return index < _buttonChannels.length ? qsTr("Ch %1 button").arg(_buttonChannels[index]) : qsTr("No button")
                                                    }
                                                    return ""
                                                }

                                                required property int index
                                            }
                                        }

                                        Repeater {
                                            model: ModeRows.modeSlotCount

                                            ConfigChoiceComboBox {
                                                objectName:         "flightModes_slot_" + (index + 1)
                                                Layout.row:         index
                                                Layout.column:      2
                                                Layout.fillWidth:   true
                                                fact:               _fact("COM_FLTMODE" + (index + 1))
                                                choices:            fact ? _modeChoices(fact) : []

                                                required property int index
                                            }
                                        }

                                        Repeater {
                                            model: ModeRows.modeSlotCount

                                            QGCChip {
                                                objectName:     "flightModes_slotActive_" + (index + 1)
                                                Layout.row:     index
                                                Layout.column:  3
                                                // A released button leaves its mode selected until something else changes the mode
                                                text:           _method === "buttons" ? qsTr("Last pressed") : qsTr("Active")
                                                textColor:      qgcPal.colorYellow
                                                opacity:        _activeSlot === index + 1 ? 1 : 0

                                                required property int index
                                            }
                                        }
                                    }
                                }
                            },

                            Loader {
                                active:     rowLayout._isSwitch && rowLayout.active
                                visible:    active

                                sourceComponent: QGCCaptionCell {
                                    caption: qsTr("Live")

                                    QGCLabel {
                                        objectName: "flightModes_live_" + rowLayout.row.id
                                        color:      rowLayout._livePosition === "on" || rowLayout._livePosition === "mid" ? qgcPal.colorYellow : qgcPal.textPrimary
                                        text: {
                                            const pwm = controller.rcChannelValues[_rawValue(rowLayout.row.channelParam) - 1]
                                            if (pwm === undefined || pwm < 0) {
                                                return qsTr("No RC input")
                                            }
                                            const positions = { "on": qsTr("On"), "mid": qsTr("Mid"), "off": qsTr("Off") }
                                            const position = positions[rowLayout._livePosition]
                                            return position ? qsTr("%1 · %2 µs").arg(position).arg(pwm) : qsTr("%1 µs").arg(pwm)
                                        }
                                    }
                                }
                            }
                        ]
                    }
                }
            }
        }
    }
}
