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

QGCButton {
    property string name:           ""
    property int    group:          0
    property string defaultText:    qsTr("Not checked yet")
    property string pendingText:    ""
    property string failureText:    qsTr("Failure. Check console.")
    property int    state:         stateNotChecked

    readonly property int stateNotChecked:  0
    readonly property int statePending:     1
    readonly property int stateMinorIssue:  2
    readonly property int stateMajorIssue:  3
    readonly property int statePassed:      4

    property var    _color:         qgcPal.button
    property int    _nrClicked:     0
    property string _text:          name + ": " + defaultText
    property var    _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle

    enabled:    (!_activeVehicle || _activeVehicle.connectionLost) ? false : preFlightCheckList._checkState >= group
    opacity:    (!_activeVehicle || _activeVehicle.connectionLost) ? 0.4 : 0.2 + (0.8 * (preFlightCheckList._checkState >= group))
    width:      40 * ScreenTools.defaultFontPixelWidth

    style: ButtonStyle {
        background: Rectangle {color:_color; border.color: qgcPal.button; radius:3}
        label: Label {
            text:                   _text
            wrapMode:               Text.WordWrap
            horizontalAlignment:    Text.AlignHCenter
            color:                  state > 0 ? qgcPal.mapWidgetBorderLight : qgcPal.buttonText
        }
    }

    onPendingTextChanged: { if (state === statePending) { getTextFromState(); getColorFromState(); } }
    onFailureTextChanged: { if (state === stateMajorIssue) { getTextFromState(); getColorFromState(); } }
    onStateChanged: { getTextFromState(); getColorFromState(); }
    onClicked: {
        if (state <= statePending) {
            _nrClicked = _nrClicked + 1 //Only allow click-counter to increase when not failed yet
        }
        updateItem()
    }

    function updateItem() {
        // This is the default updateFunction. It assumes the item is a MANUAL check list item, i.e. one that
        // only requires user clicks (one click if pendingText="", two clicks otherwise) for completion.

        if (_nrClicked === 0) {
            state = stateNotChecked
        } else if (_nrClicked === 1) {
            if (pendingText.length === 0) {
                state = statePassed
            } else {
                state = statePending
            }
        } else {
            state = statePassed
        }

        getTextFromState();
        getColorFromState();
    }

    function getTextFromState() {
        if (state === stateNotChecked) {
            _text = qsTr(name) + ": " + qsTr(defaultText)
        } else if (state === statePending) {
            _text = "<b>"+qsTr(name)+"</b>" +": " + pendingText
        } else if (state === stateMinorIssue) {
            _text = "<b>"+qsTr(name)+"</b>" +": " + qsTr("Minor problem")
        } else if (state === stateMajorIssue) {
            _text = "<b>"+qsTr(name)+"</b>" +": " + failureText
        } else if (state === statePassed) {
            _text = "<b>"+qsTr(name)+"</b>" +": " + qsTr("OK")
        } else {
            console.warn("Internal Error: invalid state", state)
        }
    }

    function getColorFromState() {
        if (state === stateNotChecked) {
            _color = qgcPal.button
        } else if (state === statePending) {
            _color = Qt.rgba(0.9,0.47,0.2,1)
        } else if (state === stateMinorIssue) {
            _color = Qt.rgba(1.0,0.6,0.2,1)
        } else if (state === stateMajorIssue) {
            _color = Qt.rgba(0.92,0.22,0.22,1)
        } else if (state === statePassed ) {
            _color = Qt.rgba(0.27,0.67,0.42,1)
        } else {
            console.warn("Internal Error: invalid state", state)
        }
    }

    function resetNrClicks() {
        _nrClicked=0;
        updateItem();
    }
}
