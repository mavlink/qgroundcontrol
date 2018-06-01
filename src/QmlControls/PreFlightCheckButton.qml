/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick                  2.3
import QtQuick.Controls         1.2
import QtQuick.Controls.Styles  1.4

import QGroundControl               1.0
import QGroundControl.Palette       1.0
import QGroundControl.ScreenTools   1.0

/// The PreFlightCheckButtons supports creating a button which the user then has to verify/click to confirm a check.
/// It also supports failing the check based on values from within the system: telemetry or QGC app values.
///
/// Two types of checks may be included on the button:
///     Manual - This is simply a check which the user must verify and confirm. It is not based on any system state.
///     Telemetry - This type of check can fail due to some state within the system. A telemetry check failure can be
///                 a hard stop in that there is no way to pass the checklist until the system state resolves itself.
///                 Or it can also optionall be override by the user.
/// If a button uses both manual and telemetry checks, the telemetry check takes precendence and must be passed first.
QGCButton {
    property string name:                  ""
    property int    group:                  0
    property string manualText:             ""      ///< text to show for a manual check, "" signals no manual check
    property string telemetryTextOverride:  ""      ///< text to show if telemetry check failed and override is allowed
    property string telemetryTextFailure            ///< text to show if telemetry check failed (override not allowed)
    property bool   telemetryFailure:       false   ///< true: telemetry check failing, false: telemetry check passing
    property bool   passed:                 _manualState === _statePassed && _telemetryState === _statePassed

    property int _manualState:          manualText === "" ? _statePassed : _statePending
    property int _telemetryState:       _statePassed
    property int _horizontalPadding:    ScreenTools.defaultFontPixelWidth
    property int _verticalPadding:      Math.round(ScreenTools.defaultFontPixelHeight / 2)
    property real _stateFlagWidth:      ScreenTools.defaultFontPixelWidth * 4

    readonly property int _statePending:    0   ///< Telemetry check has failed or manual check not yet verified, user can click to make it pass
    readonly property int _stateFailed:     1   ///< Telemetry check has failed, user cannot click to make it pass
    readonly property int _statePassed:     2   ///< Check has passed

    readonly property color _passedColor:   Qt.rgba(0.27,0.67,0.42,1)
    readonly property color _pendingColor:  Qt.rgba(0.9,0.47,0.2,1)
    readonly property color _failedColor:   Qt.rgba(0.92,0.22,0.22,1)

    property string _text: "<b>" + name +"</b>: " +
                           ((_telemetryState !== _statePassed) ?
                               (_telemetryState === _statePending ? telemetryTextOverride : telemetryTextFailure) :
                               (_manualState !== _statePassed ? manualText : qsTr("OK")))
    property color  _color: _telemetryState === _statePassed && _manualState === _statePassed ?
                                _passedColor :
                                (_telemetryState == _stateFailed ?
                                     _failedColor :
                                     (_telemetryState === _statePending || _manualState === _statePending ?
                                          _pendingColor :
                                          _failedColor))

    property var    _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle
    property bool   _allowTelemetryFailureOverride:  telemetryTextOverride !== ""

    enabled:    preFlightCheckList._checkState >= group
    opacity:    0.2 + (0.8 * (preFlightCheckList._checkState >= group))
    width:      40 * ScreenTools.defaultFontPixelWidth

    style: ButtonStyle {
        padding {
            top:    _verticalPadding
            bottom: _verticalPadding
            left:   (_horizontalPadding * 2) + _stateFlagWidth
            right:  _horizontalPadding
        }

        background: Rectangle {
            color:          qgcPal.button
            border.color:   qgcPal.button;
            radius:         3

            Rectangle {
                color:          _color
                anchors.left:   parent.left
                anchors.top:    parent.top
                anchors.bottom: parent.bottom
                width:          _stateFlagWidth
            }
        }

        label: Label {
            text:                   _text
            wrapMode:               Text.WordWrap
            horizontalAlignment:    Text.AlignHCenter
            color:                  qgcPal.buttonText
        }
    }

    onTelemetryFailureChanged: {
        if (telemetryFailure) {
            // We have a new telemetry failure, reset user pass
            _telemetryState = _allowTelemetryFailureOverride ? _statePending : _stateFailed
        } else {
            _telemetryState = _statePassed
        }
    }

    onClicked: {
        if (telemetryFailure && !_allowTelemetryFailureOverride) {
            // No way to proceed past this failure
            return
        }
        if (telemetryFailure && _allowTelemetryFailureOverride && _telemetryState !== _statePassed) {
            // User is allowed to proceed past this failure
            _telemetryState = _statePassed
            return
        }
        if (manualText !== "" && _manualState !== _statePassed) {
            // User is confirming a manual check
            _manualState = _statePassed
        }
    }

    function reset() {
        _manualState = manualText === "" ? statePass : _statePending
        if (telemetryFailure) {
            _telemetryState = _allowTelemetryFailureOverride ? _statePending : _stateFailed
        } else {
            _telemetryState = _statePassed
        }
    }

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }
}
