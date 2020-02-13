/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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

/// The PreFlightCheckButton supports creating a button which the user then has to verify/click to confirm a check.
/// It also supports failing the check based on values from within the system: telemetry or QGC app values. These
/// controls are normally placed within a PreFlightCheckGroup.
///
/// Two types of checks may be included on the button:
///     Manual - This is simply a check which the user must verify and confirm. It is not based on any system state.
///     Telemetry - This type of check can fail due to some state within the system. A telemetry check failure can be
///                 a hard stop in that there is no way to pass the checklist until the system state resolves itself.
///                 Or it can also optionally be override by the user.
/// If a button uses both manual and telemetry checks, the telemetry check takes precendence and must be passed first.
QGCButton {
    property string name:                           ""
    property string manualText:                     ""      ///< text to show for a manual check, "" signals no manual check
    property string telemetryTextFailure                    ///< text to show if telemetry check failed (override not allowed)
    property bool   telemetryFailure:               false   ///< true: telemetry check failing, false: telemetry check passing
    property bool   allowTelemetryFailureOverride:  false   ///< true: user can click past telemetry failure
    property bool   passed:                         _manualState === _statePassed && _telemetryState === _statePassed
    property bool   failed:                         _manualState === _stateFailed || _telemetryState === _stateFailed

    property int _manualState:          manualText === "" ? _statePassed : _statePending
    property int _telemetryState:       _statePassed
    property int _horizontalPadding:    ScreenTools.defaultFontPixelWidth
    property int _verticalPadding:      Math.round(ScreenTools.defaultFontPixelHeight / 2)
    property real _stateFlagWidth:      ScreenTools.defaultFontPixelWidth * 4

    readonly property int _statePending:    0   ///< Telemetry check is failing or manual check not yet verified, user can click to make it pass
    readonly property int _stateFailed:     1   ///< Telemetry check is failing, user cannot click to make it pass
    readonly property int _statePassed:     2   ///< Check has passed

    readonly property color _passedColor:   "#86cc6a"
    readonly property color _pendingColor:  "#f7a81f"
    readonly property color _failedColor:   "#c31818"

    property string _text: "<b>" + name +"</b>: " +
                           ((_telemetryState !== _statePassed) ?
                               telemetryTextFailure :
                               (_manualState !== _statePassed ? manualText : qsTr("Passed")))
    property color  _color: _telemetryState === _statePassed && _manualState === _statePassed ?
                                _passedColor :
                                (_telemetryState == _stateFailed ?
                                     _failedColor :
                                     (_telemetryState === _statePending || _manualState === _statePending ?
                                          _pendingColor :
                                          _failedColor))

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

    function _updateTelemetryState() {
        if (telemetryFailure) {
            // We have a new telemetry failure, reset user pass
            _telemetryState = allowTelemetryFailureOverride ? _statePending : _stateFailed
        } else {
            _telemetryState = _statePassed
        }
    }

    onTelemetryFailureChanged:              _updateTelemetryState()
    onAllowTelemetryFailureOverrideChanged: _updateTelemetryState()

    onClicked: {
        if (telemetryFailure && !allowTelemetryFailureOverride) {
            // No way to proceed past this failure
            return
        }
        if (telemetryFailure && allowTelemetryFailureOverride && _telemetryState !== _statePassed) {
            // User is allowed to proceed past this failure
            _telemetryState = _statePassed
            return
        }
        if (manualText !== "" && _manualState !== _statePassed) {
            // User is confirming a manual check
            _manualState = _statePassed
        }
    }

    onPassedChanged: callButtonPassedChanged()
    onParentChanged: callButtonPassedChanged()

    function callButtonPassedChanged() {
        if (typeof parent.buttonPassedChanged === "function") {
            parent.buttonPassedChanged()
        }
    }

    function reset() {
        _manualState = manualText === "" ? _statePassed : _statePending
        if (telemetryFailure) {
            _telemetryState = allowTelemetryFailureOverride ? _statePending : _stateFailed
        } else {
            _telemetryState = _statePassed
        }
    }

}
