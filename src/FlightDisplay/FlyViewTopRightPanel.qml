/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Layouts
import QtQuick.Shapes

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlightDisplay
import QGroundControl.FlightMap
import QGroundControl.Palette
import QGroundControl.ScreenTools

Rectangle {
    id:             topRightPanel
    width:          _panelWidth
    height:         _panelHeight
    color:          "transparent"
    radius:         Math.round(ScreenTools.defaultFontPixelWidth * 0.78)
    border.color:   "transparent"
    border.width:   0
    visible:        panelExpanded && !QGroundControl.videoManager.fullScreen && _activeVehicle
    clip:           true

    property bool panelExpanded:            true
    property bool _settingEnableMVPanel:    QGroundControl.settingsManager.appSettings.enableMultiVehiclePanel.value
    property bool _multipleVehicles:        QGroundControl.multiVehicleManager.vehicles.count > 1
    property var  _activeVehicle:           QGroundControl.multiVehicleManager.activeVehicle
    property var  vehicles:                 QGroundControl.multiVehicleManager.vehicles
    property var  selectedVehicles:         QGroundControl.multiVehicleManager.selectedVehicles
    property var  backdropSourceItem:       null
    property real maximumHeight

    property real _panelHeight:             Math.min(maximumHeight > 0 ? maximumHeight : Math.max(560, _panelWidth * 1.32),
                                                     Math.max(560, Math.min(620, _panelWidth * 1.32)))
    property real _panelMargin:             Math.max(6, ScreenTools.defaultFontPixelWidth * 0.48)
    property real _panelSpacing:            Math.max(4, ScreenTools.defaultFontPixelHeight * 0.20)
    property real _panelWidth:              Math.max(410,
                                                     Math.min(parent ? parent.width * 0.225 : 430,
                                                              450))
    property real _contentHeight:           Math.max(1, _panelHeight - (_panelMargin * 2))
    property real _headerHeight:            Math.max(50, Math.min(60, _contentHeight * 0.105))
    property real _metricHeight:            Math.max(270, Math.min(312, _contentHeight * 0.48))
    property real _instrumentHeight:        Math.max(180, _contentHeight - _headerHeight - _metricHeight - (_panelSpacing * 2))
    property real _headingTapeHeight:       0
    property real _attitudeRowHeight:       Math.max(154, _instrumentHeight - (ScreenTools.defaultFontPixelHeight * 0.45))
    property real _scaleWidth:              Math.max(82, ScreenTools.defaultFontPixelWidth * 7.4)
    property real _scaleCompassGap:         Math.max(8, ScreenTools.defaultFontPixelWidth * 0.72)
    property real _compassSize:             Math.max(126, Math.min(_attitudeRowHeight * 0.58,
                                                                     _panelWidth - (_panelMargin * 2) - (_scaleWidth * 2) - (_scaleCompassGap * 2)))
    property real _compassReadoutGap:       Math.max(4, ScreenTools.defaultFontPixelHeight * 0.20)
    property real _scaleHeight:             Math.min(_attitudeRowHeight, _compassSize * 1.10)
    property string _panelFontFamily:       ScreenTools.normalFontFamily
    property real _panelTitlePointSize:     ScreenTools.titleFontPointSize
    property real _panelLabelPointSize:     ScreenTools.controlFontPointSize
    property real _panelValuePointSize:     ScreenTools.metricFontPointSize
    property real _panelAuxPointSize:       ScreenTools.labelFontPointSize
    property bool _selectionActive:         selectedVehicles && selectedVehicles.count > 0
    property bool _vehicleMenuOpen:         false

    QGCPalette { id: qgcPal }
    FlightModeDisplay { id: flightModeDisplay }

    GlassBackdrop {
        anchors.fill:       parent
        sourceItem:         topRightPanel.backdropSourceItem
        backdropBlurEnabled:true
        targetItem:         topRightPanel
        cornerRadius:       topRightPanel.radius
        sourceScale:        0.44
        blurAmount:         0.94
        blurMax:            46
        sourceBrightness:   -0.01
        sourceSaturation:   0.62
        tintColor:          Qt.rgba(0.045, 0.048, 0.052, 0.68)
        sheenColor:         "transparent"
    }

    function factText(fact, fallbackText) {
        if (!fact || fact.valueString === undefined) {
            return fallbackText
        }
        return fact.valueString + (fact.units && fact.units !== "" ? " " + fact.units : "")
    }

    function factNumberText(fact, fallbackText) {
        if (!fact || fact.valueString === undefined || fact.rawValue === undefined || isNaN(fact.rawValue)) {
            return fallbackText
        }
        return fact.valueString
    }

    function factRawValue(fact) {
        if (!fact || fact.rawValue === undefined || isNaN(fact.rawValue)) {
            return NaN
        }
        return fact.rawValue
    }

    function coordinateText(fact, fallbackText) {
        if (!fact || fact.rawValue === undefined || isNaN(fact.rawValue)) {
            return fallbackText
        }
        return Number(fact.rawValue).toFixed(7)
    }

    function primaryVoltageText(vehicle) {
        if (vehicle && vehicle.batteries && vehicle.batteries.count > 0) {
            var battery = vehicle.batteries.get(0)
            if (battery && battery.voltage && battery.voltage.valueString !== undefined && !isNaN(battery.voltage.rawValue)) {
                return battery.voltage.valueString + (battery.voltage.units && battery.voltage.units !== "" ? " " + battery.voltage.units : "")
            }
        }
        return qsTr("N/A")
    }

    function batteryText(vehicle) {
        if (vehicle && vehicle.batteries && vehicle.batteries.count > 0) {
            var battery = vehicle.batteries.get(0)
            if (battery && !isNaN(battery.percentRemaining.rawValue)) {
                return battery.percentRemaining.valueString + battery.percentRemaining.units
            }
            if (battery && !isNaN(battery.voltage.rawValue)) {
                return battery.voltage.valueString + " " + battery.voltage.units
            }
        }
        return qsTr("N/A")
    }

    function vehicleKindText(vehicle) {
        if (!vehicle) {
            return qsTr("Unknown")
        }
        if (vehicle.vehicleTypeString && vehicle.vehicleTypeString !== "" && vehicle.vehicleTypeString !== "MAV_TYPE_UNKNOWN") {
            return vehicle.vehicleTypeString
        }
        return qsTr("Unknown")
    }

    function vehicleDisplayName(vehicle) {
        return vehicle ? vehicleKindText(vehicle) + " #" + vehicle.id : qsTr("Unknown")
    }

    function activeActionLabel() {
        return _activeVehicle && _activeVehicle.armed ? qsTr("Disarm") : qsTr("Arm")
    }

    function activeActionEnabled() {
        return _guidedController.showArm || _guidedController.showDisarm
    }

    function activeActionCode() {
        return _activeVehicle && _activeVehicle.armed ? _guidedController.actionDisarm : _guidedController.actionArm
    }

    function toggleVehicleMenu() {
        if (vehicles && vehicles.count > 1) {
            _vehicleMenuOpen = !_vehicleMenuOpen
        }
    }

    function headingRaw(vehicle) {
        if (!vehicle || !vehicle.heading || vehicle.heading.rawValue === undefined || isNaN(vehicle.heading.rawValue)) {
            return NaN
        }
        var value = vehicle.heading.rawValue % 360
        return value < 0 ? value + 360 : value
    }

    function headingAtOffset(offset) {
        var heading = headingRaw(_activeVehicle)
        if (isNaN(heading)) {
            return NaN
        }
        var value = (heading + offset) % 360
        return value < 0 ? value + 360 : value
    }

    function headingDisplayText() {
        var heading = headingRaw(_activeVehicle)
        if (isNaN(heading)) {
            return "---"
        }
        var text = Math.round(heading).toString()
        while (text.length < 3) {
            text = "0" + text
        }
        return text + String.fromCharCode(176)
    }

    function headingTickText(offset) {
        var value = headingAtOffset(offset)
        if (isNaN(value)) {
            return "--"
        }
        var rounded = Math.round(value)
        if (rounded === 360) {
            rounded = 0
        }
        if (rounded % 45 === 0) {
            return cardinalText(rounded)
        }
        return rounded.toString()
    }

    function cardinalText(degrees) {
        var normalized = ((degrees % 360) + 360) % 360
        var names = [qsTr("N"), qsTr("NE"), qsTr("E"), qsTr("SE"), qsTr("S"), qsTr("SW"), qsTr("W"), qsTr("NW")]
        return names[Math.round(normalized / 45) % 8]
    }

    DeadMouseArea {
        anchors.fill: parent
    }

    component PanelLine: Rectangle {
        color: Qt.rgba(0.82, 0.90, 0.95, 0.13)
    }

    component TapeScale: Item {
        id: tape
        property string title: ""
        property string unit: ""
        property string valueText: ""
        property real numericValue: NaN
        property real stepValue: 25
        property real minimumValue: -1000000
        property bool leftScale: true
        property bool _hasValue: !isNaN(numericValue)
        property bool _hasMinimum: minimumValue > -999999
        property real _displayValue: _hasValue ? (_hasMinimum ? Math.max(numericValue, minimumValue) : numericValue) : 0
        property real _rulerValue: _hasMinimum ? Math.max(_displayValue, minimumValue + (stepValue * 3)) : _displayValue
        property real _centerY: height * 0.52
        property real _majorStepPixels: height * 0.155
        property real _minorDivisions: 5
        property real _baseMajorValue: Math.floor(_rulerValue / stepValue) * stepValue
        property real _firstMajorValue: _baseMajorValue - (stepValue * 3)
        property real _labelWidth: Math.max(ScreenTools.defaultFontPixelWidth * 6.2, ScreenTools.minTouchPixels * 0.82)
        property real _pointerWidth: Math.max(9, ScreenTools.defaultFontPixelWidth * 0.85)
        property real _badgeWidth: _labelWidth + _pointerWidth
        property real _majorTickWidth: Math.max(ScreenTools.defaultFontPixelWidth * 3.2, width * 0.34)
        property real _minorTickWidth: Math.max(ScreenTools.defaultFontPixelWidth * 2.0, width * 0.22)
        property real _tickLabelWidth: Math.max(ScreenTools.defaultFontPixelWidth * 4.0, ScreenTools.minTouchPixels * 0.55)
        property real _tickLabelGap: ScreenTools.defaultFontPixelWidth * 0.28
        property real _leftTickStartX: _badgeWidth - 0.5
        property real _rightTickEndX: width - _badgeWidth + 0.5

        Layout.preferredWidth:  Math.max(54, ScreenTools.defaultFontPixelWidth * 6.6)
        Layout.preferredHeight: _attitudeRowHeight
        Layout.minimumHeight:   _attitudeRowHeight
        Layout.maximumHeight:   _attitudeRowHeight
        Layout.alignment:       Qt.AlignVCenter

        function yForValue(value) {
            return _centerY - ((value - _rulerValue) / stepValue) * _majorStepPixels
        }

        function badgeCenterY() {
            return _hasValue ? yForValue(_displayValue) : _centerY
        }

        function majorValue(index) {
            return _firstMajorValue + index * stepValue
        }

        function minorValue(index) {
            return _firstMajorValue + index * (stepValue / _minorDivisions)
        }

        function tickText(index) {
            if (isNaN(numericValue)) {
                return "--"
            }
            var value = majorValue(index)
            if (value < minimumValue) {
                return ""
            }
            return Math.round(value).toString()
        }

        function tickLabelVisible(index, yPosition, labelHeight, labelText) {
            if (labelText === "" || yPosition <= ScreenTools.defaultFontPixelHeight * 2.05 || yPosition >= tape.height - labelHeight) {
                return false
            }
            if (_hasValue && Math.abs(majorValue(index) - _displayValue) < stepValue * 0.36) {
                return false
            }
            return true
        }

        function tickWidth(index) {
            return index % _minorDivisions === 0 ? _majorTickWidth : _minorTickWidth
        }

        function tickX(index) {
            var currentTickWidth = tickWidth(index)
            return leftScale ? _leftTickStartX : _rightTickEndX - currentTickWidth
        }

        function titleText() {
            if (title === "") {
                return ""
            }
            return unit === "" ? title : title + "(" + unit + ")"
        }

        function rulerCenterX() {
            return leftScale ? _leftTickStartX + (_majorTickWidth / 2)
                             : _rightTickEndX - (_majorTickWidth / 2)
        }

        QGCLabel {
            id:                     scaleTitle
            x:                      tape.rulerCenterX() - (width / 2)
            y:                      0
            width:                  Math.min(parent.width, Math.max(implicitWidth, tape._majorTickWidth + ScreenTools.defaultFontPixelWidth * 1.4))
            text:                   tape.titleText()
            color:                  qgcPal.text
            font.bold:              true
            font.family:            _panelFontFamily
            font.pointSize:         _panelAuxPointSize
            horizontalAlignment:    Text.AlignHCenter
            verticalAlignment:      Text.AlignVCenter
            fontSizeMode:           Text.HorizontalFit
            minimumPointSize:       ScreenTools.captionFontPointSize
            elide:                  Text.ElideNone
            maximumLineCount:       1
            visible:                text !== ""
        }

        Repeater {
            model: 36

            Rectangle {
                x:      tape.tickX(index)
                y:      tape.yForValue(tape.minorValue(index)) - height / 2
                width:  tape.tickWidth(index)
                height: index % tape._minorDivisions === 0 ? 2 : 1
                color:  Qt.rgba(1, 1, 1, index % tape._minorDivisions === 0 ? 0.82 : 0.34)
                visible: tape.minorValue(index) >= tape.minimumValue &&
                         y > ScreenTools.defaultFontPixelHeight * 2.25 &&
                         y < parent.height - ScreenTools.defaultFontPixelHeight * 0.35
            }
        }

        Repeater {
            model: 7

            QGCLabel {
                x:                      tape.leftScale ? tape._leftTickStartX - tape._tickLabelWidth - tape._tickLabelGap
                                                        : tape._rightTickEndX + tape._tickLabelGap
                y:                      tape.yForValue(tape.majorValue(index)) - height / 2
                width:                  tape._tickLabelWidth
                text:                   tape.tickText(index)
                color:                  qgcPal.buttonText
                font.family:            _panelFontFamily
                font.pointSize:         _panelAuxPointSize
                horizontalAlignment:    tape.leftScale ? Text.AlignRight : Text.AlignLeft
                visible:                tape.tickLabelVisible(index, y, height, text)
            }
        }

        Item {
            id:                 valueBadge
            x:                  tape.leftScale ? 0 : parent.width - width
            y:                  Math.max(ScreenTools.defaultFontPixelHeight * 2.10,
                                         Math.min(parent.height - height - ScreenTools.defaultFontPixelHeight * 0.35,
                                                  tape.badgeCenterY() - height / 2))
            width:              tape._badgeWidth
            height:             Math.max(ScreenTools.defaultFontPixelHeight * 0.86, 16)

            Shape {
                id:             badgeShape
                anchors.fill:   parent

                ShapePath {
                    strokeWidth: 1
                    strokeColor: qgcPal.primaryButton
                    fillColor:   "transparent"
                    startX:      tape.leftScale ? 0.5 : tape._pointerWidth + 0.5
                    startY:      0.5

                    PathLine { x: tape.leftScale ? badgeShape.width - tape._pointerWidth - 0.5 : badgeShape.width - 0.5; y: 0.5 }
                    PathLine { x: tape.leftScale ? badgeShape.width - tape._pointerWidth - 0.5 : badgeShape.width - 0.5; y: tape.leftScale ? badgeShape.height * 0.32 : badgeShape.height - 0.5 }
                    PathLine { x: tape.leftScale ? badgeShape.width - 0.5 : tape._pointerWidth + 0.5; y: tape.leftScale ? badgeShape.height * 0.50 : badgeShape.height - 0.5 }
                    PathLine { x: tape.leftScale ? badgeShape.width - tape._pointerWidth - 0.5 : tape._pointerWidth + 0.5; y: tape.leftScale ? badgeShape.height * 0.68 : badgeShape.height * 0.68 }
                    PathLine { x: tape.leftScale ? badgeShape.width - tape._pointerWidth - 0.5 : 0.5; y: tape.leftScale ? badgeShape.height - 0.5 : badgeShape.height * 0.50 }
                    PathLine { x: tape.leftScale ? 0.5 : tape._pointerWidth + 0.5; y: tape.leftScale ? badgeShape.height - 0.5 : badgeShape.height * 0.32 }
                    PathLine { x: tape.leftScale ? 0.5 : tape._pointerWidth + 0.5; y: 0.5 }
                }
            }

            QGCLabel {
                x:                      tape.leftScale ? 0 : tape._pointerWidth
                width:                  tape._labelWidth
                anchors.verticalCenter: parent.verticalCenter
                text:                   tape.valueText
                color:                  qgcPal.text
                font.bold:              true
                font.family:            _panelFontFamily
                font.pointSize:         _panelLabelPointSize
                horizontalAlignment:    Text.AlignHCenter
                verticalAlignment:      Text.AlignVCenter
                fontSizeMode:           Text.HorizontalFit
                minimumPointSize:       ScreenTools.captionFontPointSize
                elide:                  Text.ElideNone
            }
        }
    }

    component HeadingTape: Item {
        id: headingTape
        Layout.fillWidth:       true
        Layout.preferredHeight: _headingTapeHeight
        Layout.minimumHeight:   _headingTapeHeight
        Layout.maximumHeight:   _headingTapeHeight

        property real _headingValue: isNaN(topRightPanel.headingRaw(_activeVehicle)) ? 0 : topRightPanel.headingRaw(_activeVehicle)
        property real _stepValue: 15
        property real _minorDivisions: 3
        property real _centerX: width / 2
        property real _majorStepPixels: width / 6.0
        property real _baseMajorValue: Math.floor(_headingValue / _stepValue) * _stepValue
        property real _firstMajorValue: _baseMajorValue - (_stepValue * 3)
        property real _badgePointerHeight: 7
        property real _badgeBodyHeight: Math.max(16, ScreenTools.defaultFontPixelHeight * 0.86)

        function normalizeDegrees(value) {
            var normalized = Math.round(value) % 360
            return normalized < 0 ? normalized + 360 : normalized
        }

        function xForValue(value) {
            return _centerX + ((value - _headingValue) / _stepValue) * _majorStepPixels
        }

        function majorValue(index) {
            return _firstMajorValue + index * _stepValue
        }

        function minorValue(index) {
            return _firstMajorValue + index * (_stepValue / _minorDivisions)
        }

        function majorText(index) {
            if (isNaN(topRightPanel.headingRaw(_activeVehicle))) {
                return "--"
            }
            var value = normalizeDegrees(majorValue(index))
            return value % 45 === 0 ? topRightPanel.cardinalText(value) : value.toString()
        }

        Repeater {
            model: 7

            QGCLabel {
                x:                      headingTape.xForValue(headingTape.majorValue(index)) - width / 2
                y:                      0
                width:                  Math.max(ScreenTools.defaultFontPixelWidth * 4.0, contentWidth + 4)
                text:                   headingTape.majorText(index)
                color:                  qgcPal.buttonText
                font.family:            _panelFontFamily
                font.pointSize:         _panelAuxPointSize
                horizontalAlignment:    Text.AlignHCenter
                elide:                  Text.ElideRight
                visible:                x > -width && x < parent.width
            }
        }

        Repeater {
            model: 19

            Rectangle {
                x:      headingTape.xForValue(headingTape.minorValue(index))
                y:      ScreenTools.defaultFontPixelHeight * 1.55
                width:  1
                height: index % headingTape._minorDivisions === 0 ? ScreenTools.defaultFontPixelHeight * 0.54 : ScreenTools.defaultFontPixelHeight * 0.32
                color:  Qt.rgba(1, 1, 1, index % headingTape._minorDivisions === 0 ? 0.62 : 0.30)
                visible: x >= 0 && x <= parent.width
            }
        }

        Item {
            id: headingBadge
            anchors.horizontalCenter:   parent.horizontalCenter
            anchors.bottom:             parent.bottom
            width:                      Math.max(ScreenTools.defaultFontPixelWidth * 6.4, headingValue.contentWidth + ScreenTools.defaultFontPixelWidth * 1.1)
            height:                     headingTape._badgeBodyHeight + headingTape._badgePointerHeight

            Canvas {
                id: headingBadgeShape
                anchors.fill: parent

                onPaint: {
                    var ctx = getContext("2d")
                    ctx.reset()
                    var w = width
                    var h = height
                    var p = headingTape._badgePointerHeight
                    ctx.beginPath()
                    ctx.moveTo(w / 2, 0)
                    ctx.lineTo((w / 2) + p, p)
                    ctx.lineTo(w, p)
                    ctx.lineTo(w, h)
                    ctx.lineTo(0, h)
                    ctx.lineTo(0, p)
                    ctx.lineTo((w / 2) - p, p)
                    ctx.closePath()
                    ctx.fillStyle = Qt.rgba(0, 0, 0, 0)
                    ctx.strokeStyle = qgcPal.primaryButton
                    ctx.lineWidth = 1
                    ctx.fill()
                    ctx.stroke()
                }
            }

            QGCLabel {
                id:                     headingValue
                anchors.left:           parent.left
                anchors.right:          parent.right
                anchors.bottom:         parent.bottom
                height:                 headingTape._badgeBodyHeight
                text:                   topRightPanel.headingDisplayText()
                color:                  qgcPal.text
                font.bold:              true
                font.family:            _panelFontFamily
                font.pointSize:         _panelLabelPointSize
                horizontalAlignment:    Text.AlignHCenter
                verticalAlignment:      Text.AlignVCenter
            }
        }
    }

    component AttitudeCompass: Item {
        id: attitudeCompass

        property var vehicle: null
        property real headingValue: isNaN(topRightPanel.headingRaw(vehicle)) ? 0 : topRightPanel.headingRaw(vehicle)
        property real rollValue: vehicle && vehicle.roll && vehicle.roll.rawValue !== undefined && !isNaN(vehicle.roll.rawValue) ? vehicle.roll.rawValue : 0
        property real pitchValue: vehicle && vehicle.pitch && vehicle.pitch.rawValue !== undefined && !isNaN(vehicle.pitch.rawValue) ? vehicle.pitch.rawValue : 0

        Canvas {
            id: attitudeCanvas
            anchors.fill: parent

            property real headingValue: attitudeCompass.headingValue
            property real rollValue:    attitudeCompass.rollValue
            property real pitchValue:   attitudeCompass.pitchValue

            onHeadingValueChanged:  requestPaint()
            onRollValueChanged:     requestPaint()
            onPitchValueChanged:    requestPaint()
            onWidthChanged:         requestPaint()
            onHeightChanged:        requestPaint()

            onPaint: {
                var ctx = getContext("2d")
                ctx.reset()

                var w = width
                var h = height
                var cx = w / 2
                var cy = (h / 2) - (h * 0.03)
                var r = Math.max(1, Math.min(w, h) * 0.46)
                var heading = headingValue
                var roll = rollValue * Math.PI / 180
                var pitch = Math.max(-30, Math.min(30, pitchValue))
                var pitchOffset = (pitch / 30) * r * 0.42

                function sx(radius, degrees) {
                    var radians = degrees * Math.PI / 180
                    return cx + Math.sin(radians) * radius
                }

                function sy(radius, degrees) {
                    var radians = degrees * Math.PI / 180
                    return cy - Math.cos(radians) * radius
                }

                ctx.beginPath()
                ctx.arc(cx, cy, r * 1.06, 0, Math.PI * 2)
                ctx.fillStyle = Qt.rgba(0.16, 0.17, 0.18, 0.24)
                ctx.fill()

                ctx.save()
                ctx.beginPath()
                ctx.arc(cx, cy, r * 0.64, 0, Math.PI * 2)
                ctx.clip()
                ctx.translate(cx, cy)
                ctx.rotate(-roll)
                ctx.translate(0, pitchOffset)

                ctx.fillStyle = Qt.rgba(0.30, 0.62, 0.90, 0.94)
                ctx.fillRect(-r, -r * 1.35, r * 2, r * 1.35)
                ctx.fillStyle = Qt.rgba(0.82, 0.64, 0.26, 0.96)
                ctx.fillRect(-r, 0, r * 2, r * 1.35)

                ctx.strokeStyle = Qt.rgba(1.0, 1.0, 0.96, 0.92)
                ctx.lineWidth = Math.max(1.5, r * 0.018)
                ctx.beginPath()
                ctx.moveTo(-r * 0.72, 0)
                ctx.lineTo(r * 0.72, 0)
                ctx.stroke()

                ctx.strokeStyle = Qt.rgba(1.0, 1.0, 0.96, 0.48)
                ctx.lineWidth = 1
                for (var p = -20; p <= 20; p += 10) {
                    if (p === 0) {
                        continue
                    }
                    var lineY = -(p / 30) * r * 0.42
                    var lineW = Math.abs(p) === 20 ? r * 0.34 : r * 0.48
                    ctx.beginPath()
                    ctx.moveTo(-lineW / 2, lineY)
                    ctx.lineTo(lineW / 2, lineY)
                    ctx.stroke()
                }
                ctx.restore()

                ctx.strokeStyle = Qt.rgba(0.92, 0.94, 0.96, 0.14)
                ctx.lineWidth = Math.max(1, r * 0.018)
                ctx.beginPath()
                ctx.arc(cx, cy, r * 0.66, 0, Math.PI * 2)
                ctx.stroke()

                for (var i = 0; i < 36; i++) {
                    var degrees = (i * 10) - heading
                    var major = i % 3 === 0
                    var cardinal = i % 9 === 0
                    var outer = r * 0.99
                    var inner = major ? r * 0.86 : r * 0.91
                    ctx.strokeStyle = cardinal ? Qt.rgba(0.95, 0.97, 0.99, 0.72) :
                                                 (major ? Qt.rgba(0.95, 0.97, 0.99, 0.46) : Qt.rgba(0.95, 0.97, 0.99, 0.22))
                    ctx.lineWidth = cardinal ? Math.max(1.6, r * 0.020) : (major ? Math.max(1.2, r * 0.014) : 1)
                    ctx.beginPath()
                    ctx.moveTo(sx(inner, degrees), sy(inner, degrees))
                    ctx.lineTo(sx(outer, degrees), sy(outer, degrees))
                    ctx.stroke()
                }

                var labels = [topRightPanel.cardinalText(0), topRightPanel.cardinalText(90),
                              topRightPanel.cardinalText(180), topRightPanel.cardinalText(270)]
                var labelDegrees = [0, 90, 180, 270]
                ctx.font = "700 " + Math.max(11, r * 0.15) + "px \"" + _panelFontFamily + "\""
                ctx.textAlign = "center"
                ctx.textBaseline = "middle"
                for (var labelIndex = 0; labelIndex < labels.length; labelIndex++) {
                    var labelAngle = labelDegrees[labelIndex] - heading
                    ctx.fillStyle = labelDegrees[labelIndex] === 0 ? qgcPal.text : Qt.rgba(0.82, 0.85, 0.88, 0.78)
                    ctx.fillText(labels[labelIndex], sx(r * 0.74, labelAngle), sy(r * 0.74, labelAngle))
                }

                ctx.fillStyle = qgcPal.primaryButton
                ctx.beginPath()
                ctx.moveTo(cx, cy - r * 1.12)
                ctx.lineTo(cx - r * 0.055, cy - r * 1.00)
                ctx.lineTo(cx + r * 0.055, cy - r * 1.00)
                ctx.closePath()
                ctx.fill()

                ctx.strokeStyle = qgcPal.text
                ctx.lineWidth = Math.max(2, r * 0.026)
                ctx.lineCap = "round"
                ctx.beginPath()
                ctx.moveTo(cx - r * 0.36, cy)
                ctx.lineTo(cx - r * 0.12, cy)
                ctx.lineTo(cx, cy + r * 0.055)
                ctx.lineTo(cx + r * 0.12, cy)
                ctx.lineTo(cx + r * 0.36, cy)
                ctx.stroke()

                ctx.fillStyle = qgcPal.primaryButton
                ctx.beginPath()
                ctx.arc(cx, cy, Math.max(2.4, r * 0.034), 0, Math.PI * 2)
                ctx.fill()

                ctx.strokeStyle = Qt.rgba(0.92, 0.94, 0.96, 0.22)
                ctx.lineWidth = Math.max(1, r * 0.012)
                ctx.beginPath()
                ctx.arc(cx, cy, r * 1.02, 0, Math.PI * 2)
                ctx.stroke()
            }
        }

    }

    component MetricTile: Item {
        id: metricTile
        property string label: ""
        property string value: ""
        property color accent: qgcPal.text
        property real _innerMarginX: ScreenTools.defaultFontPixelWidth * 0.52
        property real _innerMarginY: ScreenTools.defaultFontPixelHeight * 0.18
        property real tileWidth: implicitWidth
        property real tileHeight: implicitHeight

        implicitWidth:          Math.max(1, (_panelWidth - (_panelMargin * 2)) / 3)
        implicitHeight:         _metricHeight / 2
        width:                  tileWidth
        height:                 tileHeight
        Layout.fillWidth:       true
        Layout.fillHeight:      true
        Layout.preferredWidth:  implicitWidth
        Layout.preferredHeight: _metricHeight / 2
        Layout.minimumWidth:    0
        Layout.minimumHeight:   0
        clip:                   true

        QGCLabel {
            id:                     metricLabel
            anchors.left:           parent.left
            anchors.right:          parent.right
            anchors.top:            parent.top
            anchors.leftMargin:     metricTile._innerMarginX
            anchors.rightMargin:    metricTile._innerMarginX
            anchors.topMargin:      metricTile._innerMarginY
            height:                 parent.height * 0.27
            text:                   metricTile.label
            color:                  qgcPal.buttonText
            font.family:            _panelFontFamily
            font.pointSize:         _panelLabelPointSize
            verticalAlignment:      Text.AlignBottom
            elide:                  Text.ElideRight
        }

        QGCLabel {
            id:                     metricValue
            anchors.left:           parent.left
            anchors.right:          parent.right
            anchors.top:            metricLabel.bottom
            anchors.leftMargin:     metricTile._innerMarginX
            anchors.rightMargin:    metricTile._innerMarginX
            height:                 parent.height * 0.38
            text:                   metricTile.value
            color:                  metricTile.accent
            font.bold:              true
            font.family:            _panelFontFamily
            font.pointSize:         _panelValuePointSize
            verticalAlignment:      Text.AlignVCenter
            elide:                  Text.ElideRight
        }

    }

    component ActionTile: Rectangle {
        id: actionTile
        property string title: ""
        property string iconSource: ""
        property bool available: false
        signal activated()

        Layout.fillWidth:       true
        Layout.fillHeight:      true
        radius:                 Math.round(ScreenTools.defaultFontPixelWidth * 0.34)
        color:                  available && mouseArea.pressed ? Qt.rgba(0.135, 0.140, 0.150, 0.24) : "transparent"
        border.color:           available ? qgcPal.primaryButton : Qt.rgba(0.82, 0.90, 0.95, 0.10)
        border.width:           1
        opacity:                available ? 1.0 : 0.50
        clip:                   true

        ColumnLayout {
            anchors.fill:       parent
            anchors.margins:    ScreenTools.defaultFontPixelWidth * 0.58
            spacing:            ScreenTools.defaultFontPixelHeight * 0.18

            QGCColoredImage {
                Layout.alignment:   Qt.AlignHCenter
                Layout.fillHeight:  true
                Layout.maximumHeight: ScreenTools.defaultFontPixelHeight * 1.75
                source:             actionTile.iconSource
                color:              available ? qgcPal.primaryButton : qgcPal.buttonText
                width:              ScreenTools.defaultFontPixelHeight * 1.55
                height:             width
                sourceSize.width:   width
                fillMode:           Image.PreserveAspectFit
            }

            QGCLabel {
                Layout.alignment:   Qt.AlignHCenter
                Layout.fillWidth:   true
                text:               actionTile.title
                color:              qgcPal.text
                font.bold:          true
                font.family:        _panelFontFamily
                font.pointSize:     _panelLabelPointSize
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment:  Text.AlignVCenter
                elide:              Text.ElideRight
                maximumLineCount:   1
            }
        }

        QGCMouseArea {
            id:             mouseArea
            anchors.fill:   parent
            enabled:        actionTile.available
            onClicked:      actionTile.activated()
        }
    }

    Item {
        id:                 panelLayout
        anchors.fill:       parent
        anchors.margins:    _panelMargin

        Rectangle {
            id:                     headerFrame
            z:                      10
            anchors.left:           parent.left
            anchors.right:          parent.right
            anchors.top:            parent.top
            height:                 _headerHeight
            radius:                 0
            color:                  "transparent"

            RowLayout {
                anchors.fill:       parent
                anchors.leftMargin: ScreenTools.defaultFontPixelWidth * 0.50
                anchors.rightMargin:ScreenTools.defaultFontPixelWidth * 0.50
                spacing:            ScreenTools.defaultFontPixelWidth * 0.70

                Rectangle {
                    Layout.alignment:   Qt.AlignVCenter
                    width:              ScreenTools.defaultFontPixelHeight * 0.62
                    height:             width
                    radius:             width / 2
                    color:              _activeVehicle && !_activeVehicle.vehicleLinkManager.communicationLost ? qgcPal.colorGreen : qgcPal.colorRed
                }

                Item {
                    id:                 vehicleSelector
                    Layout.fillWidth:   true
                    Layout.fillHeight:  true
                    Layout.minimumWidth: Math.max(150, ScreenTools.defaultFontPixelWidth * 12)

                    ColumnLayout {
                        anchors.left:           parent.left
                        anchors.right:          parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        spacing:                ScreenTools.defaultFontPixelHeight * 0.04

                        QGCLabel {
                            Layout.fillWidth:   true
                            text:               vehicleDisplayName(_activeVehicle)
                            color:              qgcPal.text
                            font.bold:          true
                            font.family:        _panelFontFamily
                            font.pointSize:     _panelTitlePointSize
                            elide:              Text.ElideRight
                        }

                        Item {
                            id:                 activeModeTextHost
                            Layout.fillWidth:   true
                            Layout.preferredHeight: Math.max(activeModeLabel.implicitHeight,
                                                             activeModeBadge.visible ? activeModeBadge.height : 0)

                            property string modeDisplayText: _activeVehicle ? flightModeDisplay.modeText(_activeVehicle, _activeVehicle.flightMode, qsTr("--")) : qsTr("--")

                            RowLayout {
                                id:                 activeModeRow
                                anchors.left:       parent.left
                                anchors.verticalCenter: parent.verticalCenter
                                spacing:            activeModeBadge.visible ? ScreenTools.defaultFontPixelWidth * 0.28 : 0

                                QGCLabel {
                                    id:                 activeModeLabel
                                    Layout.preferredWidth: Math.max(0, Math.min(implicitWidth,
                                                                                activeModeTextHost.width -
                                                                                (activeModeBadge.visible ? activeModeBadge.width + activeModeRow.spacing : 0)))
                                    Layout.maximumWidth: Layout.preferredWidth
                                    text:               flightModeDisplay.labelText(activeModeTextHost.modeDisplayText)
                                    color:              qgcPal.buttonText
                                    font.family:        _panelFontFamily
                                    font.pointSize:     _panelLabelPointSize
                                    verticalAlignment:  Text.AlignVCenter
                                    elide:              Text.ElideRight
                                    maximumLineCount:   1
                                }

                                Rectangle {
                                    id:                 activeModeBadge
                                    Layout.alignment:   Qt.AlignVCenter
                                    Layout.preferredWidth: activeModeBadgeText.implicitWidth + ScreenTools.defaultFontPixelWidth * 0.46
                                    Layout.preferredHeight: Math.max(ScreenTools.defaultFontPixelHeight * 0.70,
                                                                     activeModeBadgeText.implicitHeight + ScreenTools.defaultFontPixelHeight * 0.06)
                                    radius:             Math.round(height * 0.28)
                                    color:              Qt.rgba(0.82, 0.88, 0.94, 0.10)
                                    border.color:       Qt.rgba(0.82, 0.88, 0.94, 0.18)
                                    border.width:       1
                                    visible:            activeModeBadgeText.text !== ""

                                    QGCLabel {
                                        id:                     activeModeBadgeText
                                        anchors.centerIn:       parent
                                        text:                   flightModeDisplay.badgeText(activeModeTextHost.modeDisplayText)
                                        color:                  qgcPal.buttonText
                                        font.bold:              true
                                        font.family:            _panelFontFamily
                                        font.pointSize:         Math.max(7, ScreenTools.captionFontPointSize - 1)
                                        horizontalAlignment:    Text.AlignHCenter
                                        verticalAlignment:      Text.AlignVCenter
                                        maximumLineCount:       1
                                    }
                                }
                            }
                        }
                    }

                    QGCMouseArea {
                        anchors.fill:   parent
                        onClicked:      toggleVehicleMenu()
                    }
                }

                Item {
                    id:                 vehicleMenuToggle
                    Layout.alignment:   Qt.AlignVCenter
                    Layout.preferredWidth:  ScreenTools.defaultFontPixelHeight * 1.18
                    Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 1.18
                    enabled:            vehicles && vehicles.count > 1
                    opacity:            enabled ? 1.0 : 0.45

                    QGCColoredImage {
                        anchors.centerIn:   parent
                        source:             "qrc:/InstrumentValueIcons/cheveron-down.svg"
                        color:              vehicleMenuToggleMouse.containsMouse || _vehicleMenuOpen ? qgcPal.text : qgcPal.buttonText
                        width:              ScreenTools.defaultFontPixelHeight * 0.80
                        height:             width
                        sourceSize.width:   width
                        fillMode:           Image.PreserveAspectFit
                    }

                    QGCMouseArea {
                        id:             vehicleMenuToggleMouse
                        anchors.fill:   parent
                        hoverEnabled:   !ScreenTools.isMobile
                        enabled:        vehicleMenuToggle.enabled
                        onClicked:      toggleVehicleMenu()
                    }
                }

                PanelLine {
                    Layout.fillHeight:      true
                    Layout.preferredWidth:  1
                    Layout.topMargin:       ScreenTools.defaultFontPixelHeight * 0.35
                    Layout.bottomMargin:    ScreenTools.defaultFontPixelHeight * 0.35
                }

                ColumnLayout {
                    Layout.alignment:   Qt.AlignVCenter
                    Layout.preferredWidth: Math.max(92, ScreenTools.defaultFontPixelWidth * 6.0)
                    Layout.maximumWidth:   Math.max(118, ScreenTools.defaultFontPixelWidth * 7.0)
                    spacing:            0

                    QGCLabel {
                        Layout.fillWidth:   true
                        text:               qsTr("Battery")
                        color:              qgcPal.buttonText
                        font.family:        _panelFontFamily
                        font.pointSize:     _panelLabelPointSize
                        horizontalAlignment: Text.AlignHCenter
                    }

                    RowLayout {
                        Layout.alignment:   Qt.AlignHCenter
                        spacing:            ScreenTools.defaultFontPixelWidth * 0.30

                        QGCColoredImage {
                            Layout.alignment:   Qt.AlignVCenter
                            source:             "qrc:/InstrumentValueIcons/battery-full.svg"
                            color:              qgcPal.primaryButton
                            width:              ScreenTools.defaultFontPixelHeight * 1.05
                            height:             width
                            sourceSize.width:   width
                            fillMode:           Image.PreserveAspectFit
                        }

                        QGCLabel {
                            text:               batteryText(_activeVehicle)
                            color:              qgcPal.text
                            font.bold:          true
                            font.family:        _panelFontFamily
                            font.pointSize:     _panelTitlePointSize
                        }
                    }
                }
            }

            PanelLine {
                anchors.left:       parent.left
                anchors.right:      parent.right
                anchors.bottom:     parent.bottom
                height:             1
                visible:            false
            }

            Rectangle {
                id:                 vehicleMenu
                z:                  100
                anchors.left:       parent.left
                anchors.right:      parent.right
                anchors.top:        parent.bottom
                height:             Math.min(ScreenTools.defaultFontPixelHeight * 2.65 * vehicles.count,
                                             ScreenTools.defaultFontPixelHeight * 9.2)
                visible:            _vehicleMenuOpen && vehicles && vehicles.count > 1
                radius:             Math.round(ScreenTools.defaultFontPixelWidth * 0.44)
                color:              Qt.rgba(0.045, 0.048, 0.052, 1.0)
                border.color:       Qt.rgba(0.82, 0.90, 0.95, 0.16)
                border.width:       1
                clip:               true

                ColumnLayout {
                    anchors.fill:       parent
                    anchors.margins:    ScreenTools.defaultFontPixelWidth * 0.35
                    spacing:            0

                    Repeater {
                        model: vehicles

                        Rectangle {
                            id:                 vehicleMenuRow
                            Layout.fillWidth:   true
                            Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 2.36
                            radius:             Math.round(ScreenTools.defaultFontPixelWidth * 0.30)
                            color:              object === _activeVehicle ? Qt.rgba(1, 1, 1, 0.080) : (rowMouse.containsMouse ? Qt.rgba(1, 1, 1, 0.048) : "transparent")

                            RowLayout {
                                anchors.fill:       parent
                                anchors.leftMargin: ScreenTools.defaultFontPixelWidth * 0.60
                                anchors.rightMargin:ScreenTools.defaultFontPixelWidth * 0.60
                                spacing:            ScreenTools.defaultFontPixelWidth * 0.55

                                Rectangle {
                                    Layout.alignment:   Qt.AlignVCenter
                                    width:              ScreenTools.defaultFontPixelHeight * 0.52
                                    height:             width
                                    radius:             width / 2
                                    color:              object && !object.vehicleLinkManager.communicationLost ? qgcPal.colorGreen : qgcPal.buttonBorder
                                }

                                ColumnLayout {
                                    Layout.fillWidth:   true
                                    spacing:            0

                                    QGCLabel {
                                        Layout.fillWidth:   true
                                        text:               vehicleDisplayName(object)
                                        color:              qgcPal.text
                                        font.bold:          object === _activeVehicle
                                        font.family:        _panelFontFamily
                                        font.pointSize:     _panelLabelPointSize
                                        elide:              Text.ElideRight
                                    }

                                    Item {
                                        id:                 rowModeTextHost
                                        Layout.fillWidth:   true
                                        Layout.preferredHeight: Math.max(rowModeLabel.implicitHeight,
                                                                         rowModeBadge.visible ? rowModeBadge.height : 0)

                                        property string modeDisplayText: object ? flightModeDisplay.modeText(object, object.flightMode, qsTr("--")) : qsTr("--")

                                        RowLayout {
                                            id:                 rowModeTextRow
                                            anchors.left:       parent.left
                                            anchors.verticalCenter: parent.verticalCenter
                                            spacing:            rowModeBadge.visible ? ScreenTools.defaultFontPixelWidth * 0.28 : 0

                                            QGCLabel {
                                                id:                 rowModeLabel
                                                Layout.preferredWidth: Math.max(0, Math.min(implicitWidth,
                                                                                            rowModeTextHost.width -
                                                                                            (rowModeBadge.visible ? rowModeBadge.width + rowModeTextRow.spacing : 0)))
                                                Layout.maximumWidth: Layout.preferredWidth
                                                text:               flightModeDisplay.labelText(rowModeTextHost.modeDisplayText)
                                                color:              qgcPal.buttonText
                                                font.family:        _panelFontFamily
                                                font.pointSize:     _panelAuxPointSize
                                                verticalAlignment:  Text.AlignVCenter
                                                elide:              Text.ElideRight
                                                maximumLineCount:   1
                                            }

                                            Rectangle {
                                                id:                 rowModeBadge
                                                Layout.alignment:   Qt.AlignVCenter
                                                Layout.preferredWidth: rowModeBadgeText.implicitWidth + ScreenTools.defaultFontPixelWidth * 0.46
                                                Layout.preferredHeight: Math.max(ScreenTools.defaultFontPixelHeight * 0.66,
                                                                                 rowModeBadgeText.implicitHeight + ScreenTools.defaultFontPixelHeight * 0.04)
                                                radius:             Math.round(height * 0.28)
                                                color:              Qt.rgba(0.82, 0.88, 0.94, 0.10)
                                                border.color:       Qt.rgba(0.82, 0.88, 0.94, 0.18)
                                                border.width:       1
                                                visible:            rowModeBadgeText.text !== ""

                                                QGCLabel {
                                                    id:                     rowModeBadgeText
                                                    anchors.centerIn:       parent
                                                    text:                   flightModeDisplay.badgeText(rowModeTextHost.modeDisplayText)
                                                    color:                  qgcPal.buttonText
                                                    font.bold:              true
                                                    font.family:            _panelFontFamily
                                                    font.pointSize:         Math.max(7, ScreenTools.captionFontPointSize - 1)
                                                    horizontalAlignment:    Text.AlignHCenter
                                                    verticalAlignment:      Text.AlignVCenter
                                                    maximumLineCount:       1
                                                }
                                            }
                                        }
                                    }
                                }

                                QGCLabel {
                                    Layout.alignment:   Qt.AlignVCenter
                                    text:               batteryText(object)
                                    color:              object === _activeVehicle ? qgcPal.primaryButton : qgcPal.buttonText
                                    font.bold:          object === _activeVehicle
                                    font.family:        _panelFontFamily
                                    font.pointSize:     _panelLabelPointSize
                                }
                            }

                            QGCMouseArea {
                                id:         rowMouse
                                anchors.fill: parent
                                hoverEnabled: !ScreenTools.isMobile
                                onClicked: {
                                    if (object) {
                                        QGroundControl.multiVehicleManager.activeVehicle = object
                                    }
                                    _vehicleMenuOpen = false
                                }
                            }
                        }
                    }
                }
            }
        }

        Rectangle {
            id:                     instrumentFrame
            z:                      1
            anchors.left:           parent.left
            anchors.right:          parent.right
            anchors.top:            headerFrame.bottom
            anchors.topMargin:      _panelSpacing
            height:                 _instrumentHeight
            radius:                 0
            color:                  "transparent"
            clip:                   true

            Item {
                anchors.fill:       parent
                anchors.leftMargin: ScreenTools.defaultFontPixelWidth * 0.80
                anchors.rightMargin:ScreenTools.defaultFontPixelWidth * 0.80
                anchors.topMargin:  ScreenTools.defaultFontPixelHeight * 0.36
                anchors.bottomMargin:ScreenTools.defaultFontPixelHeight * 0.34

                Item {
                    id:                 attitudeBand
                    anchors.left:       parent.left
                    anchors.right:      parent.right
                    anchors.top:        parent.top
                    anchors.bottom:     parent.bottom

                    TapeScale {
                        id:             altitudeTape
                        anchors.left:   parent.left
                        anchors.leftMargin: ScreenTools.defaultFontPixelWidth * 0.80
                        anchors.verticalCenter: parent.verticalCenter
                        width:          _scaleWidth
                        height:         _scaleHeight
                        title:          qsTr("Altitude")
                        unit:           QGroundControl.unitsConversion.appSettingsVerticalDistanceUnitsString
                        valueText:      factNumberText(_activeVehicle ? _activeVehicle.altitudeRelative : null, qsTr("N/A"))
                        numericValue:   factRawValue(_activeVehicle ? _activeVehicle.altitudeRelative : null)
                        stepValue:      25
                        leftScale:      true
                    }

                    Item {
                        id:                 instrumentWell
                        anchors.left:       altitudeTape.right
                        anchors.right:      speedTape.left
                        anchors.top:        parent.top
                        anchors.bottom:     parent.bottom
                        anchors.leftMargin: _scaleCompassGap
                        anchors.rightMargin:_scaleCompassGap

                        Column {
                            id:                         compassStack
                            anchors.centerIn:           parent
                            width:                      parent.width
                            height:                     implicitHeight
                            spacing:                    _compassReadoutGap

                            QGCLabel {
                                width:                    parent.width
                                text:                     qsTr("HDG") + " " + topRightPanel.headingDisplayText()
                                color:                    qgcPal.text
                                font.bold:                true
                                font.family:              _panelFontFamily
                                font.pointSize:           _panelLabelPointSize
                                horizontalAlignment:      Text.AlignHCenter
                                verticalAlignment:        Text.AlignVCenter
                                maximumLineCount:         1
                            }

                            AttitudeCompass {
                                id:                     compassBox
                                width:                  _compassSize
                                height:                 width
                                anchors.horizontalCenter: parent.horizontalCenter
                                vehicle:                _activeVehicle
                            }
                        }
                    }

                    TapeScale {
                        id:             speedTape
                        anchors.right:  parent.right
                        anchors.rightMargin: ScreenTools.defaultFontPixelWidth * 0.80
                        anchors.verticalCenter: parent.verticalCenter
                        width:          _scaleWidth
                        height:         _scaleHeight
                        title:          qsTr("Speed")
                        unit:           QGroundControl.unitsConversion.appSettingsSpeedUnitsString
                        valueText:      factNumberText(_activeVehicle ? _activeVehicle.groundSpeed : null, qsTr("N/A"))
                        numericValue:   factRawValue(_activeVehicle ? _activeVehicle.groundSpeed : null)
                        stepValue:      10
                        minimumValue:   0
                        leftScale:      false
                    }
                }
            }

            PanelLine {
                anchors.left:       parent.left
                anchors.right:      parent.right
                anchors.bottom:     parent.bottom
                height:             1
                visible:            false
            }
        }

        Rectangle {
            id:                     metricsFrame
            anchors.left:           parent.left
            anchors.right:          parent.right
            anchors.top:            instrumentFrame.bottom
            anchors.topMargin:      _panelSpacing
            height:                 _metricHeight
            radius:                 0
            color:                  "transparent"
            clip:                   true

            property int  _metricColumns:     4
            property int  _metricRows:        5
            property real _metricCellWidth:   width / _metricColumns
            property real _bottomInset:       Math.max(10, ScreenTools.defaultFontPixelHeight * 0.72)
            property real _gridHeight:        Math.max(1, height - _bottomInset)
            property real _metricCellHeight:  _gridHeight / _metricRows

            Repeater {
                model: metricsFrame._metricColumns - 1

                delegate: PanelLine {
                    x:      (index + 1) * metricsFrame.width / metricsFrame._metricColumns
                    y:      0
                    width:  1
                    height: metricsFrame._gridHeight
                }
            }

            Repeater {
                model: metricsFrame._metricRows - 1

                delegate: PanelLine {
                    anchors.left:   metricsFrame.left
                    anchors.right:  metricsFrame.right
                    y:              (index + 1) * metricsFrame._gridHeight / metricsFrame._metricRows
                    height:         1
                }
            }

            Repeater {
                id: metricsGrid
                model: [
                    { "label": qsTr("Home Dist"),    "value": factText(_activeVehicle ? _activeVehicle.distanceToHome : null, qsTr("N/A")) },
                    { "label": qsTr("Next WP"),      "value": factText(_activeVehicle ? _activeVehicle.distanceToNextWP : null, qsTr("N/A")) },
                    { "label": qsTr("Climb"),        "value": factText(_activeVehicle ? _activeVehicle.climbRate : null, qsTr("N/A")) },
                    { "label": qsTr("Voltage"),      "value": primaryVoltageText(_activeVehicle) },
                    { "label": qsTr("Rel Alt"),      "value": factText(_activeVehicle ? _activeVehicle.altitudeRelative : null, qsTr("N/A")) },
                    { "label": qsTr("AMSL Alt"),     "value": factText(_activeVehicle ? _activeVehicle.altitudeAMSL : null, qsTr("N/A")) },
                    { "label": qsTr("Thr"),          "value": factText(_activeVehicle ? _activeVehicle.throttlePct : null, qsTr("N/A")) },
                    { "label": qsTr("GPS HDG"),      "value": factText(_activeVehicle && _activeVehicle.gps ? _activeVehicle.gps.courseOverGround : null, qsTr("N/A")) },
                    { "label": qsTr("Flight Time"),  "value": factText(_activeVehicle ? _activeVehicle.flightTime : null, qsTr("00:00:00")) },
                    { "label": qsTr("Flight Dist"),  "value": factText(_activeVehicle ? _activeVehicle.flightDistance : null, qsTr("N/A")) },
                    { "label": qsTr("Home Time"),    "value": factText(_activeVehicle ? _activeVehicle.timeToHome : null, qsTr("N/A")) },
                    { "label": qsTr("Course"),       "value": factText(_activeVehicle ? _activeVehicle.headingToNextWP : null, qsTr("N/A")) },
                    { "label": qsTr("Pitch"),        "value": factText(_activeVehicle ? _activeVehicle.pitch : null, qsTr("N/A")) },
                    { "label": qsTr("Roll"),         "value": factText(_activeVehicle ? _activeVehicle.roll : null, qsTr("N/A")) },
                    { "label": qsTr("Wind Spd"),     "value": factText(_activeVehicle && _activeVehicle.wind ? _activeVehicle.wind.speed : null, qsTr("N/A")) },
                    { "label": qsTr("Wind Dir"),     "value": factText(_activeVehicle && _activeVehicle.wind ? _activeVehicle.wind.direction : null, qsTr("N/A")) },
                    { "label": qsTr("Heading"),      "value": factText(_activeVehicle ? _activeVehicle.heading : null, qsTr("N/A")) },
                    { "label": qsTr("Lat"),          "value": coordinateText(_activeVehicle && _activeVehicle.gps ? _activeVehicle.gps.lat : null, qsTr("N/A")) },
                    { "label": qsTr("Lon"),          "value": coordinateText(_activeVehicle && _activeVehicle.gps ? _activeVehicle.gps.lon : null, qsTr("N/A")) },
                    { "label": qsTr("IMU Temp"),     "value": factText(_activeVehicle ? _activeVehicle.imuTemp : null, qsTr("N/A")) }
                ]

                delegate: Item {
                    x:              (index % metricsFrame._metricColumns) * metricsFrame._metricCellWidth
                    y:              Math.floor(index / metricsFrame._metricColumns) * metricsFrame._metricCellHeight
                    width:          metricsFrame._metricCellWidth
                    height:         metricsFrame._metricCellHeight
                    clip:           true

                    property real _innerMarginX: Math.max(4, ScreenTools.defaultFontPixelWidth * 0.44)
                    property real _innerMarginY: Math.max(2, ScreenTools.defaultFontPixelHeight * 0.10)

                    QGCLabel {
                        id:                     metricLabel
                        anchors.left:           parent.left
                        anchors.right:          parent.right
                        anchors.top:            parent.top
                        anchors.leftMargin:     parent._innerMarginX
                        anchors.rightMargin:    parent._innerMarginX
                        anchors.topMargin:      parent._innerMarginY * 1.15
                        height:                 parent.height * 0.34
                        text:                   modelData.label
                        color:                  qgcPal.buttonText
                        font.family:            _panelFontFamily
                        font.pointSize:         Math.max(8, _panelLabelPointSize)
                        verticalAlignment:      Text.AlignBottom
                        fontSizeMode:           Text.HorizontalFit
                        minimumPointSize:       Math.max(6, ScreenTools.captionFontPointSize - 1)
                        elide:                  Text.ElideRight
                        maximumLineCount:       1
                    }

                    QGCLabel {
                        id:                     metricValue
                        anchors.left:           metricLabel.left
                        anchors.right:          parent.right
                        anchors.top:            metricLabel.bottom
                        anchors.bottom:         parent.bottom
                        anchors.rightMargin:    parent._innerMarginX
                        anchors.bottomMargin:   parent._innerMarginY * 1.10
                        text:                   modelData.value
                        color:                  qgcPal.text
                        font.bold:              true
                        font.family:            _panelFontFamily
                        font.pointSize:         Math.max(10, _panelValuePointSize * 0.82)
                        verticalAlignment:      Text.AlignVCenter
                        fontSizeMode:           Text.HorizontalFit
                        minimumPointSize:       Math.max(7, ScreenTools.captionFontPointSize)
                        elide:                  Text.ElideRight
                        maximumLineCount:       1
                    }
                }
            }

            PanelLine {
                anchors.left:       parent.left
                anchors.right:      parent.right
                anchors.bottom:     parent.bottom
                height:             1
                visible:            false
            }
        }

    }
}
