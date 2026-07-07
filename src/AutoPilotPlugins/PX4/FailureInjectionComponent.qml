/****************************************************************************
 * FailureInjectionComponent.qml
 *
 * Vehicle Setup page (PX4) that injects simulated sensor/system failures via
 * MAV_CMD_INJECT_FAILURE: SYS_FAILURE_EN gate -> reboot -> armed, then a
 * component/type/instance picker with an activity log and reset-all.
 ****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

import "FailureInjectionInstances.js" as Instances

SetupPage {
    id:             failureInjectionPage
    pageComponent:  pageComponent

    // ---- state / model ------------------------------------------------------

    property var  _activeVehicle:       QGroundControl.multiVehicleManager.activeVehicle
    property Fact _sysFailureEn:        controller.getParameterFact(-1, "SYS_FAILURE_EN", true /* reportMissing */)
    property bool _paramSet:            _sysFailureEn && _sysFailureEn.value === 1
    property bool _pendingReboot:       false   // SYS_FAILURE_EN just toggled, reboot not yet triggered
    property bool _armed:               _paramSet   // PX4 honors SYS_FAILURE_EN at boot; arm once the param reads 1

    readonly property int _cmdInjectFailure: 420   // MAV_CMD_INJECT_FAILURE

    // FAILURE_UNIT / FAILURE_TYPE sourced from the MAVLink dialect via the C++ singleton
    // (enum values + names track common.xml).
    property var _units: FailureInjection.units   // [{ name, unit }]
    property var _types: FailureInjection.types   // [{ name, type }]

    property int _unitIndex:          4    // GPS
    property int _typeIndex:          1    // Off
    property var _selectedInstances:  [1]  // 1-based instance numbers
    readonly property int _instanceCount: 8   // fixed number of selectable instances (not tied to the unit)

    // Activity log column widths — shared by the header and every row so the fields line up.
    readonly property real _colTimeWidth: ScreenTools.defaultFontPixelWidth * 9
    readonly property real _colUnitWidth: ScreenTools.defaultFontPixelWidth * 17
    readonly property real _colTypeWidth: ScreenTools.defaultFontPixelWidth * 15

    FactPanelController { id: controller }
    QGCPalette          { id: qgcPal; colorGroupEnabled: true }

    // Activity log + injected-unit set live in the FailureInjection singleton so they
    // persist across navigating away from / back to this page (which recreates the QML).

    // Holds MAV_CMD_INJECT_FAILURE sends not yet dispatched. Each entry is sent only once the
    // previous one's ack has come back through onMavCommandResult/_onAck, so at most one is ever
    // in flight at a time.
    property var _sendQueue: []   // outstanding sends: [{ unit, type, instance, logArgs|null }]

    Connections {
        target: _activeVehicle
        function onMavCommandResult(vehicleId, targetComponent, command, ackResult, failureCode) {
            if (command === _cmdInjectFailure) {
                _onAck(ackResult)
            }
        }
    }

    // ---- behaviour ----------------------------------------------------------

    function _injectOne(unitEnum, typeEnum, param3, param4) {
        if (!_activeVehicle) {
            return
        }
        // sendCommand is the QML-callable form of sendMavCommand. Target the component
        // that owns SYS_FAILURE_EN (the autopilot); fall back to component 1.
        var compId = _sysFailureEn ? _sysFailureEn.componentId : 1
        // param3 = instance (0 = all, NaN = use bitmask), param4 = instance bitmask (bit 0 = instance 1).
        // showError=false: the ack result is surfaced via the mavCommandResult handler (per-row status).
        _activeVehicle.sendCommand(compId,
                                   _cmdInjectFailure,
                                   false,                              // showError
                                   unitEnum, typeEnum, param3, param4, // param1..4
                                   0, 0, 0)                            // param5..7
    }

    function _instanceSend() {
        return Instances.instanceSend(_selectedInstances)
    }

    // Queue one command; kick off sending if the queue was idle.
    function _enqueueSend(item) {
        var q = _sendQueue.slice()
        q.push(item)
        _sendQueue = q
        if (q.length === 1) {
            _sendCurrent()
        }
    }

    // Send the command at the head of the queue, logging it when it's an injection (not a reset).
    function _sendCurrent() {
        if (_sendQueue.length === 0 || !_activeVehicle) {
            return
        }
        var s = _sendQueue[0]
        if (s.logArgs) {
            var stamp = Qt.formatDateTime(new Date(), "hh:mm:ss")
            if (s.logArgs.track) {
                // injection: log the row and remember the unit for Reset
                FailureInjection.logInjection(s.logArgs.unitName, s.logArgs.typeName, s.unit, s.logArgs.instanceLabel, stamp)
            } else {
                // reset OK: log the row but don't re-track the unit
                FailureInjection.logRow(s.logArgs.unitName, s.logArgs.typeName, s.logArgs.instanceLabel, stamp)
            }
        }
        _injectOne(s.unit, s.type, s.param3, s.param4)
    }

    // One ack arrived: resolve the matching log row (only one is pending at a time), then send the next.
    function _onAck(ackResult) {
        FailureInjection.resolveResult(ackResult)   // no-op when the head was a reset (no pending row)
        if (_sendQueue.length > 0) {
            var q = _sendQueue.slice()
            q.shift()
            _sendQueue = q
        }
        _sendCurrent()
    }

    function _apply() {
        if (!_armed || _selectedInstances.length === 0) {
            return
        }
        var u    = _units[_unitIndex]
        var t    = _types[_typeIndex]
        var send = _instanceSend()   // one command covers all selected instances (bitmask when >1)
        _enqueueSend({ unit: u.unit, type: t.type, param3: send.param3, param4: send.param4,
                       logArgs: { unitName: u.name, typeName: t.name, instanceLabel: send.label, track: true } })
    }

    function _toggleInstance(n) {
        _selectedInstances = Instances.toggleInstance(_selectedInstances, n)
    }

    function _setEnabled(on) {
        if (_sysFailureEn) {
            _sysFailureEn.value = on ? 1 : 0
        }
        _pendingReboot = true   // PX4 evaluates SYS_FAILURE_EN at boot, so any change (on or off) needs a reboot
    }

    function _unitName(unitEnum) {
        return Instances.unitName(_units, unitEnum)
    }

    function _typeName(typeEnum) {
        return Instances.typeName(_types, typeEnum)
    }

    // Send FAILURE_TYPE_OK (all instances) to every unit injected this session and log each as a row.
    // The activity list is kept; the injected-units set is cleared since those failures are now reverted.
    function _resetAll() {
        var injected = FailureInjection.injectedUnits()   // Q_INVOKABLE method — needs the call parentheses
        FailureInjection.clearInjectedUnits()
        for (var i = 0; i < injected.length; ++i) {
            _enqueueSend({ unit: injected[i], type: 0 /* FAILURE_TYPE_OK */, param3: 0 /* all instances */, param4: 0,
                           logArgs: { unitName: _unitName(injected[i]), typeName: _typeName(0), instanceLabel: "all", track: false } })
        }
    }

    // ---- page ---------------------------------------------------------------

    Component {
        id: pageComponent

        ColumnLayout {
            width:   availableWidth
            spacing: ScreenTools.defaultFontPixelHeight

            QGCLabel {
                Layout.fillWidth:   true
                wrapMode:           Text.WordWrap
                text:               qsTr("Injects simulated failures into flight components (sensors, GPS, motors, and more) to test failsafe behavior, " +
                                          "Use with care — an injected failure affects the vehicle immediately.")
            }

            // ---- SYS_FAILURE_EN gate + reboot ----------------------------------
            Rectangle {
                Layout.fillWidth:   true
                Layout.preferredHeight: gateCol.height + (ScreenTools.defaultFontPixelHeight * 1.2)
                color:              qgcPal.windowShade
                radius:             ScreenTools.defaultFontPixelWidth * 0.5

                ColumnLayout {
                    id:                 gateCol
                    anchors.margins:    ScreenTools.defaultFontPixelHeight * 0.6
                    anchors.left:       parent.left
                    anchors.right:      parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    spacing:            ScreenTools.defaultFontPixelHeight * 0.4

                    RowLayout {
                        Layout.fillWidth: true
                        spacing:          ScreenTools.defaultFontPixelWidth * 2

                        QGCCheckBox {
                            objectName: "failureInjection_enableCheckbox"
                            text:       qsTr("SYS_FAILURE_EN")
                            enabled:    _sysFailureEn !== null
                            checked:    _paramSet
                            onClicked:  _setEnabled(checked)
                        }
                        QGCLabel {
                            objectName: "failureInjection_pendingRebootLabel"
                            Layout.fillWidth: true
                            elide:      Text.ElideRight
                            color:      qgcPal.colorOrange
                            visible:    _pendingReboot
                            text:       qsTr("Reboot required to apply.")
                        }
                        QGCLabel {
                            objectName: "failureInjection_armedLabel"
                            Layout.fillWidth: true
                            color:      qgcPal.colorGreen
                            visible:    _armed && !_pendingReboot
                            text:       qsTr("Active — injection armed.")
                        }
                        QGCButton {
                            objectName: "failureInjection_rebootButton"
                            text:       qsTr("Reboot Vehicle")
                            visible:    _pendingReboot
                            onClicked: {
                                if (_activeVehicle) { _activeVehicle.rebootVehicle() }
                                _pendingReboot = false   // link drops & reconnects; param re-reads on return
                            }
                        }
                    }
                }
            }

            // ---- builder -------------------------------------------------------
            Rectangle {
                Layout.fillWidth:       true
                Layout.preferredHeight: builderCol.height + (ScreenTools.defaultFontPixelHeight * 1.2)
                color:                  qgcPal.windowShade
                radius:                 ScreenTools.defaultFontPixelWidth * 0.5
                enabled:                _armed
                opacity:                _armed ? 1.0 : 0.4

                ColumnLayout {
                    id:                 builderCol
                    anchors.margins:    ScreenTools.defaultFontPixelHeight * 0.6
                    anchors.left:       parent.left
                    anchors.right:      parent.right
                    anchors.top:        parent.top
                    spacing:            ScreenTools.defaultFontPixelHeight * 0.6

                    RowLayout {
                        spacing: ScreenTools.defaultFontPixelWidth * 2

                        ColumnLayout {
                            QGCLabel { text: qsTr("Component") }
                            QGCComboBox {
                                id:                 unitCombo
                                Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 24
                                model:              _units.map(function(u){ return u.name })
                                currentIndex:       _unitIndex
                                onActivated: function(index) {
                                    _unitIndex = index
                                    _selectedInstances = [1]   // reset selection on unit change
                                }
                            }
                        }
                        ColumnLayout {
                            QGCLabel { text: qsTr("Failure type") }
                            QGCComboBox {
                                Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 18
                                model:              _types.map(function(t){ return t.name })
                                currentIndex:       _typeIndex
                                onActivated:        function(index) { _typeIndex = index }
                            }
                        }
                        Item { Layout.fillWidth: true }
                        QGCButton {
                            objectName: "failureInjection_injectButton"
                            text:       qsTr("Inject failure")
                            primary:    true
                            enabled:    _armed && _selectedInstances.length > 0
                            onClicked:  _apply()
                        }
                    }

                    // multi-select instances ("All" = instance 0 = every instance of the unit)
                    QGCLabel { text: qsTr("Instances — select one or more, or All") }
                    Flow {
                        Layout.fillWidth: true
                        spacing:          ScreenTools.defaultFontPixelWidth * 2
                        Repeater {
                            model: _instanceCount
                            QGCCheckBox {
                                text:       "i = " + (index + 1)
                                checked:    _selectedInstances.indexOf(index + 1) >= 0
                                onClicked:  _toggleInstance(index + 1)
                            }
                        }
                        // vertical divider, then the "All" toggle, left-aligned right after the instances
                        Rectangle {
                            width:   1
                            height:  ScreenTools.defaultFontPixelHeight * 1.5
                            color:   qgcPal.text
                            opacity: 0.3
                        }
                        QGCCheckBox {
                            text:       qsTr("All (i = 0)")
                            checked:    _selectedInstances.indexOf(0) >= 0
                            // "All" is exclusive: selecting it clears specific instances, and vice versa
                            onClicked:  _selectedInstances = checked ? [0] : []
                        }
                    }

                    // live command preview
                    QGCLabel {
                        Layout.fillWidth: true
                        font.family:    ScreenTools.fixedFontFamily
                        color:          qgcPal.colorGreen
                        wrapMode:       Text.WordWrap
                        text: {
                            if (_selectedInstances.length === 0) {
                                return "MAV_CMD_INJECT_FAILURE  param1=" + _units[_unitIndex].unit +
                                       "  param2=" + _types[_typeIndex].type + "  (no instance selected)"
                            }
                            var u = _units[_unitIndex]
                            var t = _types[_typeIndex]
                            var s = _instanceSend()
                            var p3 = isNaN(s.param3) ? "NaN" : s.param3
                            return "MAV_CMD_INJECT_FAILURE  param1=" + u.unit +
                                   "  param2=" + t.type + "  param3=" + p3 +
                                   "  param4=" + s.param4 + "  (i=" + s.label + ")"
                        }
                    }
                }
            }

            // ---- activity log (disabled until armed, same as the builder) ------
            RowLayout {
                Layout.fillWidth: true
                enabled:          _armed
                opacity:          _armed ? 1.0 : 0.4
                QGCLabel { Layout.fillWidth: true; text: qsTr("Activity — newest first") }
                QGCButton { objectName: "failureInjection_resetAllButton"; text: qsTr("Reset all"); onClicked: _resetAll() }
            }
            // Column header, aligned with the rows below.
            RowLayout {
                Layout.fillWidth:    true
                Layout.leftMargin:   ScreenTools.defaultFontPixelWidth
                Layout.rightMargin:  ScreenTools.defaultFontPixelWidth
                spacing:             ScreenTools.defaultFontPixelWidth * 2
                enabled:             _armed
                opacity:             _armed ? 1.0 : 0.4
                QGCLabel { Layout.preferredWidth: _colTimeWidth; font.family: ScreenTools.fixedFontFamily; color: qgcPal.text; text: qsTr("Time") }
                QGCLabel { Layout.preferredWidth: _colUnitWidth; font.family: ScreenTools.fixedFontFamily; color: qgcPal.text; text: qsTr("Component") }
                QGCLabel { Layout.preferredWidth: _colTypeWidth; font.family: ScreenTools.fixedFontFamily; color: qgcPal.text; text: qsTr("Failure") }
                QGCLabel { Layout.fillWidth: true;               font.family: ScreenTools.fixedFontFamily; color: qgcPal.text; text: qsTr("Instances") }
                QGCLabel {                                       font.family: ScreenTools.fixedFontFamily; color: qgcPal.text; text: qsTr("Result") }
            }
            Rectangle {
                Layout.fillWidth:       true
                Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 12
                color:                  qgcPal.windowShadeDark
                radius:                 ScreenTools.defaultFontPixelWidth * 0.5
                enabled:                _armed
                opacity:                _armed ? 1.0 : 0.4

                QGCListView {
                    objectName:         "failureInjection_activityList"
                    anchors.fill:       parent
                    anchors.margins:    ScreenTools.defaultFontPixelWidth
                    clip:               true
                    model:              FailureInjection.activity
                    delegate: RowLayout {
                        required property var modelData
                        required property int index
                        readonly property bool _pending: modelData.result === "pending"
                        readonly property bool _accepted: modelData.result === "accepted"
                        objectName: "failureInjection_activityRow_" + index
                        width:   ListView.view.width
                        spacing: ScreenTools.defaultFontPixelWidth * 2
                        QGCLabel { Layout.preferredWidth: _colTimeWidth; font.family: ScreenTools.fixedFontFamily; color: qgcPal.colorOrange; text: modelData.time }
                        QGCLabel { objectName: "failureInjection_unitName_" + index; Layout.preferredWidth: _colUnitWidth; font.family: ScreenTools.fixedFontFamily; text: modelData.unitName }
                        QGCLabel { objectName: "failureInjection_typeName_" + index; Layout.preferredWidth: _colTypeWidth; font.family: ScreenTools.fixedFontFamily; text: modelData.typeName }
                        QGCLabel { objectName: "failureInjection_instance_" + index; Layout.fillWidth: true; font.family: ScreenTools.fixedFontFamily; elide: Text.ElideRight; text: modelData.instance }
                        QGCLabel {
                            objectName:  "failureInjection_result_" + index
                            font.family: ScreenTools.fixedFontFamily
                            color:       _pending ? qgcPal.colorOrange : (_accepted ? qgcPal.colorGreen : qgcPal.colorRed)
                            text:        _pending ? qsTr("…") : (_accepted ? qsTr("✓ Accepted") : ("× " + modelData.result))
                        }
                    }
                }
            }
        }
    }
}
